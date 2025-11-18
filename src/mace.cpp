//
// Created by Tamas K Stenczel on 07/07/2024.
//

#include "mace.h"

#include "neighbour_list.h"

#include <torch/torch.h>
#include <torch/script.h>

/*
 * Interface definitions for helpers
 */

torch::Device get_device();

torch::jit::Module load_mace_model(const std::string &model_path, c10::Device device);

void model_analyse(const torch::jit::Module &model);

torch::ScalarType model_get_dtype(const torch::jit::Module &model);

double model_get_rmax(const torch::jit::Module &model);

int64_t model_get_num_interactions(const torch::jit::Module &model);

std::vector<int64_t> model_get_atomic_numbers(const torch::jit::Module &model);

/*
 * Class definitions
 */

MACE::MACE(const std::string &model_path) {
    this->model_path = model_path;
    device = get_device();

    // reload sets other internals
    reload();
}

void MACE::reload() {
    // load the model from file
    model = load_mace_model(model_path, device);

    // model settings
    torch_float_dtype = model_get_dtype(model);
    mace_atomic_numbers = model_get_atomic_numbers(model);
    r_max = model_get_rmax(model);

    // construct mapping from Z -> MACE element index
    std::size_t z_idx = 0;
    for (auto z: mace_atomic_numbers) {
        z_to_mace_type[z] = z_idx;
        z_idx++;
    }
}

void MACE::print() const {
    // extract default dtype from mace model
    std::cout << "  - The torch_float_dtype is: " << torch_float_dtype << std::endl;

    // extract r_max from mace model
    std::cout << "  - The r_max is: " << r_max << "." << std::endl;
    const auto num_interactions = model_get_num_interactions(model);
    std::cout << "  - The model has: " << num_interactions << " layers." << std::endl;

    // extract atomic numbers from mace model
    std::cout << "  - The MACE model atomic numbers are: " << mace_atomic_numbers << "." << std::endl;
}

CalculationResult
MACE::calculate(
    const real2d<3, 3> &cell,
    const bool1d<3> &pbc,
    const std::vector<std::array<double, 3> > &positions,
    const std::vector<int> &atomic_numbers,
    const bool &calc_virial
) {
    // ----- edge_index and unit_shifts -----
    // compute & unpack - neighbour list
    const auto cutoff = model_get_rmax(model);
    auto [nl_edge_index, nl_unit_shifts, nl_cell] = calc_mace_neighbour_list(cell, pbc, positions, cutoff);
    const int n_edges = nl_edge_index.size();
    auto edge_index = torch::empty({2, n_edges}, torch::dtype(torch::kInt64));
    auto unit_shifts = torch::zeros({n_edges, 3}, torch_float_dtype);
    for (int i_edge = 0; i_edge < n_edges; i_edge++) {
        edge_index[0][i_edge] = nl_edge_index[i_edge][0];
        edge_index[1][i_edge] = nl_edge_index[i_edge][1];
        for (int j = 0; j < 3; j++) {
            unit_shifts[i_edge][j] = nl_unit_shifts[i_edge][j];
        }
    }

    // ----- cell -----
    auto torch_cell = torch::zeros({3, 3}, torch_float_dtype);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            torch_cell[i][j] = nl_cell[i][j];
        }
    }

    // based on mace.data.neighbourhood.get_neighborhood(...), this is
    // `shifts = np.dot(unit_shifts, cell)  # [n_edges, 3]` which is based on the
    // matscipy docs. I am wondering if getting `D` from the neighbour list leads to
    // this with fewer operations? Probably this has no significance compared to the
    // rest of the computations within the forward pass.
    auto shifts = unit_shifts.matmul(torch_cell); // todo: will this computation work if we use a GPU?


    // ----- positions -----
    const int n_nodes = positions.size(); // Clang-Tidy complains about conversion here
    if (n_nodes != atomic_numbers.size()) {
        throw std::logic_error("Length mis-match between positions & types!");
    }
    int i_atom = 0;
    auto torch_positions = torch::empty({n_nodes, 3}, torch_float_dtype);
    for (auto row: positions) {
        for (int j = 0; j < 3; j++) {
            torch_positions[i_atom][j] = row[j];
        }
        i_atom++;
    }

    // ----- node_attrs -----
    int n_node_feats = mace_atomic_numbers.size();
    auto node_attrs = torch::zeros({n_nodes, n_node_feats}, torch_float_dtype);
    i_atom = 0;
    for (const auto z: atomic_numbers) {
        node_attrs[i_atom][z_to_mace_type[z]] = 1.0;
        i_atom++;
    }

    // ----- mask for ghost -----
    // irrelevant for us, ghost atoms are not supported, so all are 1.
    const auto mask = torch::ones(n_nodes, dtype(torch::kBool));

    // containers for results
    auto batch = torch::zeros({n_nodes}, torch::dtype(torch::kInt64));
    auto energy = torch::empty({1}, torch_float_dtype);
    auto forces = torch::empty({n_nodes, 3}, torch_float_dtype);
    auto ptr = torch::empty({2}, torch::dtype(torch::kInt64));
    auto weight = torch::empty({1}, torch_float_dtype);
    ptr[0] = 0;
    ptr[1] = n_nodes;
    weight[0] = 1.0;

    // transfer data to device
    batch = batch.to(device);
    torch_cell = torch_cell.to(device);
    edge_index = edge_index.to(device);
    energy = energy.to(device);
    forces = forces.to(device);
    node_attrs = node_attrs.to(device);
    torch_positions = torch_positions.to(device);
    ptr = ptr.to(device);
    shifts = shifts.to(device);
    unit_shifts = unit_shifts.to(device);
    weight = weight.to(device);

    // pack the input, call the model
    c10::Dict<std::string, torch::Tensor> input;
    input.insert("batch", batch);
    input.insert("cell", torch_cell);
    input.insert("edge_index", edge_index);
    input.insert("energy", energy);
    input.insert("forces", forces);
    input.insert("node_attrs", node_attrs);
    input.insert("positions", torch_positions);
    input.insert("ptr", ptr);
    input.insert("shifts", shifts);
    input.insert("unit_shifts", unit_shifts);
    input.insert("weight", weight);
    auto output = model.forward({input, mask.to(device), calc_virial}).toGenericDict();

    // ----- Unpack the results -----

    // mace energy
    const auto mace_energy = output.at("total_energy_local").toTensor().cpu().item<double>();

    // mace site energies
    std::vector<double> mace_node_energy(n_nodes);
    const auto node_energy = output.at("node_energy").toTensor().cpu();
    for (i_atom = 0; i_atom < n_nodes; i_atom++) {
        mace_node_energy[i_atom] = node_energy[i_atom].item<double>();
    }

    // mace forces
    //   -> derivatives of total mace energy
    std::vector<real1d<3> > mace_forces(n_nodes);
    auto out_forces = output.at("forces").toTensor().cpu();
    for (i_atom = 0; i_atom < n_nodes; i_atom++) {
        for (int j = 0; j < 3; j++) {
            mace_forces[i_atom][j] = out_forces[i_atom][j].item<double>();
        }
    }

    // mace virials
    //   -> derivatives of sum of site energies of local atoms
    real1d<6> mace_virial;
    if (calc_virial) {
        const auto vir = output.at("virials").toTensor().cpu();
        mace_virial[0] = vir[0][0][0].item<double>();
        mace_virial[1] = vir[0][1][1].item<double>();
        mace_virial[2] = vir[0][2][2].item<double>();
        mace_virial[3] = 0.5 * (vir[0][1][0].item<double>() + vir[0][0][1].item<double>());
        mace_virial[4] = 0.5 * (vir[0][2][0].item<double>() + vir[0][0][2].item<double>());
        mace_virial[5] = 0.5 * (vir[0][2][1].item<double>() + vir[0][1][2].item<double>());
    }

    return {
        mace_energy,
        mace_node_energy,
        mace_forces,
        mace_virial
    };
}

/*
 * Helper functions
 */

torch::Device get_device() {
    if (torch::cuda::is_available()) {
        // torch device type: CUDA, no MPI
        std::cout << "CUDA found, setting device type to torch::kCUDA." << std::endl;
        return {torch::kCUDA};
    } else {
        // torch device type: CPU
        std::cout << "CUDA unavailable, setting device type to torch::kCPU." << std::endl;
        return {torch::kCPU};
    }
}

torch::jit::Module load_mace_model(const std::string &model_path, c10::Device device) {
    // load MACE model
    std::cout << "Loading MACE model from \"" << model_path << "\" ...";
    auto model = torch::jit::load(model_path, device);
    std::cout << " finished." << std::endl;
    return model;
}

void model_analyse(const torch::jit::Module &model) {
    // extract default dtype from mace model
    auto torch_float_dtype = model_get_dtype(model);
    std::cout << "  - The torch_float_dtype is: " << torch_float_dtype << std::endl;

    // extract r_max from mace model
    auto r_max = model_get_rmax(model);
    std::cout << "  - The r_max is: " << r_max << "." << std::endl;
    auto num_interactions = model_get_num_interactions(model);
    std::cout << "  - The model has: " << num_interactions << " layers." << std::endl;

    // extract atomic numbers from mace model
    auto mace_atomic_numbers = model_get_atomic_numbers(model);
    std::cout << "  - The MACE model atomic numbers are: " << mace_atomic_numbers << "." << std::endl;
}

torch::ScalarType model_get_dtype(const torch::jit::Module &model) {
    // extract default dtype from mace model
    torch::ScalarType torch_float_dtype;
    for (auto p: model.named_attributes()) {
        // this is a somewhat random choice of variable to check. could it be improved?
        if (p.name == "model.node_embedding.linear.weight") {
            if (p.value.toTensor().dtype() == caffe2::TypeMeta::Make<float>()) {
                return torch::kFloat32;
            }
            if (p.value.toTensor().dtype() == caffe2::TypeMeta::Make<double>()) {
                return torch::kFloat64;
            }
        }
    }
    throw std::logic_error("Could not find appropriate device for torch.");
}

double model_get_rmax(const torch::jit::Module &model) {
    return model.attr("r_max").toTensor().item<double>();
}

int64_t model_get_num_interactions(const torch::jit::Module &model) {
    return model.attr("num_interactions").toTensor().item<int64_t>();
}

std::vector<int64_t> model_get_atomic_numbers(const torch::jit::Module &model) {
    std::vector<int64_t> mace_atomic_numbers;
    auto a_n = model.attr("atomic_numbers").toTensor();
    mace_atomic_numbers.reserve(a_n.size(0));
    for (int i = 0; i < a_n.size(0); ++i) {
        mace_atomic_numbers.push_back(a_n[i].item<int64_t>());
    }
    return mace_atomic_numbers;
}
