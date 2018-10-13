#include "milecsa_jsonrpc.hpp"
#include "milecsa_rpc_id.hpp"
#include "crypto_types.h"

#include "milecsa_rpc_session.hpp"

namespace milecsa {
    namespace rpc {

        bool detail::RpcSession::debug_on = false;

        using namespace boost::asio::ip;
        namespace ssl = boost::asio::ssl;

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
            session = std::shared_ptr<detail::RpcSession>(
                    new detail::RpcSession(
                            url_->get_host(),
                            url_->get_port(),
                            url_->get_path(),
                            url_->get_protocol(),
                            verify_ssl)
            );
        }

        std::optional<time_t> Client::ping(const http::ResponseHandler &handler, const ErrorHandler &error) const {
            auto start = std::chrono::high_resolution_clock::now();
            if(auto res = session->request(session->next_command("ping"),handler,error)) {
                if (res->at("result") == true || res->at("result") == "true") {
                    auto elapsed = std::chrono::high_resolution_clock::now() - start;
                    return std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
                }
                return -1;
            }
            return std::nullopt;
        }

        std::optional<uint256_t> Client::get_current_block_id(const http::ResponseHandler &handler, const ErrorHandler &error) const {
            if(auto json = session->request(session->next_command("get-current-block-id"),handler,error)){
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

        rpc::response Client::get_network_state(const http::ResponseHandler &handler, const ErrorHandler &error) const {
            return session->request(session->next_command("get-network-state"),handler,error);
        }

        rpc::response Client::get_nodes(const http::ResponseHandler &handler, const ErrorHandler &error) const {
            return session->request(session->next_command("get-nodes"),handler,error);
        }

        rpc::response Client::get_blockchain_info(const http::ResponseHandler &handler, const ErrorHandler &error) const {
            return session->request(session->next_command("get-blockchain-info"),handler,error);
        }

        rpc::response Client::get_blockchain_state(const http::ResponseHandler &handler, const ErrorHandler &error) const {
            return session->request(session->next_command("get-blockchain-state"),handler,error);
        }

        rpc::response Client::get_block(uint256_t id, const http::ResponseHandler &handler, const ErrorHandler &error) const {
            json command = session->next_command("get-block-by-id", {{"id", id}});
            return session->request(command,handler,error);
        }

        rpc::response Client::get_wallet_state(const std::string &publicKey,
                                          const http::ResponseHandler &handler, const ErrorHandler &error) const {
            json command = session->next_command("get-wallet-state",{{"public-key", publicKey}});
            return session->request(command,handler,error);
        }

        rpc::response Client::get_wallet_transactions(const std::string &publicKey,
                                                 const unsigned int limit,
                                                 const http::ResponseHandler &handler, const ErrorHandler &error) const {

            json command = session->next_command("get-wallet-transactions",{{"public-key", publicKey}, {"limit", limit}});
            return session->request(command,handler,error);
        }


        rpc::response Client::get_wallet_state(const milecsa::keys::Pair &pair,
                                          const http::ResponseHandler &handler, const ErrorHandler &error) const {
            return get_wallet_state(pair.get_public_key().encode(),handler,error);
        }

        rpc::response Client::get_wallet_transactions(const milecsa::keys::Pair &pair, const unsigned int limit,
                                                 const http::ResponseHandler &handler, const ErrorHandler &error) const {
            return get_wallet_transactions(pair.get_public_key().encode(),limit,handler);
        }

        rpc::response Client::send_transaction(const milecsa::keys::Pair &pair,
                                                 milecsa::rpc::json transactionData,
                                                 const http::ResponseHandler &handler,
                                                 const ErrorHandler &error) const {
            json command = session->next_command("send-transaction");
            command["params"] = transactionData;
            return session->request(command,handler,error);
        }
    }
}