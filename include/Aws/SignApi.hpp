#ifndef AWS_SIGN_API_HPP
#define AWS_SIGN_API_HPP

/**
 * @file SignApi.hpp
 *
 * This module declares the Aws::SignApi class.
 *
 * © 2018 by Richard Walters
 */

#include <string>

namespace Aws {

    /**
     * This class contains methods used in different stages of signing Amazon
     * Web Services (AWS) Application Programming Interface (API) calls.
     */
    class SignApi {
        // Public methods
    public:
        /**
         * This function constructs the "canonical request" which corresponds
         * to the given raw API request.  "Canonical request" is as defined by
         * Amazon here:
         * https://docs.aws.amazon.com/general/latest/gr/sigv4-create-canonical-request.html.
         *
         * @param[in] rawRequest
         *     This is the raw API request message.
         *
         * @return
         *     The corresponding canonical request is returned.
         */
        static std::string ConstructCanonicalRequest(const std::string& rawRequest);
    };

}

#endif /* AWS_SIGN_API_HPP */
