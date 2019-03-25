#pragma once

/**
 * @file S3.hpp
 *
 * This module declares the Aws::S3 class.
 *
 * © 2019 by Richard Walters
 */

#include <string>

namespace Aws {

    /**
     * This class provides an abstraction for the Amazon Simple Storage Service
     * (S3) Representational State Transfer (REST) Application Programming
     * Interface (API).
     */
    class S3 {
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
        std::unique_ptr< Impl > impl_;
    };

}
