//
// Created by denn on 15/10/2018.
//

#include "milecsa_rpc_session.hpp"
#include <optional>

namespace milecsa::rpc::detail {

    bool detail::RpcSession::debug_on = false;

    using loop_result = std::optional<boost::system::error_code>;
    using deadline_timer = boost::asio::deadline_timer;

    RpcSession::RpcSession(const std::string &host,
                           uint64_t port,
                           const std::string &target,
                           Url::protocol protocol,
                           bool verify,
                           time_t timeout):

            use_ssl(protocol == Url::protocol::https),
            verify_ssl(verify),
            timeout(timeout),

            host(host),
            port(boost::to_string(port)),
            target(target),

            resolver(0),
            socket(0),
            stream(0) {
        prepare();
    }

    bool RpcSession::reconnect(const milecsa::ErrorHandler &error) {
        if (prepare()){
            return connect(error);
        }
        return false;
    }

    bool RpcSession::prepare() {
        if (use_ssl){
            ssl::context ctx{ssl::context::tls_client};
            ctx.set_options(ssl::context::sslv23_client|ssl::context::tlsv12_client);

            if (verify_ssl) {
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

        return  resolver != nullptr;
    }

    bool RpcSession::connect(const milecsa::ErrorHandler &error) {

        try {
            auto const results = resolver->resolve(host, port);

            if (results.empty()) {
                error(result::NOT_FOUND,ErrorFormat("Host %s:%s not found", host.c_str(), port.c_str()));
                return false;
            }

            deadline_timer timer(use_ssl ? stream->get_io_service() : socket->get_io_service());

            loop_result connection_result;
            loop_result timer_result;

            auto timer_setup = [&](){
                connection_result.reset();
                timer_result.reset();

                timer.expires_from_now(boost::posix_time::seconds(timeout));
                timer.async_wait([&timer_result] (const boost::system::error_code& error) {
                    timer_result = std::make_optional(error); });
            };

            auto timeout_loop = [&]{
                if (use_ssl)
                    stream->get_io_service().reset();
                else
                    socket->get_io_service().reset();

                while ( (use_ssl ? stream->get_io_service().run_one() : socket->get_io_service().run_one()) )
                {
                    if (connection_result)
                        timer.cancel();
                    else if (timer_result){
                        error(result::TIMEOUT,ErrorFormat("%s: %s:%s",
                                                          "Connection timeout", host.c_str(), port.c_str()));
                        if (use_ssl)
                            stream->next_layer().cancel();
                        else
                            socket->cancel();

                        return false;
                    }
                }

                if (*connection_result){
                    error(result::FAIL,ErrorFormat("%s: %s:%s",
                                                   boost::system::system_error(*connection_result).what(), host.c_str(), port.c_str()));
                    return false;
                }

                return true;
            };

            if (use_ssl) {

                stream->set_verify_callback([&](bool preverified,
                                                boost::asio::ssl::verify_context& ctx){
                    return this->verify_ssl;
                });

                if(! SSL_set_tlsext_host_name(stream->native_handle(), host.c_str()))
                {
                    error(result::FAIL, ErrorFormat("SSL  %s:%s  handshake error", host.c_str(), port.c_str()));
                    return false;
                }
                try {

                    timer_setup();

                    stream->next_layer().async_connect(results->endpoint(),
                            [&connection_result](const boost::system::error_code& error){
                                connection_result = std::make_optional(error);
                    });

                    if (!timeout_loop())
                        return false;

                    timer_setup();

                    stream->async_handshake(ssl::stream_base::client,
                            [&connection_result](const boost::system::error_code& error){
                        connection_result = std::make_optional(error);
                    });

                    return timeout_loop();
                }
                catch(std::exception const& e)
                {
                    error(result::FAIL,ErrorFormat("%s: %s:%s", e.what(), host.c_str(), port.c_str()));
                    return false;
                }
                catch(boost::system::system_error const& e)
                {
                    error(result::FAIL,ErrorFormat("%s: %s:%s", e.what(), host.c_str(), port.c_str()));
                    return false;
                }
            }
            else {
                timer_setup();
                socket->async_connect(results->endpoint(),
                                      [&connection_result](const boost::system::error_code& error){
                                          connection_result = std::make_optional(error);
                                      });
                return timeout_loop();
            }
        }
        catch (std::exception const& e) {
            error(result::FAIL,ErrorFormat("%s: %s:%s", e.what(), host.c_str(), port.c_str()));
            return false;
        }
    }


    RpcSession::~RpcSession(){
        if(socket) {
            boost::system::error_code ec;
            socket->shutdown(tcp::socket::shutdown_both, ec);
            delete socket;
        }
        if (stream) {
            stream->async_shutdown([this](const boost::system::error_code&){
                delete stream;
            });
        }
        if (resolver) {
            delete resolver;
        }
    }

    rpc::response RpcSession::request(const rpc::request &body,
                                      const http::ResponseHandler &response_fail_handler,
                                      const milecsa::ErrorHandler &error_handler) {

        boost::beast::http::request<boost::beast::http::string_body> req;

        try {

            deadline_timer timer(use_ssl ? stream->get_io_service() : socket->get_io_service());

            loop_result connection_result;
            loop_result timer_result;

            auto timer_setup = [&](){
                connection_result.reset();
                timer_result.reset();

                timer.expires_from_now(boost::posix_time::seconds(timeout));
                timer.async_wait([&timer_result] (const boost::system::error_code& error) {
                    timer_result = std::make_optional(error); });
            };

            auto timeout_loop = [&]{
                if (use_ssl)
                    stream->get_io_service().reset();
                else
                    socket->get_io_service().reset();

                while ( (use_ssl ? stream->get_io_service().run_one() : socket->get_io_service().run_one()) )
                {
                    if (connection_result)
                        timer.cancel();

                    else if (timer_result){
                        error_handler(result::TIMEOUT,ErrorFormat("%s: %s:%s",
                                                          "Connection timeout", host.c_str(), port.c_str()));
                        if (use_ssl)
                            stream->next_layer().cancel();
                        else
                            socket->cancel();

                        return false;
                    }
                }

                if (*connection_result){
                    error_handler(result::FAIL,ErrorFormat("%s: %s:%s",
                                                   boost::system::system_error(*connection_result).what(), host.c_str(), port.c_str()));
                    return false;
                }

                return true;
            };

            // Set up an HTTP GET request message
            req.version(11);
            req.method(boost::beast::http::verb::post);
            req.target(target);
            req.set(boost::beast::http::field::host, host);
            req.set(boost::beast::http::field::user_agent, user_agent);
            req.set(boost::beast::http::field::content_type, "application/json");

            auto _data = body.dump(); //+ "\n";
            req.set(boost::beast::http::field::content_length, _data.size());
            req.body() = _data;
            req.prepare_payload();

            if (RpcSession::debug_on) {
                std::cerr << "\nDebug info: " << host << std::endl;
                std::cerr << req << std::endl;
                std::cerr << "..." << std::endl;
            }

            timer_setup();

            if (use_ssl) {
                boost::beast::http::async_write(*stream, req, [&connection_result](const boost::system::error_code& error, size_t){
                    connection_result = std::make_optional(error);
                });

            } else {
                boost::beast::http::async_write(*socket, req, [&connection_result](const boost::system::error_code& error, size_t){
                    connection_result = std::make_optional(error);
                });
            }

            if (!timeout_loop())
                return false;

            boost::beast::flat_buffer buffer;
            boost::beast::http::response<boost::beast::http::string_body> res;

            timer_setup();

            if (use_ssl) {

                boost::beast::http::async_read(*stream, buffer, res, [&connection_result](const boost::system::error_code& error, size_t){
                    connection_result = std::make_optional(error);
                });

            } else {
                boost::beast::http::async_read(*socket, buffer, res, [&connection_result](const boost::system::error_code& error, size_t){
                    connection_result = std::make_optional(error);
                });
            }

            if (!timeout_loop())
                return false;

            auto status = res.result();

            if (RpcSession::debug_on) {
                std::cerr << "\nResponse info: " << status << std::endl;
                std::cerr << res << std::endl;
                std::cerr << "..." << std::endl;
            }
            if (status == boost::beast::http::status::ok) {
                auto json = json::parse(res.body().data());
                if (json.count("result") > 0 && json.at("result") != nullptr){
                    return json["result"];
                }
            }

            response_fail_handler(status, body["method"],res);
            return std::nullopt;
        }
        catch(std::exception const& e)
        {
            error_handler(result::FAIL,ErrorFormat("json-rpc request: %s: %s:%s", e.what() , host.c_str(), port.c_str()));
            return std::nullopt;
        }
        catch(nlohmann::json::parse_error& e) {
            error_handler(milecsa::result::EXCEPTION, ErrorFormat("json-rpc request: parse error: %s", e.what()));
            return std::nullopt;
        }
        catch(nlohmann::json::invalid_iterator& e){
            error_handler(milecsa::result::EXCEPTION, ErrorFormat("json-rpc request: invalid iterator error: %s", e.what()));
            return std::nullopt;
        } catch(nlohmann::json::type_error & e){
            error_handler(milecsa::result::EXCEPTION, ErrorFormat("json-rpc request: type error: %s", e.what()));            return std::nullopt;
        } catch(nlohmann::json::out_of_range& e){
            error_handler(milecsa::result::EXCEPTION, ErrorFormat("json-rpc request: out of range error: %s", e.what()));
            return std::nullopt;
        } catch(nlohmann::json::other_error& e){
            error_handler(milecsa::result::EXCEPTION, ErrorFormat("json-rpc request: other error: %s", e.what()));
            return std::nullopt;
        }
        catch (...) {
            error_handler(milecsa::result::EXCEPTION, ErrorFormat("json-rpc request: unknown error"));
            return std::nullopt;
        }
    }

    rpc::request RpcSession::next_command(const std::string &method, const rpc::request &params) const {
        json command = {
                {"jsonrpc", "2.0"},
                {"method", method},
                {"params", params},
                {"id", ClientId::Instance().get_next()},
                {"version", 0.0}
        };
        return command;
    }
}