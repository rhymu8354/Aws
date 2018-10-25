/**
 * @file ConfigTests.cpp
 *
 * This module contains the unit tests of the
 * Aws::Config class.
 *
 * © 2018 by Richard Walters
 */

#include <gtest/gtest.h>
#include <Aws/Config.hpp>

/**
 * This is the test fixture for these tests, providing common
 * setup and teardown for each test.
 */
struct ConfigTests
    : public ::testing::Test
{
    // Properties

    // Methods

    // ::testing::Test

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};

TEST_F(ConfigTests, FromString) {
    const auto config = Aws::Config::FromString(
        "[default]\r\n"
        "region = us-west-1\r\n"
        "output = json\r\n"
        "\r\n"
        "[another section]\r\n"
        "foo =\r\n"
        "  x =42\r\n"
        "  y= 18 \r\n"
    );
    EXPECT_EQ(
        (Json::Object({
            {"default", Json::Object({
                {"region", "us-west-1"},
                {"output", "json"},
            })},
            {"another section", Json::Object({
                {"foo", Json::Object({
                    {"x", "42"},
                    {"y", "18"},
                })},
            })},
        })),
        config
    );
}
