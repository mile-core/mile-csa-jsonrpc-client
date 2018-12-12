//
// Created by lotus mile on 2018-12-12.
//

#pragma once

#include <optional>
#include <functional>
#include <any>
#include <boost/asio.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/beast/http.hpp>

#include "milecsa.hpp"
#include "milecsa_url.hpp"
#include "milecsa_rpc_session.hpp"

namespace milecsa {

    namespace http {

        static const std::string version = "v1.0";
        static const std::string user_agent = "MILE CLI Wallet" + version;

        /**
         * MILE Http utility client
         */
        class Client {

        public:
            static time_t timeout;

            static std::optional<Client> Connect(
                    const std::string &urlString,
                    bool verify_ssl = true,
                    const ErrorHandler &error_handler = default_error_handler);

            Client(const Client &client);

            ~Client();

            /**
             * Get the current url
             * @return url
             */
            const Url &get_url() const { return *url_; }

            /**
             * Get body by client url
             * @return otional body if operation is completed successfully
             */
            std::optional<std::string> get();

            Client& operator=(const Client&);

        private:

            Client(const Url &url,
                   bool verify_ssl,
                   const ErrorHandler &error_handler);

            Client():verify_ssl_(true),
                     error_handler(default_error_handler){};

            std::optional<Url> url_;
            bool verify_ssl_;
            std::shared_ptr<milecsa::http::Session> session;

            http::ResponseHandler response_fail_handler;
            ErrorHandler error_handler;
        };
    }
}