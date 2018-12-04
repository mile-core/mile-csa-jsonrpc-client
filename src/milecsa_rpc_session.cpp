//
// Created by denn on 15/10/2018.
//

#include "milecsa_rpc_session.hpp"

#include <optional>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

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

            socket(0),
            stream(0),
            deadline(ioc){
        prepare();
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

        wait_deadline();

        return  socket != nullptr || stream != nullptr;
    }

    void RpcSession::wait_deadline(){

        if (deadline.expires_at() <= deadline_timer::traits_type::now())
        {
            boost::system::error_code ignored_ec;

            if (use_ssl && stream)
                stream->next_layer().close(ignored_ec);
            else if (socket) {
                socket->close(ignored_ec);
            }

            deadline.expires_at(boost::posix_time::pos_infin);
        }
        deadline.async_wait(boost::lambda::bind(&RpcSession::wait_deadline, this));
    }

    bool RpcSession::check_socket(){

        if (use_ssl && stream)
            return  stream->next_layer().is_open();
        else if (socket)
            return socket->is_open();

        return false;
    }

    bool RpcSession::connect(const milecsa::ErrorHandler &error) {

        try {
            auto const results = tcp::resolver(ioc).resolve(host, port);

            if (results.empty()) {
                error(result::NOT_FOUND,ErrorFormat("Host %s:%s not found", host.c_str(), port.c_str()));
                return false;
            }

            boost::posix_time::time_duration tm = boost::posix_time::seconds(timeout);

            deadline.expires_from_now(tm);

            boost::system::error_code ec = boost::asio::error::would_block;
            std::string stage = "Connection timeout";

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

                    boost::asio::async_connect(
                            stream->next_layer(),
                            results, boost::lambda::var(ec) = boost::lambda::_1);

                    do ioc.run_one(); while (ec == boost::asio::error::would_block);

                    if (ec || !check_socket()) {
                        error(result::TIMEOUT, ErrorFormat("%s %s: %s:%s",
                                                           stage.c_str(),
                                                           boost::system::system_error(
                                                                   ec ? ec : boost::asio::error::operation_aborted).what(),
                                                           host.c_str(), port.c_str()));
                        return false;
                    }

                    stage = "SSL Handshake timeout";

                    deadline.expires_from_now(tm);
                    ec = boost::asio::error::would_block;

                    stream->async_handshake(ssl::stream_base::client, boost::lambda::var(ec) = boost::lambda::_1);

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
                boost::asio::async_connect(
                        *socket,
                        results, boost::lambda::var(ec) = boost::lambda::_1);
            }

            do ioc.run_one(); while (ec == boost::asio::error::would_block);

            if (ec || !check_socket()) {
                error(result::TIMEOUT, ErrorFormat("%s %s: %s:%s",
                                                   stage.c_str(),
                                                   boost::system::system_error(
                                                           ec ? ec : boost::asio::error::operation_aborted).what(),
                                                   host.c_str(), port.c_str()));
                return false;
            }
        }
        catch (std::exception const& e) {
            error(result::FAIL,ErrorFormat("%s: %s:%s", e.what(), host.c_str(), port.c_str()));
            return false;
        }
        catch (...) {
            error(result::FAIL,ErrorFormat("Unknown error: %s:%s", host.c_str(), port.c_str()));
            return false;
        }

        return true;
    }


    RpcSession::~RpcSession(){

        if(socket) {
            boost::system::error_code ec;
            socket->shutdown(tcp::socket::shutdown_both, ec);
            delete socket;
            socket = 0;
        }
        if (stream) {
            boost::system::error_code ec;
            stream->shutdown(ec);
            delete stream;
            stream = 0;
        }
        ioc.stop();
    }

    rpc::response RpcSession::request(const rpc::request &body,
                                      const http::ResponseHandler &response_fail_handler,
                                      const milecsa::ErrorHandler &error_handler) {

        boost::beast::http::request<boost::beast::http::string_body> req;

        try {

            boost::posix_time::time_duration tm = boost::posix_time::seconds(timeout);
            boost::system::error_code ec = boost::asio::error::would_block;

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

            boost::beast::flat_buffer buffer;
            boost::beast::http::response<boost::beast::http::string_body> res;

            deadline.expires_from_now(tm);
            ec = boost::asio::error::would_block;

            if (use_ssl) {
                boost::beast::http::async_read(*stream, buffer, res, [&ec](const boost::system::error_code& error, size_t){
                    ec = error;
                });
            } else {
                boost::beast::http::async_read(*socket, buffer, res,  [&ec](const boost::system::error_code& error, size_t){
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

            auto status = res.result();

            if (RpcSession::debug_on) {
                std::cerr << "\nResponse info: " << status << std::endl;
                std::cerr << res << std::endl;
                std::cerr << " ------- " << std::endl;
                std::cerr << res.body() << std::endl;
                std::cerr << " ------- " << std::endl;
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