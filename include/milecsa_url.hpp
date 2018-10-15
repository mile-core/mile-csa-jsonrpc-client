//
// Created by denn on 05/10/2018.
//

#pragma once

#include <optional>
#include <string>
#include <functional>
#include "milecsa.hpp"
#include "milecsa_error.hpp"

namespace milecsa::rpc {

    /**
     * Url class
     */
    class Url {

    public:

        /**
         * Url protocol is supported by the current version
         */
        enum protocol{
            http = 0,
            https,
            unknown
        };

    public:

        /**
         * Parse url string
         * @param urlString - url string
         * @param error - error handler
         * @return optional Url object
         */
        static std::optional<Url> Parse(
                const std::string &urlString,
                const milecsa::ErrorHandler &error = default_error_handler);

        /**
         * Copy Url object
         */
        Url(const Url &);

        /**
         * Get url reference string
         * @return - url encoded string
         */
        const std::string &get_absolut_string() const { return url_string_; };

        /**
         * Get url protocol
         * @return - url protocol
         */
        const protocol get_protocol() const { return protocol_; };

        /**
         * Get host
         * @return - host string
         */
        const std::string &get_host() const { return domain_; };

        /**
         * Get port
         * @return port number
         */
        const uint16_t get_port() const { return port_; };

        /**
         * Get query
         * @return - query string
         */
        const std::string &get_query() const { return query_;}

        /**
         * Get path
         * @return - file path string
         */
        const std::string &get_path() const { return path_;}

        /**
         * Get fragment url
         * @return - fragment url
         */
        const std::string &get_fragment() const { return fragment_;}

        Url& operator=(const Url&);

    private:
        Url(const std::string &urlString);
        Url(){};

        std::string url_string_;
        protocol protocol_;
        std::string domain_;
        uint16_t port_;
        std::string path_;
        std::string fragment_;
        std::string query_;

    };
}
