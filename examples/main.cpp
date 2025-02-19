//
// Example program: load & execute MACE model.
//
//
// usage: $executable $mace_model_path
//

#include <iostream>
#include "mace.h"

// array helpers: these are private in the library at the moment, but very useful
#include "../src/arrays.h"


int main(int argc, const char *argv[]) {
    // model path as 1st arg
    const std::string model_path = argv[1];
    std::cout << "Hello! Loading & running MACE model from `" << model_path << "`" << std::endl;

    // MACE model from the library
    auto calculator = MACE(model_path);
    calculator.print();

    // Atoms: SiC cell
    constexpr real2d<3, 3> cell = {
        {
            {2.15, 2.15, 0.0},
            {0.0, 2.15, 2.20},
            {2.01, 0.0, 2.15},
        }
    };
    constexpr bool1d<3> pbc = {{true, true, true}};
    const std::vector<std::array<double, 3> > positions = {
        {
            {1.077, 1.2, 1.0},
            {0.0, 0.0, 0.01},
        }
    };
    const std::vector elements = {6, 14};

    // Computation
    const auto [total_energy, node_energy, forces, virial] = calculator.calculate(cell, pbc, positions, elements, true);
    std::cout << "total_energy:" << total_energy << std::endl;
    std::cout << "node_energy:" << std::endl;
    printVector(node_energy);
    std::cout << "forces:" << std::endl;
    printVector(forces);
    std::cout << "virial:" << std::endl;
    printArray(virial);

    for (int i = 0; i < 6; i++) {
        std::cout << "virial:" << i << " : " << std::fixed << std::setprecision(12) << virial[i] << std::endl;
    }

    return 0;
}
