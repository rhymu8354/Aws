/**
 * @file Config.cpp
 *
 * This module contains the implementation of the Aws::Config class.
 *
 * © 2018 by Richard Walters
 */

#include <Aws/Config.hpp>
#include <stack>
#include <stdlib.h>
#include <string>
#include <StringExtensions/StringExtensions.hpp>
#include <SystemAbstractions/File.hpp>
#include <vector>

namespace {

    /**
     * This is the default function to use to read environment variables.
     */
    const Aws::Config::EnvironmentShim defaultEnvironmentShim = [](const std::string& name) -> std::string {
        const auto value = getenv(name.c_str());
        if (value == NULL) {
            return "";
        } else {
            return value;
        }
    };

    /**
     * This is the current function to use to read environment variables.
     */
    Aws::Config::EnvironmentShim currentEnvironmentShim = [](const std::string& name) -> std::string {
        const auto value = getenv(name.c_str());
        if (value == NULL) {
            return "";
        } else {
            return value;
        }
    };

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
                    const auto key = StringExtensions::Trim(line.substr(1, line.length() - 2));
                    auto* section = &config.Set(key, Json::Object({}));
                    context = decltype(context)();
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
            const auto key = StringExtensions::Trim(line.substr(0, delimiter));
            const auto value = StringExtensions::Trim(line.substr(delimiter + 1));
            if (value.empty()) {
                lastValue = &context.top().value->Set(key, Json::Object({}));
            } else {
                lastValue = &context.top().value->Set(key, value);
            }
        }
        return config;
    }

    Json::Value Config::FromFile(const std::string& configFilePath) {
        SystemAbstractions::File configFile(configFilePath);
        if (!configFile.Open()) {
            return nullptr;
        }
        SystemAbstractions::File::Buffer configFileContents(configFile.GetSize());
        if (configFile.Read(configFileContents) != configFile.GetSize()) {
            return nullptr;
        }
        return FromString(
            std::string(
                configFileContents.begin(),
                configFileContents.end()
            )
        );
    }

    Config Config::GetDefaults(const Json::Value& options) {
        std::string home = options["home"];
        std::string profile = options["profile"];
        if (home.empty()) {
            home = SystemAbstractions::File::GetUserHomeDirectory();
        }
        Config defaults;
        defaults.accessKeyId = currentEnvironmentShim("AWS_ACCESS_KEY_ID");
        defaults.accessKeyId = currentEnvironmentShim("AWS_SECRET_ACCESS_KEY");
        defaults.sessionToken = currentEnvironmentShim("AWS_SESSION_TOKEN");
        defaults.region = currentEnvironmentShim("AWS_DEFAULT_REGION");
        if (profile.empty()) {
            profile = currentEnvironmentShim("AWS_PROFILE");
        }
        std::string profileConfigSection = "profile " + profile;
        if (profile.empty()) {
            profile = "default";
            profileConfigSection = "default";
        }
        auto sharedCredentialsFile = currentEnvironmentShim("AWS_SHARED_CREDENTIALS_FILE");
        if (sharedCredentialsFile.empty()) {
            sharedCredentialsFile = home + "/.aws/credentials";
        }
        const auto sharedCredentials = FromFile(sharedCredentialsFile)[profile];
        if (defaults.accessKeyId.empty()) {
            defaults.accessKeyId = (std::string)sharedCredentials["aws_access_key_id"];
        }
        if (defaults.secretAccessKey.empty()) {
            defaults.secretAccessKey = (std::string)sharedCredentials["aws_secret_access_key"];
        }
        if (defaults.sessionToken.empty()) {
            defaults.sessionToken = (std::string)sharedCredentials["aws_session_token"];
        }
        auto configFile = currentEnvironmentShim("AWS_CONFIG_FILE");
        if (configFile.empty()) {
            configFile = home + "/.aws/config";
        }
        const auto config = FromFile(configFile)[profileConfigSection];
        if (defaults.accessKeyId.empty()) {
            defaults.accessKeyId = (std::string)config["aws_access_key_id"];
        }
        if (defaults.secretAccessKey.empty()) {
            defaults.secretAccessKey = (std::string)config["aws_secret_access_key"];
        }
        if (defaults.sessionToken.empty()) {
            defaults.sessionToken = (std::string)config["aws_session_token"];
        }
        if (defaults.region.empty()) {
            defaults.region = (std::string)config["region"];
        }
        return defaults;
    }

    void Config::SetEnvironmentShim(EnvironmentShim environmentShim) {
        if (environmentShim == nullptr) {
            currentEnvironmentShim = defaultEnvironmentShim;
        } else {
            currentEnvironmentShim = environmentShim;
        }
    }

}
