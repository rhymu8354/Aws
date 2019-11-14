/**
 * @file SignApiTests.cpp
 *
 * This module contains the unit tests of the
 * Aws::SignApi class.
 *
 * © 2018 by Richard Walters
 */

#include <gtest/gtest.h>
#include <Aws/SignApi.hpp>
#include <set>
#include <sstream>
#include <SystemAbstractions/File.hpp>

#define STRINGIFY(s) #s
#define STRINGIFY_DEFINE_VALUE(s) STRINGIFY(s)
#define TEST_VECTOR_DIR_STRING STRINGIFY_DEFINE_VALUE(TEST_VECTOR_DIR)

/**
 * This is the test fixture for these tests, providing common
 * setup and teardown for each test.
 */
struct SignApiTests
    : public ::testing::Test
{
    // Properties

    static const std::set< std::string > testVectors;

    // Methods

    static std::string GetFileNameOnly(const std::string& filePath) {
        const auto delimiter = filePath.find_last_of("/\\");
        return filePath.substr(delimiter + 1);
    }

    static std::set< std::string > FindTestVectors(const std::string& path) {
        std::set< std::string > testVectors;
        std::vector< std::string > entries;
        SystemAbstractions::File::ListDirectory(path, entries);
        for (const auto& entry: entries) {
            // Skip entry if it isn't a directory.
            SystemAbstractions::File entryFile(entry);
            if (!entryFile.IsDirectory()) {
                continue;
            }

            // Extract the name portion of the entry.
            const auto entryName = GetFileNameOnly(entry);

            // If there is a file in the directory with the same name
            // and ending in ".req", that directory represents a single test.
            SystemAbstractions::File entryTestFile(entry + "/" + entryName + ".req");
            if (entryTestFile.IsExisting()) {
                (void)testVectors.insert(entryTestFile.GetPath());
                continue;
            }

            // Otherwise, scan subdirectories for test vectors.
            const auto subTestVectors = FindTestVectors(entry);
            testVectors.insert(subTestVectors.begin(), subTestVectors.end());
        }
        return testVectors;
    }

    static char MakeHexDigit(unsigned int value) {
        if (value < 10) {
            return (char)(value + '0');
        } else {
            return (char)(value - 10 + 'A');
        }
    }

    static std::string PercentEncode(const std::string& input) {
        std::string output;
        for (uint8_t c: input) {
            if ((c >= 0x21) && (c <= 0x7e)) {
                output.push_back(c);
            } else {
                output.push_back('%');
                output.push_back(MakeHexDigit(c >> 4));
                output.push_back(MakeHexDigit(c & 0x0F));
            }
        }
        return output;
    }

    static std::vector< std::string > SplitLines(const std::string& s) {
        std::vector< std::string > values;
        auto remainder = s;
        while (!remainder.empty()) {
            auto delimiter = remainder.find_first_of('\n');
            if (delimiter == std::string::npos) {
                values.push_back(remainder);
                remainder.clear();
            } else {
                values.push_back(remainder.substr(0, delimiter));
                remainder = remainder.substr(delimiter + 1);
            }
        }
        return values;
    }

    static std::string CleanUpRequest(const std::string& input) {
        std::ostringstream output;
        bool first = true;
        for (const auto& line: SplitLines(input)) {
            if (first) {
                first = false;
                const auto delimiter1 = line.find(' ');
                const auto delimiter2 = line.find_last_of(' ');
                auto rawUri = line.substr(
                    delimiter1 + 1,
                    delimiter2 - delimiter1 - 1
                );
                if (rawUri.substr(0, 2) == "//") {
                    rawUri = "/" + rawUri.substr(2);
                }
                output
                    << line.substr(0, delimiter1 + 1)
                    << PercentEncode(rawUri)
                    << line.substr(delimiter2)
                    << "\r\n";
            } else {
                output << line << "\r\n";
            }
        }
        output << "\r\n\r\n";
        return output.str();
    }

    // ::testing::Test

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};

const std::set< std::string > SignApiTests::testVectors = SignApiTests::FindTestVectors(TEST_VECTOR_DIR_STRING);

TEST_F(SignApiTests, MakeCanonicalRequest) {
    for (const auto& testVector: testVectors) {
        SystemAbstractions::File testVectorFile(testVector);
        ASSERT_TRUE(testVectorFile.OpenReadOnly());
        SystemAbstractions::File::Buffer testVectorContents(testVectorFile.GetSize());
        ASSERT_EQ(testVectorFile.GetSize(), testVectorFile.Read(testVectorContents));
        SystemAbstractions::File creqFile(testVector.substr(0, testVector.length() - 3) + "creq");
        ASSERT_TRUE(creqFile.OpenReadOnly());
        SystemAbstractions::File::Buffer creqContents(creqFile.GetSize());
        ASSERT_EQ(creqFile.GetSize(), creqFile.Read(creqContents));
        EXPECT_EQ(
            std::string(creqContents.begin(), creqContents.end()),
            Aws::SignApi::ConstructCanonicalRequest(
                CleanUpRequest(
                    std::string(testVectorContents.begin(), testVectorContents.end())
                )
            )
        ) << "******** The name of the test vector that failed was: " << GetFileNameOnly(testVector);
    }
}

TEST_F(SignApiTests, AmzUriEncodeQuery) {
    EXPECT_EQ(
        std::string(
            "GET\n"
            "/\n"
            "arg=foo%2Bbar%3D\n"
            "host:example.amazonaws.com\n"
            "x-amz-date:20150830T123600Z\n"
            "\n"
            "host;x-amz-date\n"
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
        ),
        Aws::SignApi::ConstructCanonicalRequest(
            std::string(
                "GET /?arg=foo+bar= HTTP/1.1\r\n"
                "Host:example.amazonaws.com\r\n"
                "X-Amz-Date:20150830T123600Z\r\n"
                "\r\n"
            )
        )
    );
}

TEST_F(SignApiTests, MakeStringToSign) {
    static const std::string region = "us-east-1";
    static const std::string service = "service";
    for (const auto& testVector: testVectors) {
        SystemAbstractions::File creqFile(testVector.substr(0, testVector.length() - 3) + "creq");
        ASSERT_TRUE(creqFile.OpenReadOnly());
        SystemAbstractions::File::Buffer creqContents(creqFile.GetSize());
        ASSERT_EQ(creqFile.GetSize(), creqFile.Read(creqContents));
        SystemAbstractions::File stsFile(testVector.substr(0, testVector.length() - 3) + "sts");
        ASSERT_TRUE(stsFile.OpenReadOnly());
        SystemAbstractions::File::Buffer stsContents(stsFile.GetSize());
        ASSERT_EQ(stsFile.GetSize(), stsFile.Read(stsContents));
        EXPECT_EQ(
            std::string(stsContents.begin(), stsContents.end()),
            Aws::SignApi::MakeStringToSign(
                region,
                service,
                std::string(creqContents.begin(), creqContents.end())
            )
        ) << "******** The name of the test vector that failed was: " << GetFileNameOnly(testVector);
    }
}

TEST_F(SignApiTests, MakeAuthorization) {
    for (const auto& testVector: testVectors) {
        SystemAbstractions::File creqFile(testVector.substr(0, testVector.length() - 3) + "creq");
        ASSERT_TRUE(creqFile.OpenReadOnly());
        SystemAbstractions::File::Buffer creqContents(creqFile.GetSize());
        ASSERT_EQ(creqFile.GetSize(), creqFile.Read(creqContents));
        SystemAbstractions::File stsFile(testVector.substr(0, testVector.length() - 3) + "sts");
        ASSERT_TRUE(stsFile.OpenReadOnly());
        SystemAbstractions::File::Buffer stsContents(stsFile.GetSize());
        ASSERT_EQ(stsFile.GetSize(), stsFile.Read(stsContents));
        SystemAbstractions::File authzFile(testVector.substr(0, testVector.length() - 3) + "authz");
        ASSERT_TRUE(authzFile.OpenReadOnly());
        SystemAbstractions::File::Buffer authzContents(authzFile.GetSize());
        ASSERT_EQ(authzFile.GetSize(), authzFile.Read(authzContents));
        const std::string accessKeyId = "AKIDEXAMPLE";
        const std::string accessKeySecret = "wJalrXUtnFEMI/K7MDENG+bPxRfiCYEXAMPLEKEY";
        EXPECT_EQ(
            std::string(authzContents.begin(), authzContents.end()),
            Aws::SignApi::MakeAuthorization(
                std::string(stsContents.begin(), stsContents.end()),
                std::string(creqContents.begin(), creqContents.end()),
                accessKeyId,
                accessKeySecret
            )
        ) << "******** The name of the test vector that failed was: " << GetFileNameOnly(testVector);
    }
}

TEST_F(SignApiTests, MakeAuthorizationTestCaseFromDocumentation) {
    EXPECT_EQ(
        "AWS4-HMAC-SHA256 Credential=AKIDEXAMPLE/20150830/us-east-1/iam/aws4_request, SignedHeaders=content-type;host;x-amz-date, Signature=5d672d79c15b13162d9279b0855cfba6789a8edb4c82c400e06b5924a6f2b5d7",
        Aws::SignApi::MakeAuthorization(
            (
                "AWS4-HMAC-SHA256\n"
                "20150830T123600Z\n"
                "20150830/us-east-1/iam/aws4_request\n"
                "f536975d06c0309214f805bb90ccff089219ecd68b2577efef23edd43b7e1a59"
            ),
            (
                "GET\n"
                "/\n"
                "Action=ListUsers&Version=2010-05-08\n"
                "content-type:application/x-www-form-urlencoded; charset=utf-8\n"
                "host:iam.amazonaws.com\n"
                "x-amz-date:20150830T123600Z\n"
                "\n"
                "content-type;host;x-amz-date\n"
                "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
            ),
            "AKIDEXAMPLE",
            "wJalrXUtnFEMI/K7MDENG+bPxRfiCYEXAMPLEKEY"
        )
    );
}
