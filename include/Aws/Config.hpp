#ifndef AWS_CONFIG_HPP
#define AWS_CONFIG_HPP

/**
 * @file Config.hpp
 *
 * This module declares the Aws::Config class.
 *
 * © 2018 by Richard Walters
 */

#include <functional>
#include <Json/Value.hpp>
#include <string>

namespace Aws {

    /**
     * This structure contains Amazon Web Services (AWS) configuration items
     * and methods used to read them from files.
     */
    struct Config {
        // Types

        /**
         * This is the type of function which can be set up to be called by the
         * Config class whenever it needs to read environment variables.
         *
         * @param[in] name
         *     This is the name of the environment variable to read.
         *
         * @return
         *     The value of the environment variable is returned.
         */
        typedef std::function< std::string(const std::string& name) > EnvironmentShim;

        // Properties

        /**
         * This is part of the AWS access key to use when an access key is
         * required.  It's the part which is roughly equivalent to the
         * "user name" part of a user name / password combination.
         */
        std::string accessKeyId;

        /**
         * This is part of the AWS access key to use when an access key is
         * required.  It's the part which is roughly equivalent to the
         * "password" part of a user name / password combination.
         */
        std::string secretAccessKey;

        /**
         * If the AWS access key (to be used when an access key is
         * required) is a temporary security credential, this is the
         * additional security token that goes with the key (See
         * https://docs.aws.amazon.com/general/latest/gr/aws-sec-cred-types.html#access-keys-and-secret-access-keys
         * and
         * https://docs.aws.amazon.com/IAM/latest/UserGuide/id_credentials_temp.html).
         */
        std::string sessionToken;

        /**
         * This is the AWS region to which to direct API requests.
         */
        std::string region;

        // Methods

        /**
         * This function parses an AWS configuration from a string.
         *
         * @param[in] config
         *     This string contains the AWS configuration to parse.
         *
         * @return
         *     The parsed AWS configuration is returned.
         */
        static Json::Value FromString(const std::string& configString);

        /**
         * This function parses an AWS configuration from a file.
         *
         * @param[in] configFilePath
         *     This is the path to the AWS configuration file to parse.
         *
         * @return
         *     The parsed AWS configuration is returned.
         */
        static Json::Value FromFile(const std::string& configFilePath);

        /**
         * This function returns default configuration items determined
         * by reading environment variables and configuration files,
         * customized by the given options.
         *
         * @param[in] options
         *     This is an optional JSON object used to customize the
         *     construction of the default configuration items.  The following
         *     keys are currently understood:
         *
         *     - home:  directory to consider as the user's home directory
         *     - profile:  profile to select in configuration files
         *
         * @return
         *     The default configuration items determined by reading
         *     environment variables and configuration files is returned.
         */
        static Config GetDefaults(const Json::Value& options = Json::Object({}));

        /**
         * This function changes the function used by the Config class to read
         * environment variables.
         *
         * @param[in] environmentShim
         *     This is the function the Config class should use to read
         *     environment variables.  If nullptr, the default shim is set.
         */
        static void SetEnvironmentShim(EnvironmentShim environmentShim);
    };

}

#endif /* AWS_CONFIG_HPP */
