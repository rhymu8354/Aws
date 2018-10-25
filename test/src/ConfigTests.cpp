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
#include <SystemAbstractions/File.hpp>

/**
 * This is the test fixture for these tests, providing common
 * setup and teardown for each test.
 */
struct ConfigTests
    : public ::testing::Test
{
    // Properties

    /**
     * This is the temporary directory to use for files made during tests.
     */
    std::string testAreaPath;

    // Methods

    // ::testing::Test

    virtual void SetUp() {
        testAreaPath = SystemAbstractions::File::GetExeParentDirectory() + "/TestArea";
        ASSERT_TRUE(SystemAbstractions::File::CreateDirectory(testAreaPath));
    }

    virtual void TearDown() {
        ASSERT_TRUE(SystemAbstractions::File::DeleteDirectory(testAreaPath));
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

TEST_F(ConfigTests, FromFile) {
    const auto configPath = testAreaPath + "/config";
    auto configFile = fopen(configPath.c_str(), "wt");
    static const char content[] = (
        "[default]\r\n"
        "aws_access_key_id=foo\r\n"
        "aws_secret_access_key=bar\r\n"
        "region=us-west-2\r\n"
    );
    (void)fwrite(content, sizeof(content) - 1, 1, configFile);
    (void)fclose(configFile);
    const auto config = Aws::Config::FromFile(configPath);
    EXPECT_EQ(
        (Json::Object({
            {"default", Json::Object({
                {"aws_access_key_id", "foo"},
                {"aws_secret_access_key", "bar"},
                {"region", "us-west-2"},
            })},
        })),
        config
    );
}
