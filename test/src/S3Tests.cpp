/**
 * @file S3Tests.cpp
 *
 * This module contains the unit tests of the
 * Aws::S3 class.
 *
 * © 2019 by Richard Walters
 */

#include <gtest/gtest.h>
#include <Aws/S3.hpp>

/**
 * This is the test fixture for these tests, providing common
 * setup and teardown for each test.
 */
struct S3Tests
    : public ::testing::Test
{
    // ::testing::Test

    virtual void SetUp() override {
    }

    virtual void TearDown() override {
    }
};

TEST_F(S3Tests, Placeholder) {
}
