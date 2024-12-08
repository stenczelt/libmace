//
// Created by Tamas K Stenczel on 2024/07/07.
//

#ifndef MACE_H
#define MACE_H

#include <string>
#include <vector>

#include "arrays.h"
#include <torch/script.h>

struct CalculationResult {
    double total_energy;
    std::vector<double> node_energy;
    std::vector<real1d<3> > forces;
    real1d<6> virial;
};

class MACE {
public:
    explicit MACE(const std::string &model_path);

    ~MACE() = default;

    void reload();

    void print() const;

    CalculationResult
    calculate(
        const real2d<3, 3> &cell,
        const bool1d<3> &pbc,
        const std::vector<std::array<double, 3> > &positions,
        const std::vector<int> &atomic_numbers,
        const bool &calc_virial
    );

private:
    std::string model_path;
    torch::Device device = torch::kCPU;
    torch::jit::Module model;

    torch::ScalarType torch_float_dtype;
    std::vector<int64_t> mace_atomic_numbers;
    std::unordered_map<int64_t, std::size_t> z_to_mace_type; // atomic_number -> MACE element index
    double r_max;
};

// =============================================================================
// C interface for CXX code
#ifdef __cplusplus
// for a C++ compiler
extern "C" {
class MACE;
typedef MACE CMace; // notice the renaming
#else // __cplusplus
// for a C compiler: opaque pointer
    typedef struct CMace CMace;
#endif // __cplusplus

// Constructor
CMace *cmace_init(const char *model_path);

// Destructor
void cmace_finalise(CMace *self);

// class member functions
void cmace_reload(CMace *self);

void cmace_print(CMace *self);

void cmace_calculate(
    CMace *self,
    // inputs
    int calc_virial, // int 0 or 1 -> bool
    int n_atoms,
    const double *cell, // real(3,3)
    const int *pbc, // int(3) 0 or 1 -> bool(3)
    const int *atomic_numbers, // int(n_atoms)
    const double *positions, // real(n_atoms, 3)
    // outputs
    double *total_energy,
    double *node_energy, // real(n_atoms)
    double *forces, // real(n_atoms, 3)
    double *virial // real(6)
);


// void mace_calculate(CMace * cls); // todo: write this as a subroutine

// =============================================================================
#ifdef __cplusplus
}
#endif // __cplusplus

#endif //MACE_H
