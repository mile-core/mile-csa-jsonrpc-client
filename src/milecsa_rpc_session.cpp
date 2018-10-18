//
// Created by denn on 15/10/2018.
//

#include "milecsa_rpc_session.hpp"

namespace milecsa::rpc::detail {

    RpcSession::RpcSession(const std::string &host,
               uint64_t port,
               const std::string &target,
               Url::protocol protocol,
               bool verify):

            use_ssl(protocol == Url::protocol::https),
            verify_ssl(verify),

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
        }
        catch (std::exception const& e) {
            error(result::FAIL,ErrorFormat(e.what()));
            return false;
        }
        
        return true;
    }


    RpcSession::~RpcSession(){
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

    rpc::response RpcSession::request(const rpc::request &body,
                          const http::ResponseHandler &response_fail_handler,
                          const milecsa::ErrorHandler &error_handler) {

        boost::beast::http::request<boost::beast::http::string_body> req;

        try {
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

            response_fail_handler(status, body["method"],res);
            return std::nullopt;
        }
        catch(std::exception const& e)
        {
            error_handler(result::FAIL,ErrorFormat(e.what()));
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