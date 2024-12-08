//
// Created by Tamas K Stenczel on 29/06/2024.
//

// matscipy's neighbour list re-implemented without reference to Python

#ifndef NEIGHBOURS_H
#define NEIGHBOURS_H

#include <vector>
#include "arrays.h"

struct MaceNeighbourList {
    std::vector<int1d<2> > edge_index;
    std::vector<int1d<3> > unit_shifts;
    real2d<3, 3> extended_cell;
};

MaceNeighbourList
calc_mace_neighbour_list(
    const real2d<3, 3> &cell,
    const bool1d<3> &pbc,
    const std::vector<std::array<double, 3> > &positions,
    const double &cutoff
);


struct neighbour_list {
    std::vector<int> first; // i - [nNeigh,]
    std::vector<int> secnd; // j - [nNeigh,]
    std::vector<real1d<3> > distvec; // D  - [nNeigh, 3]
    std::vector<double> absdist; // d  - [nNeigh,]
    std::vector<int1d<3> > shift; // S  - [nNeigh, 3]
};


neighbour_list compute_neighbour_list(
    const std::string &quantities,
    // known size
    const real1d<3> &cell_origin,
    const real2d<3, 3> &cell,
    const bool1d<3> &pbc,
    // variable size
    const std::vector<std::array<double, 3> > &positions,
    const std::vector<double> &cutoffs,
    const std::vector<int> &types
);



#endif //NEIGHBOURS_H
