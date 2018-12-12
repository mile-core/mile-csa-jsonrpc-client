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

        using Url = milecsa::rpc::Url;

        typedef boost::beast::http::status status;

        /**
         * JSON-RPC  HTTP response string body
         */
        typedef boost::beast::http::response<boost::beast::http::string_body> response;

        /**
         * JSON-RPC HTTP response fail handler
         */
        typedef std::function<void(const status code, const std::string &method,
                                   const response &response)> ResponseHandler;

        /**
         * Default response fail handler
         */
        static auto default_response_handler = [](const status code, const std::string &method,
                                                  const response &response) {};

        using namespace boost::asio::ip;
        namespace ssl = boost::asio::ssl;

        class Session {
        public:
            /**
             * Create single JSON-RPC over HTTP/HTTPS session
             *
             * @param host - json-rpc host
             * @param port - http/s port
             * @param target - target path
             * @param protocol - protocol, supported http or https
             * @param verify - verify ssl certs
             */
            Session(const std::string &host,
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
             * Get the current uri target
             * @return string
             */
            const std::string &get_target() const { return target;}

            /**
             * Get the current host
             * @return string
             */
            const std::string &get_host() const { return host;}

            /**
             * Get the current port
             * @return string
             */
            const std::string &get_port() const { return port;}

            /**
             * Get the current operations timeout
             * @return time
             */
            time_t get_timeout() const { return timeout;}

            /**
             * Write request body
             * @tparam T
             * @param body - body of request
             * @param error_handler
             * @return true if operation is completed successfully
             */
            template<typename T>
            bool write(T &req,
                       const milecsa::ErrorHandler &error_handler){

                boost::posix_time::time_duration tm = boost::posix_time::seconds(get_timeout());
                boost::system::error_code ec = boost::asio::error::would_block;

                deadline.expires_from_now(tm);

                if (use_ssl) {
                    boost::beast::http::async_write(*stream, req, [&ec](const boost::system::error_code& error, size_t){
                        ec = error;
                    });
                } else {
                    boost::beast::http::async_write(*socket, req, [&ec](const boost::system::error_code& error, size_t){
                        ec = error;
                    });
                }

                do ioc.run_one(); while (ec == boost::asio::error::would_block);

                if (ec || !check_socket()) {
                    error_handler(result::TIMEOUT, ErrorFormat("%s %s: %s:%s",
                                                               "Sending request timeout",
                                                               boost::system::system_error(
                                                                       ec ? ec : boost::asio::error::operation_aborted).what(),
                                                               host.c_str(), port.c_str()));
                    return false;
                }

                return true;
            };

            /**
             * Read response
             * @tparam T
             * @param response - response message
             * @param error_handler
             * @return true if operation is completed successfully
             */
            template<typename T>
            bool read(T &response,
                      const milecsa::ErrorHandler &error_handler){

                boost::posix_time::time_duration tm = boost::posix_time::seconds(get_timeout());
                boost::system::error_code ec = boost::asio::error::would_block;

                boost::beast::flat_buffer buffer;

                deadline.expires_from_now(tm);
                ec = boost::asio::error::would_block;

                if (use_ssl) {
                    boost::beast::http::async_read(*stream, buffer, response, [&ec](const boost::system::error_code& error, size_t){
                        ec = error;
                    });
                } else {
                    boost::beast::http::async_read(*socket, buffer, response,  [&ec](const boost::system::error_code& error, size_t){
                        ec = error;
                    });
                }

                do ioc.run_one(); while (ec == boost::asio::error::would_block);

                if (ec || !check_socket()) {
                    error_handler(result::TIMEOUT, ErrorFormat("%s %s: %s:%s",
                                                               "Reading response timeout",
                                                               boost::system::system_error(
                                                                       ec ? ec : boost::asio::error::operation_aborted).what(),
                                                               host.c_str(), port.c_str()));
                    return false;
                }

                return true;
            };

            ~Session();

        private:
            bool use_ssl;
            bool verify_ssl;

            const std::string host;
            const std::string port;
            const std::string target;
            time_t timeout;

            boost::asio::io_context ioc;
            tcp::socket   *socket;
            ssl::stream<tcp::socket> *stream;

            boost::asio::deadline_timer deadline;

            bool prepare();
            void wait_deadline();
            bool check_socket();
        };
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
            class RpcSession: public milecsa::http::Session {

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
                //bool connect(const milecsa::ErrorHandler &error);

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
            };
        }
    }
}
