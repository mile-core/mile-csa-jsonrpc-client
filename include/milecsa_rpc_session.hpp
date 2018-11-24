//
// Created by lotus mile on 13/10/2018.
//

#pragma once

#include "json.hpp"
#include "milecsa_url.hpp"
#include "milecsa_rpc_id.hpp"

#include <optional>
#include <chrono>
#include <iostream>
#include <boost/format.hpp>
#include <boost/exception/all.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

namespace milecsa {

    namespace http {

        typedef boost::beast::http::status status;

        /**
         * JSON-RPC  HTTP response string body
         */
        typedef boost::beast::http::response<boost::beast::http::string_body> response;

        /**
         * JSON-RPC HTTP response fail handler
         */
        typedef std::function<void(const status code, const std::string &method, const response &response)> ResponseHandler;

        /**
         * Default response fail handler
         */
        static auto default_response_handler = [](const status code, const std::string &method, const response &response) {};
    }

    namespace rpc {
        /**
         * JSON structured object
         * @see https://github.com/nlohmann/json
         */
        using json = nlohmann::json ;

        /**
         * Optional JSON object
         */
        typedef std::optional<json> response;

        /**
         * Request as JSON body
         */
        typedef json request;
    }

    namespace rpc {
        static const std::string version = "v1.0";
        static const std::string user_agent = "MILE CLI Wallet" + version;
    }

    namespace rpc {

        using namespace boost::asio::ip;
        namespace ssl = boost::asio::ssl;

        typedef milecsa::rpc::IdCounter<uint64_t> ClientId;

        namespace detail {

            /**
             * Rpc session object
             */
            class RpcSession {

            public:

                /**
                 * Global debug option
                */
                static bool debug_on;

                /**
                 * Create single JSON-RPC over HTTP/HTTPS session
                 *
                 * @param host - json-rpc host
                 * @param port - http/s port
                 * @param target - target path
                 * @param protocol - protocol, supported http or https
                 * @param verify - verify ssl certs
                 */
                RpcSession(const std::string &host,
                           uint64_t port,
                           const std::string &target,
                           Url::protocol protocol,
                           bool verify = true,
                           time_t timeout = 3);

                /**
                 * Prepare rpc session connection
                 * @param error
                 * @return false in case when conection failed
                 */
                bool connect(const milecsa::ErrorHandler &error);

                /**
                 * Reconnect session in handle error
                 * @param error
                 * @return
                 */
                bool reconnect(const milecsa::ErrorHandler &error) ;

                /**
                 * Send JSON-RPC request width request body
                 * @param body - body of json repc request
                 * @param response_fail_handler - response fail handler
                 * @param error_handler - connection error handler
                 * @return response body
                 */
                rpc::response request(const rpc::request &body,
                                      const http::ResponseHandler &response_fail_handler = http::default_response_handler,
                                      const milecsa::ErrorHandler &error_handler = default_error_handler);

                /**
                 * Get next command body with method and their parameters
                 * @param method - json-rpc method
                 * @param params - json-rpc  method prarameters
                 * @return a new rpc request body
                 */
                rpc::request next_command(const std::string &method, const rpc::request &params = {}) const;

                ~RpcSession();

            private:
                bool use_ssl;
                bool verify_ssl;

                const std::string host;
                const std::string port;
                const std::string target;
                time_t timeout;

                boost::asio::io_context ioc;
                tcp::resolver *resolver;
                tcp::socket   *socket;
                ssl::stream<tcp::socket> *stream;

                bool prepare();
            };
        }
    }
}
