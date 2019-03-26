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
         * This holds the information returned by the S3 ListBuckets API.
         */
        struct ListBucketsResult {
            /**
             * This is where the final state of the transaction made to list
             * the buckets is stored.
             */
            Http::IClient::Transaction::State transactionState = Http::IClient::Transaction::State::InProgress;

            /**
             * This is the HTTP status code from the request made to list the
             * buckets.
             */
            unsigned int statusCode = 0;

            /**
             * This describes the owner of an S3 bucket.
             */
            Owner owner;

            /**
             * This contains information about the S3 buckets listed.
             */
            std::vector< Bucket > buckets;
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
