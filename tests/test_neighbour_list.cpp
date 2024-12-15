//
// Created by Tamas K Stenczel on 09/12/2024.
//
// Tests for neighbour_list.cpp

#include <gtest/gtest.h>
#include "arrays.h"
#include "neighbour_list.h"

TEST(TestNormSquare, zeros) {
    // test for normsq function
    constexpr real1d<3> inp = {0., 0., 0.};
    EXPECT_EQ(normsq(inp), 0.);
}
TEST(TestNormSquare, ones) {
    // test for normsq function
    constexpr real1d<3> inp = {1., 0., 0.};
    EXPECT_EQ(normsq(inp), 1.);
}

TEST(TestStringContains, containsItself) {
    EXPECT_TRUE(string_contains("a", 'a'));
}

TEST(TestStringContains, ijS) {
    // contains all three letters
    EXPECT_TRUE(string_contains("ijS", 'i'));
    EXPECT_TRUE(string_contains("ijS", 'j'));
    EXPECT_TRUE(string_contains("ijS", 'S'));

    // case-sensitive
    EXPECT_FALSE(string_contains("ijS", 's'));

    // does not contain other letters
    EXPECT_FALSE(string_contains("ijS", 'a'));
    EXPECT_FALSE(string_contains("ijS", 'G'));
    EXPECT_FALSE(string_contains("ijS", 'P'));
    EXPECT_FALSE(string_contains("ijS", '['));
}

TEST(Test3x3Inverse, identity) {
    constexpr real2d<3, 3> identity = {
        {
            {1.0, 0.0, 0.0},
            {0.0, 1.0, 0.0},
            {0.0, 0.0, 1.0},
        }
    };
    auto identity_inv = inverse_transpose_3x3(identity);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            EXPECT_DOUBLE_EQ(identity[i][j], identity_inv[i][j]);
        }
    }
}

TEST(Test3x3Inverse, randomMatrix) {
    // generated a random 3x3 matrix with Numpy and inverted it as a test case
    constexpr real2d<3, 3> matrix_in = {
        {
            {0.9557160208486466, 0.8163179414231801, 0.3557891706289027},
            {0.4050454864556330, 0.5684393882941561, 0.7487249503012058},
            {0.6344967407121644, 0.8237003481499361, 0.8980769937862583},
        }
    };
    constexpr real2d<3, 3> inverse_expected = {
        {
            {5.2376240311873046, 21.6981054512882920, -20.1646439682412293},
            {-5.4880514511903646, -31.1902251752779165, 28.1774160161303833},
            {1.3331312598250995, 13.2772827160495783, -10.4838967840465145},
        }
    };
    auto computed_inv = inverse_transpose_3x3(matrix_in);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            // n.b. comparing to transpose of inverse
            EXPECT_NEAR(computed_inv[i][j], inverse_expected[j][i], 1e-12);
        }
    }
}
