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
#include <SystemAbstractions/StringExtensions.hpp>

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

    static std::string CleanUpRequest(const std::string& input) {
        std::ostringstream output;
        for (const auto& line: SystemAbstractions::Split(input, '\n')) {
            output << line << "\r\n";
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
        ASSERT_TRUE(testVectorFile.Open());
        SystemAbstractions::File::Buffer testVectorContents(testVectorFile.GetSize());
        ASSERT_EQ(testVectorFile.GetSize(), testVectorFile.Read(testVectorContents));
        SystemAbstractions::File creqFile(testVector.substr(0, testVector.length() - 3) + "creq");
        ASSERT_TRUE(creqFile.Open());
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

TEST_F(SignApiTests, MakeStringToSign) {
}

TEST_F(SignApiTests, MakeAuthorization) {
}
