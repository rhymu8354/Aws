#ifndef AWS_CONFIG_HPP
#define AWS_CONFIG_HPP

/**
 * @file Config.hpp
 *
 * This module declares the Aws::Config class.
 *
 * © 2018 by Richard Walters
 */

#include <Json/Value.hpp>

namespace Aws {

    /**
     * This class contains methods used to read and write Amazon Web Services
     * (AWS) configuration files.
     */
    class Config {
        // Public methods
    public:
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
    };

}

#endif /* AWS_CONFIG_HPP */
