/**
 * @file Config.cpp
 *
 * This module contains the implementation of the Aws::Config class.
 *
 * © 2018 by Richard Walters
 */

#include <Aws/Config.hpp>
#include <stack>
#include <string>
#include <SystemAbstractions/StringExtensions.hpp>
#include <vector>

namespace {

    /**
     * This function splits the given string into individual lines, where
     * lines may be delimited by any combination of carriage returns
     * and line feeds.
     *
     * @param[in] input
     *     This is the string to split into individual lines.
     *
     * @return
     *     The individual lines split from the input string is returned.
     */
    std::vector< std::string > SplitLines(const std::string& input) {
        std::vector< std::string > output;
        auto remainder = input;
        while (!remainder.empty()) {
            auto delimiter = remainder.find_first_of("\r\n");
            if (delimiter == std::string::npos) {
                output.push_back(remainder);
                break;
            } else {
                output.push_back(remainder.substr(0, delimiter));
                auto nonDelimiter = remainder.find_first_not_of("\r\n", delimiter);
                if (nonDelimiter == std::string::npos) {
                    break;
                } else {
                    remainder = remainder.substr(nonDelimiter);
                }
            }
        }
        return output;
    }

}

namespace Aws {

    Json::Value Config::FromString(const std::string& configString) {
        auto config = Json::Object({});
        Json::Value* lastValue = nullptr;
        struct Context {
            size_t indentation;
            Json::Value* value;
        };
        std::stack< Context > context;
        size_t indentation = 0;
        for (const auto& line: SplitLines(configString)) {
            if (line.empty()) {
                continue;
            }
            if (line[0] == '[') {
                if (line[line.length() - 1] == ']') {
                    const auto key = SystemAbstractions::Trim(line.substr(1, line.length() - 2));
                    auto* section = &config.Set(key, Json::Object({}));
                    context.swap(std::stack< Context >());
                    context.push({0, section});
                    lastValue = nullptr;
                }
                continue;
            }
            const auto indentation = line.find_first_not_of(' ');
            if (indentation == std::string::npos) {
                continue;
            }
            while (
                !context.empty()
                && (indentation < context.top().indentation)
            ) {
                context.pop();
            }
            if (context.empty()) {
                continue;
            }
            if (indentation > context.top().indentation) {
                if (lastValue == nullptr) {
                    continue;
                }
                context.push({indentation, lastValue});
                lastValue = nullptr;
            }
            const auto delimiter = line.find('=');
            if (delimiter == std::string::npos) {
                continue;
            }
            const auto key = SystemAbstractions::Trim(line.substr(0, delimiter));
            const auto value = SystemAbstractions::Trim(line.substr(delimiter + 1));
            if (value.empty()) {
                lastValue = &context.top().value->Set(key, Json::Object({}));
            } else {
                lastValue = &context.top().value->Set(key, value);
            }
        }
        return config;
    }

}
