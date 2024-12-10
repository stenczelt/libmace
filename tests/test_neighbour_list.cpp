//
// Created by Tamas K Stenczel on 09/12/2024.
//
// Tests for neighbour_list.cpp

#include <gtest/gtest.h>
#include "arrays.h"
#include "neighbour_list.h"

TEST(TestNormSquare, test1) {
    // test for normsq function
    constexpr real1d<3> inp = {0., 0., 0.};
    EXPECT_EQ(normsq(inp), 0.);
}