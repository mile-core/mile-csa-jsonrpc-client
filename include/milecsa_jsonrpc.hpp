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
#include "json.hpp"

namespace milecsa {

    namespace rpc {

        namespace detail { class RpcSession; }

        /**
         * MILE Json-Rpc client
         */
        class Client {

        public:

            static time_t timeout;

            /**
             * Create MILE json-rpc client controller
             * @param urlString - MILE node runs on json-rpcd mode
             * @param verify_ssl - if url contains https protocol it will enable SSL verification
             * @param response_fail_handler - response fail handler
             * @param error_handler - connection error handler
             * @return options Client object
             */
            static std::optional<Client> Connect(
                    const std::string &urlString,
                    bool verify_ssl = true,
                    const http::ResponseHandler &response_fail_handler = http::default_response_handler,
                    const ErrorHandler &error_handler = default_error_handler);

            Client(const Client &client);

            ~Client();

            /**
             * Get the current url
             * @return url
             */
            const Url &get_url() const { return *url_; }

            /**
             * Ping jsonrpc node service.
             * @return interval between start request and response finish in microseconds
             */
            std::optional<time_t> ping() const;

            /**
             * Get the last block id of the MILE network
             * @return current block id, that is need to build any transactions
             */
            std::optional<uint256_t> get_current_block_id() const;

            /**
             * Get blockchain network state
             * @return count of consensuse nodes. responses object in nlohmann json style
             * @see https://github.com/nlohmann/json
             */
            response get_network_state() const;

            /**
             * Get consensus node list
             * @return  blockchain nodes info in nlohmann json style
             * @see https://github.com/nlohmann/json
             */
            response get_nodes() const;

            /**
             * Get blockchain info
             * @return blockchain info in nlohmann json style
             * @see https://github.com/nlohmann/json
             */
            response get_blockchain_info() const;

            /**
             * Get the current blockchain state
             * @return blockchain state structure in nlohmann json style
             * @see https://github.com/nlohmann/json
             */
            response get_blockchain_state() const;

            /**
             * Get block by id
             * @param id - block id
             * @return block structure in nlohmann json style
             * @see https://github.com/nlohmann/json
             */
            response get_block(uint256_t id) const;

            /**
             * Get wallet state by public key
             * @param publicKey - wallet public key
             * @return response object in nlohmann json style
             * @see https://github.com/nlohmann/json
             */
            response get_wallet_state(const std::string &publicKey) const;

            /**
             * Get wallet last transactions
             * @param publicKey - wallet public key
             * @param limit - maximum transaction count
             * @return response object in nlohmann json style
             * @see https://github.com/nlohmann/json
             */
            response get_wallet_transactions(const std::string &publicKey,
                                             const unsigned int limit = 1) const;

            /**
             * Get wallet state
             * @param pair - wallet pair
             * @return response object in nlohmann json style
             * @see https://github.com/nlohmann/json
             */
            response get_wallet_state(const milecsa::keys::Pair &pair) const;

            /**
             * Get wallet last transactions
             * @param pair - wallet pair
             * @param limit - maximum transaction count
             * @return response object in nlohmann json style
             * @see https://github.com/nlohmann/json
             */
            response get_wallet_transactions(const milecsa::keys::Pair &pair,
                                             const unsigned int limit = 1) const;

            /**
             * Send user defined transaction. Transaction can be prepared by milecsa::transaction::Request<nlohmann::json>
             * @param pair - wallet pair
             * @param transactionData - transaction data
             * @return response object in nlohmann json style
             * @see https://github.com/nlohmann/json
             * @see milecsa::transaction::Request<nlohmann::json>
             */
            response send_transaction(const milecsa::keys::Pair &pair,
                                      json transactionData) const;
            /**
             * Run rpc method by name with parameters
             * @param method - method name
             * @param params - json-rpc params
             * @return rpc object respons
             */
            std::any call(const std::string &method,
                          const request &params) const;

            Client& operator=(const Client&);

        private:

            Client(const Url &url,
                   bool verify_ssl,
                   const http::ResponseHandler &response_handler,
                   const ErrorHandler &error_handler);

            Client():verify_ssl_(true),
                     response_fail_handler(http::default_response_handler),
                     error_handler(default_error_handler){};

            std::optional<Url> url_;
            bool verify_ssl_;
            std::shared_ptr<detail::RpcSession> session;

            http::ResponseHandler response_fail_handler;
            ErrorHandler error_handler;
        };
    }
}
