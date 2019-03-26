/**
 * @file ConfigTests.cpp
 *
 * This module contains the unit tests of the
 * Aws::Config structure.
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

    /**
     * This is used to provide environment variables for the Aws::Config class,
     * through a custom shim that replaces the getenv function from the C
     * library.
     */
    Json::Value testEnvironment = Json::Object({});

    // Methods

    // ::testing::Test

    virtual void SetUp() {
        Aws::Config::SetEnvironmentShim(
            [this](const std::string& name){
                return (std::string)testEnvironment[name];
            }
        );
        testAreaPath = SystemAbstractions::File::GetExeParentDirectory() + "/TestArea";
        ASSERT_TRUE(SystemAbstractions::File::CreateDirectory(testAreaPath));
    }

    virtual void TearDown() {
        ASSERT_TRUE(SystemAbstractions::File::DeleteDirectory(testAreaPath));
        Aws::Config::SetEnvironmentShim(nullptr);
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

TEST_F(ConfigTests, GetDefaults) {
    ASSERT_TRUE(SystemAbstractions::File::CreateDirectory(testAreaPath + "/.aws"));
    const auto configPath = testAreaPath + "/.aws/config";
    auto configFile = fopen(configPath.c_str(), "wt");
    static const char configContent[] = (
        "[default]\r\n"
        "region=us-west-2\r\n"
        "s3=\r\n"
        "  max_concurrent_requests=10\r\n"
        "  max_queue_size=1000\r\n"
        "\r\n"
        "[profile xyz]\r\n"
        "aws_access_key_id=xyz1\r\n"
        "aws_secret_access_key=xyz2\r\n"
        "region=us-west-1\r\n"
        "\r\n"
        "[profile zyx]\r\n"
        "region=us-west-3\r\n"
    );
    (void)fwrite(configContent, sizeof(configContent) - 1, 1, configFile);
    (void)fclose(configFile);
    const auto sharedCredentialsPath = testAreaPath + "/.aws/credentials";
    auto sharedCredentialsFile = fopen(sharedCredentialsPath.c_str(), "wt");
    static const char sharedCredentialsContent[] = (
        "[default]\r\n"
        "aws_access_key_id=foo\r\n"
        "aws_secret_access_key=bar\r\n"
        "\r\n"
        "[zyx]\r\n"
        "aws_access_key_id=foo2\r\n"
        "aws_secret_access_key=bar2\r\n"
        "aws_session_token=PogChamp\r\n"
    );
    (void)fwrite(sharedCredentialsContent, sizeof(sharedCredentialsContent) - 1, 1, sharedCredentialsFile);
    (void)fclose(sharedCredentialsFile);
    auto defaults = Aws::Config::GetDefaults(
        Json::Object({
            {"home", testAreaPath},
        })
    );
    EXPECT_EQ("foo", defaults.accessKeyId);
    EXPECT_EQ("bar", defaults.secretAccessKey);
    EXPECT_EQ("", defaults.sessionToken);
    EXPECT_EQ("us-west-2", defaults.region);
    defaults = Aws::Config::GetDefaults(
        Json::Object({
            {"home", testAreaPath},
            {"profile", "xyz"},
        })
    );
    EXPECT_EQ("xyz1", defaults.accessKeyId);
    EXPECT_EQ("xyz2", defaults.secretAccessKey);
    EXPECT_EQ("", defaults.sessionToken);
    EXPECT_EQ("us-west-1", defaults.region);
    testEnvironment["AWS_PROFILE"] = "zyx";
    defaults = Aws::Config::GetDefaults(
        Json::Object({
            {"home", testAreaPath},
        })
    );
    EXPECT_EQ("foo2", defaults.accessKeyId);
    EXPECT_EQ("bar2", defaults.secretAccessKey);
    EXPECT_EQ("PogChamp", defaults.sessionToken);
    EXPECT_EQ("us-west-3", defaults.region);
}
