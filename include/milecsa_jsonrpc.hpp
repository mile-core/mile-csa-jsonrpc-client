#ifndef MILE_JSONRPC_CSA_API_LIBRARY_H
#define MILE_JSONRPC_CSA_API_LIBRARY_H

#include <optional>
#include <functional>
#include <any>
#include <boost/asio.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/beast/http.hpp>

#include "json.hpp"
#include "milecsa.hpp"
#include "milecsa_jsonrpc.hpp"
#include "milecsa_url.hpp"

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

        namespace detail { class session; }

        /**
         * MILE Json-Rpc client
         */
        class Client {

        public:

            /**
             * Global debug option
             */
            static bool debug_on;

            /**
             * Create MILE json-rpc client controller
             * @param urlString - MILE node runs on json-rpcd mode
             * @param verify_ssl - if url contains https protocol it will enable SSL verification
             * @param error - connection error handler
             * @return options Client object
             */
            static std::optional<Client> Connect(
                    const std::string &urlString,
                    bool verify_ssl = true,
                    const ErrorHandler &error = default_error_handler);

            ~Client(){};

            /**
             * Get the current url
             * @return url
             */
            const Url &get_url() const { return *url_; }

            /**
             * Ping jsonrpc node service.
             * @param handler - response failure handler
             * @return interval between start request and response finish in microseconds
             */
            std::optional<time_t> ping(const http::ResponseHandler &handler = http::default_response_handler) const;

            /**
             * Get the last block id of the MILE network
             * @param handler - response failure handler
             * @return current block id, that is need to build any transactions
             */
            std::optional<uint256_t> get_current_block_id(const http::ResponseHandler &handler = http::default_response_handler) const;

            /**
             * Get blockchain network state
             * @param handler - response failure handler
             * @return count of consensuse nodes. responses object in nlohmann json style
             * @see https://github.com/nlohmann/json
             */
            response get_network_state(const http::ResponseHandler &handler = http::default_response_handler) const;

            /**
             * Get consensus node list
             * @param handler - response failure handler
             * @return  blockchain nodes info in nlohmann json style
             * @see https://github.com/nlohmann/json
             */
            response get_nodes(const http::ResponseHandler &handler = http::default_response_handler) const;

            /**
             * Get blockchain info
             * @param handler - response failure handler
             * @return blockchain info in nlohmann json style
             * @see https://github.com/nlohmann/json
             */
            response get_blockchain_info(const http::ResponseHandler &handler = http::default_response_handler) const;

            /**
             * Get the current blockchain state
             * @param handler - response failure handler
             * @return blockchain state structure in nlohmann json style
             * @see https://github.com/nlohmann/json
             */
            response get_blockchain_state(const http::ResponseHandler &handler = http::default_response_handler) const;

            /**
             * Get block by id
             * @param id - block id
             * @param handler - response failure handler
             * @return block structure in nlohmann json style
             * @see https://github.com/nlohmann/json
             */
            response get_block(uint256_t id, const http::ResponseHandler &handler = http::default_response_handler) const;

            /**
             * Get wallet state by public key
             * @param publicKey - wallet public key
             * @param handler - response failure handler
             * @return response object in nlohmann json style
             * @see https://github.com/nlohmann/json
             */
            response get_wallet_state(const std::string &publicKey,
                                      const http::ResponseHandler &handler = http::default_response_handler) const;

            /**
             * Get wallet last transactions
             * @param publicKey - wallet public key
             * @param limit - maximum transaction count
             * @param handler - response failure handler
             * @return response object in nlohmann json style
             * @see https://github.com/nlohmann/json
             */
            response get_wallet_transactions(const std::string &publicKey,
                                             const unsigned int limit = 1,
                                             const http::ResponseHandler &handler = http::default_response_handler) const;

            /**
             * Get wallet state
             * @param pair - wallet pair
             * @param handler - response failure handler
             * @return response object in nlohmann json style
             * @see https://github.com/nlohmann/json
             */
            response get_wallet_state(const milecsa::keys::Pair &pair,
                                      const http::ResponseHandler &handler = http::default_response_handler) const;

            /**
             * Get wallet last transactions
             * @param pair - wallet pair
             * @param limit - maximum transaction count
             * @param handler - response failure handler
             * @return response object in nlohmann json style
             * @see https://github.com/nlohmann/json
             */
            response get_wallet_transactions(const milecsa::keys::Pair &pair,
                                             const unsigned int limit = 1,
                                             const http::ResponseHandler &handler = http::default_response_handler) const;

            /**
             * Send user defined transaction. Transaction can be prepared by milecsa::transaction::Request<nlohmann::json>
             * @param pair - wallet pair
             * @param transactionData - transaction data
             * @param handler - response failure handler
             * @return response object in nlohmann json style
             * @see https://github.com/nlohmann/json
             * @see milecsa::transaction::Request<nlohmann::json>
             */
            response send_transaction(const milecsa::keys::Pair &pair,
                                      json transactionData,
                                      const http::ResponseHandler &handler = http::default_response_handler) const;
            /**
             * Run rpc method by name with parameters
             * @param method - method name
             * @param params - json-rpc params
             * @param handler - response failure handler
             * @return rpc object respons
             */
            std::any call(const std::string &method,
                          const request &params,
                          const http::ResponseHandler &handler = http::default_response_handler,
                          const ErrorHandler &error = default_error_handler) const;

        private:
            Client(const Url &url, bool verify_ssl);
            Client():verify_ssl_(true){};

            json next_command(const std::string &method) const;

            const std::optional<Url> url_;

            bool verify_ssl_;
            std::shared_ptr<detail::session> session;

            static const json jsonrpc_core_;
        };
    }
}

#endif