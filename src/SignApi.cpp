/**
 * @file SignApi.cpp
 *
 * This module contains the implementation of the Aws::SignApi class.
 *
 * © 2018 by Richard Walters
 */

#include <algorithm>
#include <Aws/SignApi.hpp>
#include <Hash/Hmac.hpp>
#include <Hash/Sha2.hpp>
#include <Hash/Templates.hpp>
#include <Http/Server.hpp>
#include <map>
#include <MessageHeaders/MessageHeaders.hpp>
#include <sstream>
#include <string>
#include <SystemAbstractions/StringExtensions.hpp>
#include <vector>

namespace {

    /**
     * This is the identifier defined by Amazon for the hash algorithm
     * used to make message digests in this module.
     */
    static const char* const HASH_ALGORITHM = "AWS4-HMAC-SHA256";

    /**
     * This function replaces sequences of two or more spaces with a single
     * space, to meet the requirement of header values in canonical API
     * requests, that multiple spaces should be replaced by a single space.
     *
     * @param[in] s
     *     This is the string for which to replace multiple spaces with a
     *     single space.
     *
     * @return
     *     The given string, with multiple spaces replaced by a single space,
     *     is returned.
     */
    std::string CanonicalizeSpaces(const std::string& s) {
        std::ostringstream output;
        bool lastCharWasSpace = false;
        for (const auto& c: s) {
            if (c == ' ') {
                if (!lastCharWasSpace) {
                    lastCharWasSpace = true;
                    output << ' ';
                }
            } else {
                output << c;
                lastCharWasSpace = false;
            }
        }
        return output.str();
    }

}

namespace Aws {

    std::string SignApi::ConstructCanonicalRequest(const std::string& rawRequest) {
        Http::Server server;
        const auto request = server.ParseRequest(rawRequest);
        if (request == nullptr) {
           return "";
        }
        std::ostringstream canonicalRequest;

        // The following steps should match those shown here:
        // https://docs.aws.amazon.com/general/latest/gr/sigv4-create-canonical-request.html

        // Step 1
        canonicalRequest << request->method << "\n";

        // Step 2
        Uri::Uri requestPath;
        requestPath.SetPath(request->target.GetPath());
        requestPath.NormalizePath();
        canonicalRequest << requestPath.GenerateString() << "\n";

        // Step 3
        if (request->target.HasQuery()) {
            Uri::Uri requestQuery;
            requestQuery.SetQuery(request->target.GetQuery());
            auto requestQueryEncoded = requestQuery.GenerateString();
            if (requestQueryEncoded.length() > 0) {
                requestQueryEncoded = requestQueryEncoded.substr(1);
            }
            auto parametersString = SystemAbstractions::Split(requestQueryEncoded, '&');
            struct Parameter {
                std::string name;
                std::string value;
            };
            std::vector< Parameter > parametersVector;
            for (const auto& parameter: parametersString) {
                const auto delimiter = parameter.find('=');
                if (delimiter == std::string::npos) {
                    parametersVector.push_back({parameter, ""});
                } else {
                    parametersVector.push_back({
                        parameter.substr(0, delimiter),
                        parameter.substr(delimiter + 1)
                    });
                }
            }
            std::sort(
                parametersVector.begin(),
                parametersVector.end(),
                [](
                    const Parameter& lhs,
                    const Parameter& rhs
                ) {
                    if (lhs.name < rhs.name) {
                        return true;
                    } else if (lhs.name > rhs.name) {
                        return false;
                    } else {
                        return lhs.value < rhs.value;
                    }
                }
            );
            bool first = true;
            for (const auto& parameter: parametersVector) {
                if (first) {
                    first = false;
                } else {
                    canonicalRequest << '&';
                }
                canonicalRequest << parameter.name << '=' << parameter.value;
            }
        }
        canonicalRequest << "\n";

        // Step 4
        std::map<
            std::string,
            std::vector< std::string >
        > headersByName;
        for (const auto& header: request->headers.GetAll()) {
            auto& headerValues = headersByName[SystemAbstractions::ToLower(header.name)];
            headerValues.push_back(CanonicalizeSpaces(header.value));
        }
        struct Header {
            std::string name;
            std::vector< std::string > values;
        };
        std::vector< Header > headersVector;
        headersVector.reserve(headersByName.size());
        for (auto& header: headersByName) {
            headersVector.push_back({
                header.first,
                header.second
            });
        }
        std::sort(
            headersVector.begin(),
            headersVector.end(),
            [](
                const Header& lhs,
                const Header& rhs
            ) {
                return (lhs.name < rhs.name);
            }
        );
        for (const auto& header: headersVector) {
            canonicalRequest << header.name << ':' << SystemAbstractions::Join(header.values, ",") << "\n";
        }
        canonicalRequest << "\n";

        // Step 5
        bool first = true;
        for (const auto& header: headersVector) {
            if (first) {
                first = false;
            } else {
                canonicalRequest << ';';
            }
            canonicalRequest << header.name;
        }
        canonicalRequest << "\n";

        // Step 6
        canonicalRequest << Hash::StringToString< Hash::Sha256 >(request->body);

        // Done.  Return constructed request.
        return canonicalRequest.str();
    }

    std::string SignApi::MakeStringToSign(
        const std::string& region,
        const std::string& service,
        const std::string& canonicalRequest
    ) {
        std::ostringstream output;
        std::string dateTime;
        for (const auto& line: SystemAbstractions::Split(canonicalRequest, '\n')) {
            if (line.substr(0, 11) == "x-amz-date:") {
                dateTime = line.substr(11);
                break;
            }
        }
        output
            << HASH_ALGORITHM << "\n"
            << dateTime << "\n"
            << dateTime.substr(0, 8) << "/" << region << "/" << service << "/aws4_request\n"
            << Hash::StringToString< Hash::Sha256 >(canonicalRequest);
        return output.str();
    }

    std::string SignApi::MakeAuthorization(
        const std::string& stringToSign,
        const std::string& canonicalRequest,
        const std::string& accessKeyId,
        const std::string& accessKeySecret
    ) {
        std::ostringstream output;
        const auto credentialScope = SystemAbstractions::Split(stringToSign, '\n')[2];
        const auto credentialScopeParts = SystemAbstractions::Split(credentialScope, '/');
        const auto date = credentialScopeParts[0];
        const auto region = credentialScopeParts[1];
        const auto service = credentialScopeParts[2];
        const auto terminationString = credentialScopeParts[3];
        const auto hmacRawStringToBytes = Hash::MakeHmacStringToBytesFunction(
            Hash::StringToBytes< Hash::Sha256 >,
            Hash::SHA256_BLOCK_SIZE
        );
        const auto hmacBytesToBytes = Hash::MakeHmacBytesToBytesFunction(
            Hash::Sha256,
            Hash::SHA256_BLOCK_SIZE
        );
        const auto hmacBytesToHexString = Hash::MakeHmacBytesToStringFunction(
            Hash::BytesToString< Hash::Sha256 >,
            Hash::SHA256_BLOCK_SIZE
        );
        const auto signingKey = hmacBytesToBytes(
            hmacBytesToBytes(
                hmacBytesToBytes(
                    hmacRawStringToBytes(
                        "AWS4" + accessKeySecret,
                        date
                    ),
                    std::vector< uint8_t >(region.begin(), region.end())
                ),
                std::vector< uint8_t >(service.begin(), service.end())
            ),
            std::vector< uint8_t >(terminationString.begin(), terminationString.end())
        );
        const auto signature = hmacBytesToHexString(
            signingKey,
            std::vector< uint8_t >(stringToSign.begin(), stringToSign.end())
        );
        const auto canonicalRequestLines = SystemAbstractions::Split(canonicalRequest, '\n');
        const auto signedHeaders = canonicalRequestLines[canonicalRequestLines.size() - 2];
        output
            << HASH_ALGORITHM
            << " Credential=" << accessKeyId << "/" << credentialScope
            << ", SignedHeaders=" << signedHeaders
            << ", Signature=" << signature;
        return output.str();
    }

}
