//
// Created by Tamas K Stenczel on 29/06/2024.
//
// Arrays: simple array utilities for the neighbour list & MACE codes, without
// non-standard library dependencies.
//

#ifndef MACE_ARRAYS_H
#define MACE_ARRAYS_H


#include <array>
#include <iostream>

// Type definitions for arrays
template<typename T, std::size_t num>
using array1d = std::array<T, num>;

template<typename T, std::size_t Row, std::size_t Col>
using array2d = std::array<std::array<T, Col>, Row>;

template<std::size_t num>
using bool1d = array1d<bool, num>;

template<std::size_t num>
using int1d = array1d<int, num>;

template<std::size_t num>
using real1d = array1d<double, num>;

template<std::size_t Row, std::size_t Col>
using real2d = array2d<double, Row, Col>;

// Fetch the number of rows from the Row non-type template parameter
template<typename T, std::size_t num>
constexpr int length(const array1d<T, num> &) // you can return std::size_t if you prefer
{
    return num;
}

// Fetch the number of rows from the Row non-type template parameter
template<typename T, std::size_t Row, std::size_t Col>
constexpr int rowLength(const array2d<T, Row, Col> &) { return Row; }


// Fetch the number of cols from the Col non-type template parameter
template<typename T, std::size_t Row, std::size_t Col>
constexpr int colLength(const array2d<T, Row, Col> &) { return Col; }

template<typename T, std::size_t num>
void printArray(const array1d<T, num> &arr) {
    // get each element of the array
    std::cout << '[';
    for (const auto &e: arr) std::cout << e << ", ";
    std::cout << "]\n";
}

template<typename T, std::size_t Row, std::size_t Col>
void printArray(const array2d<T, Row, Col> &arr) {
    for (const auto &arow: arr) printArray(arow);
}

#endif //MACE_ARRAYS_H
