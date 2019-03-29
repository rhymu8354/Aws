#pragma once

/**
 * @file S3.hpp
 *
 * This module declares the Aws::S3 class.
 *
 * © 2019 by Richard Walters
 */

#include "Config.hpp"

#include <future>
#include <Http/IClient.hpp>
#include <MessageHeaders/MessageHeaders.hpp>
#include <string>
#include <vector>

namespace Aws {

    /**
     * This class provides an abstraction for the Amazon Simple Storage Service
     * (S3) Representational State Transfer (REST) Application Programming
     * Interface (API).
     */
    class S3 {
        // Types
    public:
        /**
         * This describes the owner of an S3 bucket.
         */
        struct Owner {
            /**
             * This is the owner's canonical user ID.
             */
            std::string id;

            /**
             * This is the owner's display name, if known.
             */
            std::string displayName;
        };

        /**
         * This contains information about an S3 bucket.
         */
        struct Bucket {
            /**
             * This is the name of the S3 bucket.
             */
            std::string name;

            /**
             * This is the time, in seconds past the UNIX epoch (midnight UTC,
             * January 1, 1970), when the S3 bucket was created.
             */
            double creationDate = 0.0;
        };

        /**
         * This contains information about an object in an S3 bucket.
         */
        struct Object {
            /**
             * This is the key of the object.
             */
            std::string key;

            /**
             * This is the entity tag of the object.
             */
            std::string eTag;

            /**
             * This is the time, in seconds past the UNIX epoch (midnight UTC,
             * January 1, 1970), when the object was last modified.
             */
            double lastModified = 0.0;

            /**
             * This is the size of the object in bytes.
             */
            size_t size = 0;
        };

        /**
         * This holds the information returned by the S3 ListBuckets API.
         */
        struct ListBucketsResult {
            /**
             * This is where the final state of the transaction for the last
             * request made to S3.
             */
            Http::IClient::Transaction::State transactionState = Http::IClient::Transaction::State::InProgress;

            /**
             * This is the HTTP status code from the last request made to S3.
             */
            unsigned int statusCode = 0;

            /**
             * This describes the owner of the S3 buckets.
             */
            Owner owner;

            /**
             * This contains information about the S3 buckets listed.
             */
            std::vector< Bucket > buckets;

            /**
             * If the request was not completely successful, this is a copy
             * of the error information provided in the last response.
             */
            Json::Value errorInfo;
        };

        /**
         * This holds the information returned by the S3 ListObjects API.
         */
        struct ListObjectsResult {
            /**
             * This is where the final state of the transaction for the last
             * request made to S3.
             */
            Http::IClient::Transaction::State transactionState = Http::IClient::Transaction::State::InProgress;

            /**
             * This is the HTTP status code from the last request made to S3.
             */
            unsigned int statusCode = 0;

            /**
             * This contains information about the objects in the S3 bucket.
             */
            std::vector< Object > objects;

            /**
             * If the request was not completely successful, this is a copy
             * of the error information provided in the last response.
             */
            Json::Value errorInfo;
        };

        /**
         * This holds the information returned by the S3 GetObject API.
         */
        struct GetObjectResult {
            /**
             * This is where the final state of the transaction for the last
             * request made to S3.
             */
            Http::IClient::Transaction::State transactionState = Http::IClient::Transaction::State::InProgress;

            /**
             * This is the HTTP status code from the last request made to S3.
             */
            unsigned int statusCode = 0;

            /**
             * This contains the content of the object in the S3 bucket.
             */
            std::string content;

            /**
             * This contains a copy of the headers provided from the S3
             * response, which contains metadata and other information
             * about the object and its retrieval.
             */
            MessageHeaders::MessageHeaders headers;

            /**
             * If the request was not completely successful, this is a copy
             * of the error information provided in the last response.
             */
            Json::Value errorInfo;
        };

        // Lifecycle management
    public:
        ~S3() noexcept;
        S3(const S3&) = delete;
        S3(S3&&) noexcept;
        S3& operator=(const S3&) = delete;
        S3& operator=(S3&&) noexcept;

        // Public methods
    public:
        /**
         * This is the default constructor.
         */
        S3();

        /**
         * Set up the object to communicate with Amazon S3.
         *
         * @param[in] http
         *     This is the HTTP client to use to communicate with Amazon S3.
         *
         * @param[in] config
         *     This is the Amazon Web Services (AWS) configuration to use.
         */
        void Configure(
            std::shared_ptr< Http::IClient > http,
            Config config = Config::GetDefaults()
        );

        /**
         * Retrieve the list of the S3 buckets available to the user.
         *
         * @return
         *     A future is returned which will return the results of the
         *     S3 request.
         */
        std::future< ListBucketsResult > ListBuckets();

        /**
         * Retrieve the list of the objects in the given S3 bucket.
         *
         * @param[in] bucketName
         *     This is the name of the bucket whose objects should be listed.
         *
         * @return
         *     A future is returned which will return the results of the
         *     S3 request.
         */
        std::future< ListObjectsResult > ListObjects(const std::string& bucketName);

        /**
         * Retrieve the contents of an object in the given S3 bucket.
         *
         * @param[in] bucketName
         *     This is the name of the bucket containing the object.
         *
         * @param[in] objectName
         *     This is the name of the object to retrieve.
         *
         * @return
         *     A future is returned which will return the results of the
         *     S3 request.
         */
        std::future< GetObjectResult > GetObject(
            const std::string& bucketName,
            const std::string& objectName
        );

        // Private properties
    private:
        /**
         * This is the type of structure that contains the private
         * properties of the instance.  It is defined in the implementation
         * and declared here to ensure that it is scoped inside the class.
         */
        struct Impl;

        /**
         * This contains the private properties of the instance.
         */
        std::shared_ptr< Impl > impl_;
    };

}
