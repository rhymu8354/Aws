/**
 * @file S3.cpp
 *
 * This module contains the implementation of the Aws::S3 class.
 *
 * © 2019 by Richard Walters
 */

#include <Aws/S3.hpp>

namespace Aws {

    /**
     * This contains the private properties of an S3 instance.
     */
    struct S3::Impl {
    };

    S3::~S3() noexcept = default;
    S3::S3(S3&& other) noexcept = default;
    S3& S3::operator=(S3&& other) noexcept = default;

    S3::S3()
        : impl_(new Impl)
    {
    }

}
