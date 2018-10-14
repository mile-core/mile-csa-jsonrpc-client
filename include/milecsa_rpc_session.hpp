//
// Created by lotus mile on 13/10/2018.
//

#pragma once

#include "json.hpp"
#include "milecsa_url.hpp"
#include "milecsa_rpc_id.hpp"
#include "crypto_types.h"

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
        typedef boost::beast::http::response<boost::beast::http::string_body> response;
        typedef std::function<void(const response &response)> ResponseHandler;
        static auto default_response_handler = [](const response &response) {};
    }

    namespace rpc {
        using json = nlohmann::json ;
        typedef std::optional<json> response;
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
                           bool verify = true):

                        host(host),
                        port(boost::to_string(port)),
                        target(target),

                        use_ssl(protocol == Url::protocol::https),
                        verify_ssl(verify),
                        resolver(0),
                        socket(0),
                        stream(0) {

                    if (use_ssl){
                        ssl::context ctx{ssl::context::tls_client};
                        ctx.set_options(ssl::context::sslv23_client|ssl::context::tlsv12_client);

                        if (verify) {
                            ctx.set_verify_mode(ssl::verify_client_once);
                        }
                        else {
                            ctx.set_verify_mode(ssl::verify_none);
                        }

                        stream = new ssl::stream<tcp::socket>{ioc, ctx};
                    }
                    else {
                        socket = new tcp::socket(ioc);
                    }

                    resolver = new tcp::resolver(ioc);
                }

                ~RpcSession(){
                    if(socket) {
                        boost::system::error_code ec;
                        socket->shutdown(tcp::socket::shutdown_both, ec);
                        delete socket;
                    }
                    if (stream) {
                        boost::system::error_code ec;
                        stream->shutdown(ec);
                        delete stream;
                    }
                    if (resolver) {
                        delete resolver;
                    }
                }

                bool connect(const milecsa::ErrorHandler &error) {

                    auto const results = resolver->resolve(host, port);

                    if (results.empty()) {
                        error(result::NOT_FOUND,ErrorFormat("Host not found"));
                        return false;
                    }

                    if (use_ssl) {

                        stream->set_verify_callback([&](bool preverified,
                                                        boost::asio::ssl::verify_context& ctx){
                            return this->verify_ssl;
                        });

                        if(! SSL_set_tlsext_host_name(stream->native_handle(), host.c_str()))
                        {
                            error(result::FAIL,ErrorFormat("SSL handshake error"));
                            return false;
                        }
                        try {
                            boost::asio::connect(stream->next_layer(), results.begin(), results.end());
                            stream->handshake(ssl::stream_base::client);
                        }
                        catch(std::exception const& e)
                        {
                            error(result::FAIL,ErrorFormat(e.what()));
                            return false;
                        }
                    }
                    else {
                        boost::asio::connect(*socket, results.begin(), results.end());
                    }
                    return true;
                }

                rpc::response request(const rpc::request &data,
                                      const http::ResponseHandler &handler = http::default_response_handler,
                                      const milecsa::ErrorHandler &error = default_error_handler) {

                    boost::beast::http::request<boost::beast::http::string_body> req;

                    try {
                        // Set up an HTTP GET request message
                        req.version(11);
                        req.method(boost::beast::http::verb::post);
                        req.target(target);
                        req.set(boost::beast::http::field::host, host);
                        req.set(boost::beast::http::field::user_agent, user_agent);
                        req.set(boost::beast::http::field::content_type, "application/json");

                        auto _data = data.dump(); //+ "\n";
                        req.set(boost::beast::http::field::content_length, _data.size());
                        req.body() = _data;
                        req.prepare_payload();

                        if (RpcSession::debug_on) {
                            std::cerr << "\nDebug info: " << host << std::endl;
                            std::cerr << req << std::endl;
                            std::cerr << "..." << std::endl;
                        }

                        if (use_ssl) {
                            boost::beast::http::write(*stream, req);
                        } else {
                            boost::beast::http::write(*socket, req);
                        }

                        boost::beast::flat_buffer buffer;
                        boost::beast::http::response<boost::beast::http::string_body> res;

                        if (use_ssl) {
                            boost::beast::http::read(*stream, buffer, res);
                        } else {
                            boost::beast::http::read(*socket, buffer, res);
                        }

                        auto status = res.result();

                        if (RpcSession::debug_on) {
                            std::cerr << "\nResponse info: " << status << std::endl;
                            std::cerr << res << std::endl;
                            std::cerr << "..." << std::endl;
                        }
                        if (status == boost::beast::http::status::ok) {
                            auto json = json::parse(res.body().data());
                            if (json.count("result") > 0 && json["result"] != nullptr){
                                return json;
                            }
                        }

                        handler(res);
                        return std::nullopt;
                    }
                    catch(std::exception const& e)
                    {
                        error(result::FAIL,ErrorFormat(e.what()));
                        return std::nullopt;
                    }
                }

                json next_command(const std::string &method, const rpc::request &params = {}) const {
                    json command = {
                            {"jsonrpc", "2.0"},
                            {"method", method},
                            {"params", params},
                            {"id", ClientId::Instance().get_next()},
                            {"version", 0.0}
                    };
                    return command;
                }

            private:
                bool use_ssl;
                bool verify_ssl;

                const std::string host;
                const std::string port;
                const std::string target;

                boost::asio::io_context ioc;
                tcp::resolver *resolver;
                tcp::socket   *socket;
                ssl::stream<tcp::socket> *stream;

            };
        }
    }
}
