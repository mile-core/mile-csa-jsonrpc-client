
#include "milecsa_jsonrpc.hpp"
#include "milecsa_rpc_id.hpp"
#include "milecsa_rpc_session.hpp"

namespace milecsa::rpc {

    time_t Client::timeout = 3;

    using namespace boost::asio::ip;
    namespace ssl = boost::asio::ssl;

    Client& Client::operator=(const Client& client) {
        url_ = client.url_;
        verify_ssl_=client.verify_ssl_;
        session=client.session;
        response_fail_handler=client.response_fail_handler;
        error_handler=client.error_handler;
        return *this;
    }

    Client::Client(const milecsa::rpc::Client &client):
            url_(client.url_),
            verify_ssl_(client.verify_ssl_),
            session(client.session),
            response_fail_handler(client.response_fail_handler),
            error_handler(client.error_handler){}

    std::optional<Client> Client::Connect(
            const std::string &urlString,
            bool verify_ssl,
            const http::ResponseHandler &response_fail_handler,
            const milecsa::ErrorHandler &error_handler) {
        if(auto url = Url::Parse(urlString, error_handler)){

            auto client = Client(*url, verify_ssl, response_fail_handler, error_handler);

            if (!client.session->connect(error_handler)) {
                return std::nullopt;
            }

            return client;
        }
        return std::nullopt;
    }

    Client::Client(
            const milecsa::rpc::Url &url,
            bool verify_ssl,
            const http::ResponseHandler &response_handler,
            const ErrorHandler &error_handler) :
            url_(url),
            verify_ssl_(verify_ssl),
            response_fail_handler(response_handler),
            error_handler(error_handler)
    {

        session = std::shared_ptr<detail::RpcSession>(
                new detail::RpcSession(
                        url_->get_host(),
                        url_->get_port(),
                        url_->get_path(),
                        url_->get_protocol(),
                        verify_ssl,
                        Client::timeout));
    }

    Client::~Client(){
        session.reset();
    };


    std::optional<time_t> Client::ping() const {
        auto start = std::chrono::high_resolution_clock::now();
        if(auto res = session->request(session->next_command("ping"),response_fail_handler,error_handler)) {
            if (*res == true || *res == "true") {
                auto elapsed = std::chrono::high_resolution_clock::now() - start;
                return std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
            }
            return -1;
        }
        return std::nullopt;
    }

    std::optional<uint256_t> Client::get_current_block_id() const {
        if(auto json = session->request(session->next_command("get-current-block-id"),response_fail_handler,error_handler)){
            auto result = *json;

            if (result.empty()) {
                return std::nullopt;
            }

            uint256_t block_id;
            StringToUInt256(result["current-block-id"], block_id, false);

            return block_id;
        }
        return std::nullopt;
    }

    rpc::response Client::get_network_state() const {
        return session->request(session->next_command("get-network-state"),response_fail_handler,error_handler);
    }

    rpc::response Client::get_nodes() const {
        return session->request(session->next_command("get-nodes"),response_fail_handler,error_handler);
    }

    rpc::response Client::get_blockchain_info() const {
        return session->request(session->next_command("get-blockchain-info"),response_fail_handler,error_handler);
    }

    rpc::response Client::get_blockchain_state() const {
        return session->request(session->next_command("get-blockchain-state"),response_fail_handler,error_handler);
    }

    rpc::response Client::get_block(uint256_t id) const {
        json command = session->next_command("get-block-by-id", {{"id", id}});
        return session->request(command,response_fail_handler,error_handler);
    }

    rpc::response Client::get_wallet_state(const std::string &publicKey) const {
        json command = session->next_command("get-wallet-state",{{"public-key", publicKey}});
        return session->request(command,response_fail_handler,error_handler);
    }

    rpc::response Client::get_wallet_transactions(const std::string &publicKey,
                                                  const unsigned int limit) const {

        json command = session->next_command("get-wallet-transactions",{{"public-key", publicKey}, {"limit", limit}});
        return session->request(command,response_fail_handler,error_handler);
    }


    rpc::response Client::get_wallet_state(const milecsa::keys::Pair &pair) const {
        return get_wallet_state(pair.get_public_key().encode());
    }

    rpc::response Client::get_wallet_transactions(const milecsa::keys::Pair &pair, const unsigned int limit) const {
        return get_wallet_transactions(pair.get_public_key().encode(),limit);
    }

    rpc::response Client::send_transaction(const milecsa::keys::Pair &pair,
                                           milecsa::rpc::json transactionData) const {
        json command = session->next_command("send-transaction");
        command["params"] = transactionData;
        return session->request(command,response_fail_handler,error_handler);
    }
}