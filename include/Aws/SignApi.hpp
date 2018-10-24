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

        /**
         * This function constucts the "string to sign" for the given canonical
         * AWS API request to a server in the given region for the given
         * service.
         *
         * The "string to sign" is as defined by Amazon here:
         * https://docs.aws.amazon.com/general/latest/gr/sigv4-create-string-to-sign.html.
         *
         * @param[in] region
         *     This is the region of the server to which the request will
         *     be sent.
         *
         * @param[in] service
         *     This is the name of the service to which the request will
         *     be sent.
         *
         * @param[in] canonicalRequest
         *     This is the canonical AWS API request for which to make the
         *     string to sign.
         *
         * @return
         *     The string to sign is returned.
         */
        static std::string MakeStringToSign(
            const std::string& region,
            const std::string& service,
            const std::string& canonicalRequest
        );

        /**
         * This function constucts the authorization header value for the given
         * canonical AWS API request, using the given access key and string to
         * sign.
         *
         * The authorization header value is as defined by Amazon here:
         * https://docs.aws.amazon.com/general/latest/gr/sigv4-add-signature-to-request.html.
         *
         * @param[in] stringToSign
         *     This is the string to sign in order to make the signature
         *     included in the authorization.
         *
         * @param[in] canonicalRequest
         *     This is the canonical AWS API request for which to make the
         *     string to sign.
         *
         * @param[in] accessKeyId
         *     This is the ID of the key to use to sign the request.
         *
         * @param[in] accessKeySecret
         *     This is the secret value of the key to use to sign the request.
         *
         * @return
         *     The authorization value is returned.  This is added to the
         *     original request as a header named "Authorization", before
         *     sending the request to Amazon.
         */
        static std::string MakeAuthorization(
            const std::string& stringToSign,
            const std::string& canonicalRequest,
            const std::string& accessKeyId,
            const std::string& accessKeySecret
        );
    };

}

#endif /* AWS_SIGN_API_HPP */
