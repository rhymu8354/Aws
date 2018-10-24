/**
 * @file SignApi.cpp
 *
 * This module contains the implementation of the Aws::SignApi class.
 *
 * © 2018 by Richard Walters
 */

#include <algorithm>
#include <Aws/SignApi.hpp>
#include <Hash/Sha2.hpp>
#include <Hash/Templates.hpp>
#include <Http/Server.hpp>
#include <map>
#include <MessageHeaders/MessageHeaders.hpp>
#include <sstream>
#include <string>
#include <SystemAbstractions/StringExtensions.hpp>
#include <vector>

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
        canonicalRequest << requestPath.GenerateString() << "\n";

        // Step 3
        auto parametersString = SystemAbstractions::Split(request->target.GetQuery(), '&');
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
        canonicalRequest << "\n";

        // Step 4
        std::map<
            std::string,
            std::vector< std::string >
        > headersByName;
        for (const auto& header: request->headers.GetAll()) {
            auto& headerValues = headersByName[SystemAbstractions::ToLower(header.name)];
            // TODO: remove extra spaces from value (see
            // https://docs.aws.amazon.com/general/latest/gr/sigv4-create-canonical-request.html).
            headerValues.push_back(header.value);
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
        first = true;
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

}
