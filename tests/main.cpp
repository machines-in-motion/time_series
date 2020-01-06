/**
 * @file main.cpp
 * @author Maximilien Naveau (maximilien.naveau@gmail.com)
 * @author Vincent Berenz (vberenz@tue.mpg.de)
 * license License BSD-3-Clause
 * @copyright Copyright (c) 2019, New York University and Max Planck
 * Gesellschaft.
 * @date 2019-05-22
 *
 * @brief Main file that runs all unittest using gtest
 */

#include <gtest/gtest.h>

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
