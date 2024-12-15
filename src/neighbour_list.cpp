//
// Created by Tamas K Stenczel on 29/06/2024.
//

#include "arrays.h"
#include "neighbour_list.h"

#include <cassert>
#include <cmath>
#include <climits>

void fail(const std::string &msg) {
    // use this instead of `goto fail;`
    // todo: more granular error handling could make sense
    throw std::logic_error(msg);
}


// library functions: mainly from matscipy's tools.h/tools.c
real1d<3> cross_product(const real1d<3> &a, const real1d<3> &b) {
    real1d<3> c;
    c[0] = a[1] * b[2] - a[2] * b[1];
    c[1] = a[2] * b[0] - a[0] * b[2];
    c[2] = a[0] * b[1] - a[1] * b[0];
    return c;
}

double
normsq(const real1d<3> &a) {
    return sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
}


template<std::size_t len>
double
dot(real1d<len> a, real1d<len> b) {
    double result = 0.;
    for (int i = 0; i < len; i++) {
        result += a[i] * b[i];
    }
    return result;
}

real1d<3> mat_mul_vec(const real2d<3, 3> &mat, const real1d<3> &vec) {
    // functional version of mat_mul_vec from tools.c
    real1d<3> result;
    for (int i = 0; i < 3; i++) {
        result[i] = dot(mat[i], vec);
    }
    return result;
}

bool string_contains(const std::string &s, const char letter) {
    return s.find(letter) != std::string::npos;
}


/*
 * Some cell index algebra
 */

/* Map i back to the interval [0,n) by shifting by integer multiples of n */
int
bin_wrap(int i, const int n) {
    while (i < 0) i += n;
    while (i >= n) i -= n;
    return i;
}

/* Map i back to the interval [0,n) by assigning edge value if outside
   interval */
int
bin_trunc(int i, const int n) {
    if (i < 0) i = 0;
    else if (i >= n) i = n - 1;
    return i;
}

/* Map particle position to a cell index */
void
position_to_cell_index(
    const real1d<3> &cell_origin,
    const real2d<3, 3> &inv_cell,
    const real1d<3> &ri,
    const int n1,
    const int n2,
    const int n3,
    int *c1,
    int *c2,
    int *c3
) {
    real1d<3> dri;
    for (int i = 0; i < 3; i++) {
        dri[i] = ri[i] - cell_origin[i];
    }
    const real1d<3> si = mat_mul_vec(inv_cell, dri);
    *c1 = floor(si[0] * n1);
    *c2 = floor(si[1] * n2);
    *c3 = floor(si[2] * n3);
}

// Computes inverse(A.T) on a 3x3 matrix (analytically)
real2d<3, 3>
inverse_transpose_3x3(const real2d<3, 3> &matrix) {
    // based on from: https://stackoverflow.com/a/18504573
    //
    // Given matric `M`, we compute `1/det(M)` and fill in the new matrix element-wise,
    // swapping the indices, since `inv(A.T) == inv(A).T`.

    // 1/det(M)
    const double invdet = 1 / (matrix[0][0] * (matrix[1][1] * matrix[2][2] - matrix[2][1] * matrix[1][2]) -
                               matrix[0][1] * (matrix[1][0] * matrix[2][2] - matrix[1][2] * matrix[2][0]) +
                               matrix[0][2] * (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]));

    real2d<3, 3> minv; // inverse of matrix
    // n.b. transposing here by swapping the indices on the l.h.s. here
    minv[0][0] = (matrix[1][1] * matrix[2][2] - matrix[2][1] * matrix[1][2]) * invdet;
    minv[1][0] = (matrix[0][2] * matrix[2][1] - matrix[0][1] * matrix[2][2]) * invdet;
    minv[2][0] = (matrix[0][1] * matrix[1][2] - matrix[0][2] * matrix[1][1]) * invdet;
    minv[0][1] = (matrix[1][2] * matrix[2][0] - matrix[1][0] * matrix[2][2]) * invdet;
    minv[1][1] = (matrix[0][0] * matrix[2][2] - matrix[0][2] * matrix[2][0]) * invdet;
    minv[2][1] = (matrix[1][0] * matrix[0][2] - matrix[0][0] * matrix[1][2]) * invdet;
    minv[0][2] = (matrix[1][0] * matrix[2][1] - matrix[2][0] * matrix[1][1]) * invdet;
    minv[1][2] = (matrix[2][0] * matrix[0][1] - matrix[0][0] * matrix[2][1]) * invdet;
    minv[2][2] = (matrix[0][0] * matrix[1][1] - matrix[1][0] * matrix[0][1]) * invdet;

    return minv;
}


neighbour_list
compute_neighbour_list(
    const std::string &quantities,
    // known size
    const real1d<3> &cell_origin,
    const real2d<3, 3> &cell,
    const bool1d<3> &pbc,
    // variable size
    const std::vector<std::array<double, 3> > &positions,
    // todo: add support for singe cutoff & 2D cutoff array
    const std::vector<double> &cutoffs,
    const std::vector<int> &types
) {
    // // DEBUG: print the inputs
    // std::cout << "C++ style" << std::endl;
    // std::cout << ".. cell_origin" << std::endl;
    // printArray(cell_origin);
    // std::cout << ".. cell" << std::endl;
    // printArray(cell);
    // std::cout << ".. pbc" << std::endl;
    // printArray(pbc);

    // std::cout << ".. positions" << std::endl;
    // printVector(positions);
    // std::cout << ".. cutoffs" << std::endl;
    // printVector(cutoffs);
    // std::cout << ".. types" << std::endl;
    // printVector(types);
    // todo: allow for types to be optional -- only needed for 2D cutoff array
    constexpr bool have_types = true;

    // DEBUG end

    // invert the cell
    const real2d<3, 3> inv_cell = inverse_transpose_3x3(cell);
    // std::cout << ".. inv_cell" << std::endl;
    // printArray(inv_cell);

    /* Neighbour list */
    // n.b. initialised further down

    /* Optional quantities to be computed */
    std::vector<int> first; // i - [nNeigh,]
    std::vector<int> secnd; // j - [nNeigh,]
    std::vector<real1d<3> > distvec; // D  - [nNeigh, 3]
    std::vector<double> absdist; // d  - [nNeigh,]
    std::vector<int1d<3> > shift; // S  - [nNeigh, 3]


    /* Make sure our arrays are contiguous */
    // we don't need this

    /* Check array shapes. */
    const auto nat = positions.size();
    if (nat != types.size()) {
        fail("Length mis-match between positions & types!");
    }

    /* handle cutoffs */
    constexpr int ncutoffdims = 1; // todo: add support for 0 & 2 as well
    const int ncutoffs = nat;
    std::vector<double> cutoffs_sq;
    double cutoff = 0.0;
    for (const auto element: cutoffs) {
        cutoff = std::max(cutoff, 2 * element); // global cutoff
        cutoffs_sq.push_back(element * element); // square of each
    }
    // std::cout << ".. cutoff_sq" << std::endl;
    // printVector(cutoffs_sq);

    /* Get pointers to array data */
    // n.b. most of this is handled by the function signature
    const real1d<3> cell1 = cell[0];
    const real1d<3> cell2 = cell[1];
    const real1d<3> cell3 = cell[2];

    /* Compute vectors to opposite face */
    auto norm1 = cross_product(cell2, cell3);
    auto norm2 = cross_product(cell3, cell1);
    auto norm3 = cross_product(cell1, cell2);
    const double volume = fabs(cell3[0] * norm3[0] + cell3[1] * norm3[1] + cell3[2] * norm3[2]);
    if (volume < 1e-12) {
        fail("Zero cell volume.");
    }
    double len1 = normsq(norm1), len2 = normsq(norm2), len3 = normsq(norm3);
    for (int i = 0; i < 3; i++) {
        norm1[i] *= volume / (len1 * len1);
        norm2[i] *= volume / (len2 * len2);
        norm3[i] *= volume / (len3 * len3);
    }
    /* Compute distance of cell faces */
    len1 = volume / len1;
    len2 = volume / len2;
    len3 = volume / len3;

    /* Number of cells for cell subdivision */
    int n1 = std::max(static_cast<int>(floor(len1 / cutoff)), 1);
    int n2 = std::max(static_cast<int>(floor(len2 / cutoff)), 1);
    int n3 = std::max(static_cast<int>(floor(len3 / cutoff)), 1);

    /* Avoid overflow in total number of cells */
    bool warned = false;
    while (static_cast<double>(n1) * n2 * n3 > INT_MAX) {
        if (!warned) {
            std::cout << "Ratio of simulation cell size to cutoff is very "
                    "large; reducing number of bins for neighbour list "
                    "search, but this may be slow. Are you using a cell with "
                    "lots of vacuum?" << std::endl;
            // PyErr_WarnEx(NULL, "Ratio of simulation cell size to cutoff is very "
            //              "large; reducing number of bins for neighbour list "
            //              "search, but this may be slow. Are you using a cell with "
            //              "lots of vacuum?", 1);
            warned = true;
        }
        n1 /= 2;
        if (n1 <= 0) n1 = 1;
        n2 /= 2;
        if (n2 <= 0) n2 = 1;
        n3 /= 2;
        if (n3 <= 0) n3 = 1;
    }
    assert(n1 > 0);
    assert(n2 > 0);
    assert(n3 > 0);

    /* Find out over how many neighbor cells we need to loop (if the box is
       small */
    int nx = static_cast<int>(ceil(cutoff * n1 / len1));
    int ny = static_cast<int>(ceil(cutoff * n2 / len2));
    int nz = static_cast<int>(ceil(cutoff * n3 / len3));

    /* Sort particles into bins */
    // in C these were mallocs, and checks if they succeeded, we don't need to
    // do that here
    const int ncells = n1 * n2 * n3;
    int seed[ncells];
    int last[ncells];
    for (int i = 0; i < ncells; i++) seed[i] = -1;
    int next[nat];
    // todo: this is mostly just a copy from C, did not check for making more C++-like
    for (int i = 0; i < nat; i++) {
        /* Get cell index */
        int c1, c2, c3;
        position_to_cell_index(cell_origin, inv_cell, positions[i], n1, n2, n3,
                               &c1, &c2, &c3);

        /* Periodic/non-periodic boundary conditions */
        if (pbc[0]) c1 = bin_wrap(c1, n1);
        else c1 = bin_trunc(c1, n1);
        if (pbc[1]) c2 = bin_wrap(c2, n2);
        else c2 = bin_trunc(c2, n2);
        if (pbc[2]) c3 = bin_wrap(c3, n3);
        else c3 = bin_trunc(c3, n3);

        /* Continuous cell index */
        int ci = c1 + n1 * (c2 + n2 * c3);

        assert(c1 >= 0 && c1 < n1);
        assert(c2 >= 0 && c2 < n2);
        assert(c3 >= 0 && c3 < n3);
        assert(ci >= 0 && ci < ncells);

        /* Put atom into appropriate bin */
        if (seed[ci] < 0) {
            next[i] = -1;
            seed[ci] = i;
            last[ci] = i;
        } else {
            next[i] = -1;
            next[last[ci]] = i;
            last[ci] = i;
        }
    }

    /* Neighbour list counter and size */
    // int nneigh = 0; /* Number of neighbours found */
    // int neighsize = nat; /* Initial guess for neighbour list size */

    // FIXME: I don't think this is needed here, we have replacements for py_... versions above
    // npy_int *first = NULL, *secnd = NULL, *shift = NULL;
    // npy_double *distvec = NULL, *absdist = NULL;

    // we are unpacking the quantities to be calculated
    const bool do_first = string_contains(quantities, 'i');
    const bool do_secnd = string_contains(quantities, 'j');
    const bool do_distvec = string_contains(quantities, 'D');
    const bool do_absdist = string_contains(quantities, 'd');
    const bool do_shift = string_contains(quantities, 'S');
    if (quantities.length() > do_first + do_secnd + do_distvec + do_absdist + do_shift) {
        fail("Unsupported quantity specified.");
    }

    /* We need the square of the cutoff */
    double cutoff_sq = cutoff * cutoff;

    /* We need the shape of the bin */
    double bin1[3], bin2[3], bin3[3];
    for (int i = 0; i < 3; i++) {
        bin1[i] = cell1[i] / n1;
        bin2[i] = cell2[i] / n2;
        bin3[i] = cell3[i] / n3;
    }

    // /* Loop over atoms */
    for (int i = 0; i < nat; i++) {
        auto ri = positions[i];

        int ci01, ci02, ci03;
        position_to_cell_index(cell_origin, inv_cell, ri, n1, n2, n3,
                               &ci01, &ci02, &ci03);

        /* Truncate if non-periodic and outside of simulation domain */
        int ci1, ci2, ci3;
        if (!pbc[0]) ci1 = bin_trunc(ci01, n1);
        else ci1 = ci01;
        if (!pbc[1]) ci2 = bin_trunc(ci02, n2);
        else ci2 = ci02;
        if (!pbc[2]) ci3 = bin_trunc(ci03, n3);
        else ci3 = ci03;

        /* dri is the position relative to the lower left corner of the bin */
        double dri[3];
        dri[0] = ri[0] - ci1 * bin1[0] - ci2 * bin2[0] - ci3 * bin3[0];
        dri[1] = ri[1] - ci1 * bin1[1] - ci2 * bin2[1] - ci3 * bin3[1];
        dri[2] = ri[2] - ci1 * bin1[2] - ci2 * bin2[2] - ci3 * bin3[2];

        /* Apply periodic boundary conditions */
        if (pbc[0]) ci1 = bin_wrap(ci01, n1);
        else ci1 = bin_trunc(ci01, n1);
        if (pbc[1]) ci2 = bin_wrap(ci02, n2);
        else ci2 = bin_trunc(ci02, n2);
        if (pbc[2]) ci3 = bin_wrap(ci03, n3);
        else ci3 = bin_trunc(ci03, n3);

        /* Loop over neighbouring bins */
        int x, y, z;
        for (z = -nz; z <= nz; z++) {
            int cj3 = ci3 + z;
            if (pbc[2]) cj3 = bin_wrap(cj3, n3);

            /* Skip to next z value if cell is out of simulation bounds */
            if (cj3 < 0 || cj3 >= n3) continue;

            cj3 = bin_trunc(cj3, n3);
            int ncj3 = n2 * cj3;

            double off3[3];
            off3[0] = z * bin3[0];
            off3[1] = z * bin3[1];
            off3[2] = z * bin3[2];

            for (y = -ny; y <= ny; y++) {
                int cj2 = ci2 + y;
                if (pbc[1]) cj2 = bin_wrap(cj2, n2);

                /* Skip to next y value if cell is out of simulation bounds */
                if (cj2 < 0 || cj2 >= n2) continue;

                cj2 = bin_trunc(cj2, n2);
                int ncj2 = n1 * (cj2 + ncj3);

                double off2[3];
                off2[0] = off3[0] + y * bin2[0];
                off2[1] = off3[1] + y * bin2[1];
                off2[2] = off3[2] + y * bin2[2];

                for (x = -nx; x <= nx; x++) {
                    /* Bin index of neighbouring bin */
                    int cj1 = ci1 + x;
                    if (pbc[0]) cj1 = bin_wrap(cj1, n1);

                    /* Skip to next x value if cell is out of simulation bounds
                     */
                    if (cj1 < 0 || cj1 >= n1) continue;

                    cj1 = bin_trunc(cj1, n1);
                    int ncj = cj1 + ncj2;

                    assert(ncj == cj1+n1*(cj2+n2*cj3));

                    /* Offset of the neighboring bins */
                    double off[3];
                    off[0] = off2[0] + x * bin1[0];
                    off[1] = off2[1] + x * bin1[1];
                    off[2] = off2[2] + x * bin1[2];

                    /* Loop over all atoms in neighbouring bin */
                    int j = seed[ncj];
                    while (j >= 0) {
                        if (i != j || x != 0 || y != 0 || z != 0) {
                            auto rj = positions[j];

                            int cj1, cj2, cj3;
                            position_to_cell_index(cell_origin, inv_cell, rj,
                                                   n1, n2, n3,
                                                   &cj1, &cj2, &cj3);

                            /* Truncate if non-periodic and outside of
                               simulation domain. */
                            if (!pbc[0]) cj1 = bin_trunc(cj1, n1);
                            if (!pbc[1]) cj2 = bin_trunc(cj2, n2);
                            if (!pbc[2]) cj3 = bin_trunc(cj3, n3);

                            /* drj is position relative to lower
                               left corner of the bin */
                            double drj[3];
                            drj[0] = rj[0] - cj1 * bin1[0] - cj2 * bin2[0] -
                                     cj3 * bin3[0];
                            drj[1] = rj[1] - cj1 * bin1[1] - cj2 * bin2[1] -
                                     cj3 * bin3[1];
                            drj[2] = rj[2] - cj1 * bin1[2] - cj2 * bin2[2] -
                                     cj3 * bin3[2];

                            /* Compute distance between atoms */
                            real1d<3> dr;
                            dr[0] = drj[0] - dri[0] + off[0];
                            dr[1] = drj[1] - dri[1] + off[1];
                            dr[2] = drj[2] - dri[2] + off[2];
                            double abs_dr_sq = dr[0] * dr[0] + dr[1] * dr[1] +
                                               dr[2] * dr[2];

                            if (abs_dr_sq < cutoff_sq) {
                                bool inside_cutoff = true;
                                if (ncutoffdims == 1) {
                                    double c_sq = cutoffs[i] + cutoffs[j];
                                    c_sq *= c_sq;
                                    inside_cutoff = abs_dr_sq < c_sq;
                                } else if (ncutoffdims == 2 && have_types) {
                                    // todo: not supported yet
                                    if (types[i] < ncutoffs &&
                                        types[j] < ncutoffs) {
                                        double c_sq =
                                                cutoffs_sq[types[i] * ncutoffs +
                                                           types[j]];
                                        inside_cutoff = abs_dr_sq < c_sq;
                                    }
                                }

                                if (inside_cutoff) {
                                    // n.b. C is re-sizing the arrays here, we are using vectors so no need for that
                                    // the C code:
                                    // if (nneigh >= neighsize) {
                                    //     neighsize *= 2;
                                    //     if (py_first && !(first = resize_array(py_first, neighsize))) goto fail;
                                    //     if (py_secnd && !(secnd = resize_array(py_secnd, neighsize))) goto fail;
                                    //     if (py_distvec && !(distvec = resize_array( py_distvec, neighsize))) goto fail;
                                    //     if (py_absdist && !(absdist = resize_array( py_absdist, neighsize))) goto fail;
                                    //     if (py_shift && !(shift = resize_array(py_shift, neighsize))) goto fail;
                                    // }

                                    if (do_first) first.push_back(i);
                                    if (do_secnd) secnd.push_back(j);
                                    if (do_distvec) distvec.push_back(dr);
                                    if (do_absdist) absdist.push_back(sqrt(abs_dr_sq));
                                    if (do_shift) {
                                        shift.push_back({
                                            (ci01 - cj1 + x) / n1, (ci02 - cj2 + y) / n2, (ci03 - cj3 + z) / n3
                                        });
                                    }
                                    // nneigh++;
                                }
                            }
                        }
                        j = next[j];
                    }
                }
            }
        }
    }

    /* Release cell subdivision information */
    // no need

    /* Resize arrays to actual size of neighbour list */
    // no need

    /* Build return tuple */
    return {
        first, secnd, distvec, absdist, shift
    };

    /* Final cleanup */
    // no need
}


MaceNeighbourList
calc_mace_neighbour_list(
    const real2d<3, 3> &cell,
    const bool1d<3> &pbc,
    const std::vector<std::array<double, 3> > &positions,
    const double &cutoff
) {
    // preparations for neighbour list: cell is extended in non-periodic directions
    real2d<3, 3> extended_cell = cell;

    // find the max(abs(position)) .. in case we have non-periodic boundaries
    double max_pos = 1.0;
    for (auto row: positions) {
        for (auto entry: row) {
            max_pos = std::max(max_pos, std::abs(entry) + 1);
        }
    }

    // extend the cell in non-periodic directions
    for (int i_dim = 0; i_dim < 3; i_dim++) {
        if (pbc[i_dim]) {
            continue;
        }
        // set cell vector to (max(abs(pos[:, dim])) * 5 * cutoff) * Identity
        extended_cell[i_dim][i_dim] = max_pos * 5 * cutoff;
        for (int j_dim = 0; j_dim < 3; j_dim++) {
            if (j_dim != i_dim) extended_cell[i_dim][j_dim] = 0.;
        }
    }

    // Compute the neighbour list - matscipy->C++
    constexpr real1d<3> cell_origin = {0., 0., 0.,};
    const auto cutoffs = std::vector<double>(positions.size(), cutoff);
    const auto numbers = std::vector<int>(positions.size(), 1); // dummy
    auto [first, secnd, _0, _1, nl_shifts] = compute_neighbour_list(
        "ijS", cell_origin, extended_cell, pbc, positions, cutoffs, numbers);

    // based on mace.data.neighbourhood.get_neighborhood(...), where
    // true_self_interaction=False is the default, we are eliminate self-edges that
    // don't cross periodic boundaries
    std::vector<int1d<2> > edge_index;
    std::vector<int1d<3> > unit_shifts;

    const auto n_edge_init = first.size();
    for (int idx = 0; idx < n_edge_init; idx++) {
        const auto sender = first[idx];
        const auto receiver = secnd[idx];
        const auto shift = nl_shifts[idx];
        if (sender == receiver && shift[0] == 0 && shift[1] == 0 && shift[2] == 0) {
            // skip this: self-interaction within the same cell
            // std::cout << "dropping" << " " << sender << " " << receiver << " : " << shift[0] << shift[1] << shift[2] <<
            //         std::endl;
            continue;
        }
        // save the edge
        // std::cout << "KEEP" << " " << sender << " " << receiver << " : " << shift[0] << shift[1] << shift[2] <<
        //         std::endl;
        edge_index.push_back({sender, receiver});
        unit_shifts.push_back(shift);
    }

    // n.b. shift is better computed with torch's dot
    return {
        edge_index,
        unit_shifts,
        extended_cell
    };
}
