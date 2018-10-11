#include "milecsa_jsonrpc.hpp"
#include "milecsa_id_counter.hpp"
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
    namespace rpc {

        bool Client::debug_on = false;

        const json Client::jsonrpc_core_ = {
                {"jsonrpc", "2.0"},
                {"method", "ping"},
                {"params", {}},
                {"id", 0},
                {"version", 0.0}
        };

        using namespace boost::asio::ip;
        namespace ssl = boost::asio::ssl;

        typedef milecsa::rpc::IdCounter<uint64_t> ClientId;

        namespace detail {
            class session{

            public:

                session(const std::string &host,
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

                ~session(){
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
                                 const http::ResponseHandler &handler = http::default_response_handler) {

                    boost::beast::http::request<boost::beast::http::string_body> req;

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

                    if (Client::debug_on) {
                        std::cerr << "\nDebug info: "<< host << std::endl;
                        std::cerr << req << std::endl;
                        std::cerr << "..." << std::endl;
                    }

                    if (use_ssl){
                        boost::beast::http::write(*stream, req);
                    }
                    else {
                        boost::beast::http::write(*socket, req);
                    }

                    boost::beast::flat_buffer buffer;
                    boost::beast::http::response<boost::beast::http::string_body> res;

                    if (use_ssl){
                        boost::beast::http::read(*stream, buffer, res);
                    }
                    else {
                        boost::beast::http::read(*socket, buffer, res);
                    }

                    auto status = res.result();

                    if (Client::debug_on) {
                        std::cerr << "\nResponse info: "<< status << std::endl;
                        std::cerr << res << std::endl;
                        std::cerr << "..." << std::endl;
                    }
                    if (status == boost::beast::http::status::ok){
                        auto json = json::parse(res.body().data());
                        if(json.count("result")>0)
                            return json;
                    }

                    handler(res);
                    return std::nullopt;
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

        std::optional<Client> Client::Connect(const std::string &urlString, bool verify_ssl, const milecsa::ErrorHandler &error) {
            if(auto url = Url::Parse(urlString, error)){

                auto client = Client(*url, verify_ssl);

                if (!client.session->connect(error)) {
                    return std::nullopt;
                }

                return client;
            }
            return std::nullopt;
        }

        Client::Client(const milecsa::rpc::Url &url, bool verify_ssl) : url_(url), verify_ssl_(verify_ssl) {
            session = std::shared_ptr<detail::session>(
                    new detail::session(
                            url_->get_host(),
                            url_->get_port(),
                            url_->get_path(),
                            url_->get_protocol(),
                            verify_ssl)
            );
        }

        json Client::next_command(const std::string &method) const {
            json command = Client::jsonrpc_core_;
            command["method"] = method;
            command["id"] = ClientId::Instance().get_next();
            return command;
        }

        std::optional<time_t> Client::ping(const http::ResponseHandler &handler) const {
            auto start = std::chrono::high_resolution_clock::now();
            if(auto res = session->request(next_command("ping"),handler)) {
                if (res->at("result") == true || res->at("result") == "true") {
                    auto elapsed = std::chrono::high_resolution_clock::now() - start;
                    return std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
                }
                return -1;
            }
            return std::nullopt;
        }

        std::optional<uint256_t> Client::get_current_block_id(const http::ResponseHandler &handler) const {
            if(auto json = session->request(next_command("get-current-block-id"),handler)){
                auto result = json->at("result");

                if (result.empty()) {
                    return std::nullopt;
                }

                uint256_t block_id;
                StringToUInt256(result["current-block-id"], block_id, false);

                return block_id;
            }
            return std::nullopt;
        }

        rpc::response Client::get_network_state(const http::ResponseHandler &handler) const {
            return session->request(next_command("get-network-state"),handler);
        }

        rpc::response Client::get_nodes(const http::ResponseHandler &handler) const {
            return session->request(next_command("get-nodes"),handler);
        }

        rpc::response Client::get_blockchain_info(const http::ResponseHandler &handler) const {
            return session->request(next_command("get-blockchain-info"),handler);
        }

        rpc::response Client::get_blockchain_state(const http::ResponseHandler &handler) const {
            return session->request(next_command("get-blockchain-state"),handler);
        }

        rpc::response Client::get_block(uint256_t id, const http::ResponseHandler &handler) const {
            json command = next_command("get-block-by-id");
            command["params"] = {{"id", id}};
            return session->request(command,handler);
        }

        rpc::response Client::get_wallet_state(const std::string &publicKey,
                                          const http::ResponseHandler &handler) const {
            json command = next_command("get-wallet-state");
            command["params"] = {{"public-key", publicKey}};
            return session->request(command,handler);
        }

        rpc::response Client::get_wallet_transactions(const std::string &publicKey,
                                                 const unsigned int limit,
                                                 const http::ResponseHandler &handler) const {

            json command = next_command("get-wallet-transactions");
            command["params"] = {{"public-key", publicKey}, {"limit", limit}};

            return session->request(command,handler);
        }


        rpc::response Client::get_wallet_state(const milecsa::keys::Pair &pair,
                                          const http::ResponseHandler &handler) const {
            return get_wallet_state(pair.get_public_key().encode(),handler);
        }

        rpc::response Client::get_wallet_transactions(const milecsa::keys::Pair &pair, const unsigned int limit,
                                                 const http::ResponseHandler &handler) const {
            return get_wallet_transactions(pair.get_public_key().encode(),limit,handler);
        }

        rpc::response Client::send_transaction(const milecsa::keys::Pair &pair,
                                                 milecsa::rpc::json transactionData,
                                                 const http::ResponseHandler &handler) const {
            json command = next_command("send-transaction");
            command["params"] = transactionData;
            return session->request(command,handler);
        }
    }
}