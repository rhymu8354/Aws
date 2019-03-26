/**
 * @file S3Tests.cpp
 *
 * This module contains the unit tests of the
 * Aws::S3 class.
 *
 * © 2019 by Richard Walters
 */

#include <Aws/Config.hpp>
#include <Aws/S3.hpp>
#include <future>
#include <gtest/gtest.h>
#include <Http/IClient.hpp>

namespace {

    struct MockHttpClentTransaction
        : public Http::IClient::Transaction
    {
        // Properties

        std::function< void() > completionDelegate;
        std::promise< void > completed;

        // Methods

        void Complete() {
            if (completionDelegate != nullptr) {
                completionDelegate();
            }
            completed.set_value();
        }

        // Http::IClient::Transaction

        virtual bool AwaitCompletion(
            const std::chrono::milliseconds& relativeTime
        ) {
            return (completed.get_future().wait_for(relativeTime) == std::future_status::ready);
        }

        virtual void AwaitCompletion() {
            completed.get_future().get();
        }

        virtual void SetCompletionDelegate(
            std::function< void() > completionDelegate
        ) {
            this->completionDelegate = completionDelegate;
        }
    };

    struct MockHttpClient
        : public Http::IClient
    {
        // Properties

        std::shared_ptr< MockHttpClentTransaction > transaction;
        std::promise< Http::Request > request;

        // Http::IClient

        virtual SystemAbstractions::DiagnosticsSender::UnsubscribeDelegate SubscribeToDiagnostics(
            SystemAbstractions::DiagnosticsSender::DiagnosticMessageDelegate delegate,
            size_t minLevel = 0
        ) override {
            return []{};
        }

        virtual std::shared_ptr< Http::IClient::Transaction > Request(
            Http::Request request,
            bool persistConnection = true,
            UpgradeDelegate upgradeDelegate = nullptr
        ) override {
            transaction = std::make_shared< MockHttpClentTransaction >();
            this->request.set_value(request);
            return transaction;
        }
    };

}

/**
 * This is the test fixture for these tests, providing common
 * setup and teardown for each test.
 */
struct S3Tests
    : public ::testing::Test
{
    // Properties

    Aws::S3 s3;
    std::shared_ptr< MockHttpClient > mockClient = std::make_shared< MockHttpClient >();

    // ::testing::Test

    virtual void SetUp() override {
        Aws::Config config;
        config.region = "foobar";
        config.accessKeyId = "alex123";
        config.secretAccessKey = "letmein";
        s3.Configure(mockClient, config);
    }

    virtual void TearDown() override {
    }
};

TEST_F(S3Tests, ListBuckets) {
    auto requestFuture = mockClient->request.get_future();
    auto listBucketsFuture = s3.ListBuckets();
    ASSERT_EQ(
        std::future_status::ready,
        requestFuture.wait_for(std::chrono::milliseconds(100))
    );
    auto request = requestFuture.get();
    EXPECT_EQ("GET", request.method);
    EXPECT_EQ("//s3.foobar.amazonaws.com:443/", request.target.GenerateString());
    EXPECT_TRUE(request.headers.HasHeader("x-amz-date"));
    EXPECT_TRUE(request.headers.HasHeader("Authorization"));
    EXPECT_TRUE(request.headers.HasHeader("x-amz-content-sha256"));
    EXPECT_NE(
        std::future_status::ready,
        listBucketsFuture.wait_for(std::chrono::seconds(0))
    );
    mockClient->transaction->state = Http::IClient::Transaction::State::Completed;
    mockClient->transaction->response.statusCode = 200;
    mockClient->transaction->response.body = (
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<ListAllMyBucketsResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
        "<Owner><ID>12345</ID><DisplayName>alex</DisplayName></Owner>"
        "<Buckets>"
        "<Bucket><Name>foo</Name><CreationDate>2018-02-01T08:30:12.123Z</CreationDate></Bucket>"
        "<Bucket><Name>bar</Name><CreationDate>2018-06-08T11:25:43.456Z</CreationDate></Bucket>"
        "</Buckets>"
        "</ListAllMyBucketsResult>"
    );
    mockClient->transaction->response.state = Http::Response::State::Complete;
    mockClient->transaction->Complete();
    ASSERT_EQ(
        std::future_status::ready,
        listBucketsFuture.wait_for(std::chrono::milliseconds(1000))
    );
    auto listBuckets = listBucketsFuture.get();
    EXPECT_EQ(200, listBuckets.statusCode);
    EXPECT_EQ("12345", listBuckets.owner.id);
    EXPECT_EQ("alex", listBuckets.owner.displayName);
    ASSERT_EQ(2, listBuckets.buckets.size());
    EXPECT_EQ("foo", listBuckets.buckets[0].name);
    EXPECT_EQ(1517473812.123, listBuckets.buckets[0].creationDate);
    EXPECT_EQ("bar", listBuckets.buckets[1].name);
    EXPECT_EQ(1528457143.456, listBuckets.buckets[1].creationDate);
}
