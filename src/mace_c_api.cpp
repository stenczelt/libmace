//
// Created by Tamas K Stenczel on 07/07/2024.
//


#include "mace.h"
#include "arrays.h"


CMace *cmace_init(const char *model_path) {
    return new MACE(std::string(model_path));
}

void cmace_finalise(CMace *self) {
    delete self;
}

void cmace_reload(CMace *self) {
    self->reload();
}

void cmace_print(CMace *self) {
    self->print();
}

bool int_to_bool(const int value) {
    if (value == 0) {
        return false;
    }
    if (value == 1) {
        return true;
    }
    throw std::logic_error("Integer cannot be interpreted as boolean.");
}


void cmace_calculate(
    CMace *self,
    const int calc_virial,
    const int n_atoms,
    const double *cell,
    const int *pbc,
    const int *atomic_numbers,
    const double *positions,
    double *total_energy,
    double *node_energy,
    double *forces,
    double *virial
) {
    // conversions

    // CELL: known size
    real2d<3, 3> mace_cell;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            mace_cell[i][j] = cell[3 * i + j];
        }
    }

    // PBC: konwn size
    bool1d<3> mace_pbc;
    for (int i = 0; i < 3; i++) {
        mace_pbc[i] = int_to_bool(pbc[i]);
    }

    // Positions
    std::vector<std::array<double, 3> > mace_positions(n_atoms);
    for (int i = 0; i < n_atoms; i++) {
        for (int j = 0; j < 3; j++) {
            // note order of things in memory .. after all it's the same just indexing
            // it from the other way in C :)
            mace_positions[i][j] = positions[3 * i + j];
        }
    }

    // atomic numbers
    std::vector<int> mace_atomic_numbers(n_atoms);
    for (int i = 0; i < n_atoms; i++) {
        mace_atomic_numbers[i] = atomic_numbers[i];
    }

    // actual call to MACE
    auto [mace_total_energy, mace_node_energy, mace_forces, mace_virial] = self->calculate(
        mace_cell,
        mace_pbc,
        mace_positions,
        mace_atomic_numbers,
        int_to_bool(calc_virial));

    // unpack arrays to return
    total_energy[0] = mace_total_energy;
    for (int i = 0; i < n_atoms; i++) {
        node_energy[i] = mace_node_energy[i];
    }
    for (int i = 0; i < n_atoms; i++) {
        for (int j = 0; j < 3; j++) {
            forces[3 * i + j] = mace_forces[i][j];
        }
    }
    for (int i = 0; i < 6; i++) {
        virial[i] = mace_virial[i];
    }
}
