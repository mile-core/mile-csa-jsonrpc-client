//
// Created by denn on 06/10/2018.
//

#include "milecsa_jsonrpc.hpp"
#include "milecsa_rpc_id.hpp"
#include "mile_crypto.h"

using emission = milecsa::transaction::JsonEmission;
using transfer = milecsa::transaction::JsonTransfer;
using node = milecsa::transaction::JsonNode;

namespace milecsa::rpc {

    static std::any transfer(const Client *client,
                             const std::string &method,
                             const milecsa::rpc::request &params,
                             const ErrorHandler &error_handler);

    static std::any emission(const Client *client,
                             const std::string &method,
                             const milecsa::rpc::request &params,
                             const ErrorHandler &error_handler);

    static std::any register_node(const Client *client,
                                  const std::string &method,
                                  const milecsa::rpc::request &params,
                                  const ErrorHandler &error_handler);

    static std::any unregister_node(const Client *client,
                                  const std::string &method,
                                  const milecsa::rpc::request &params,
                                  const ErrorHandler &error_handler);

    std::any Client::call(
            const std::string &method,
            const milecsa::rpc::request &params) const {

        try {

            static time_t _trx_counter = 0;
            srand(time(NULL) + _trx_counter++);

            uint64_t trx_id = rand();

            if (method == "ping") {
                if (auto t = ping()) {
                    return std::any_cast<time_t>(*t);
                }
            } else if (method == "get-current-block-id") {
                if (auto t = get_current_block_id()) {
                    return std::any_cast<uint256_t>(*t);
                }
            } else if (method == "get-network-state") {
                if (auto t = get_network_state()) {
                    return std::any_cast<rpc::response>(t);
                }
            } else if (method == "get-nodes") {
                if (auto t = get_nodes()) {
                    return std::any_cast<rpc::response>(t);
                }
            } else if (method == "get-blockchain-info") {
                if (auto t = get_blockchain_info()) {
                    return std::any_cast<rpc::response>(t);
                }
            } else if (method == "get-blockchain-state") {
                if (auto t = get_blockchain_state()) {
                    return std::any_cast<rpc::response>(t);
                }
            } else if (method == "get-block") {
                uint256_t id;
                std::string sid = params["id"];
                if (StringToUInt256(sid, id, false)) {
                    if (auto t = get_block(id)) {
                        return std::any_cast<rpc::response>(t);
                    }
                } else {
                    error_handler(result::FAIL, ErrorFormat(" %s could not convert to uint256_t", method.c_str()));
                }
            } else if (method == "get-wallet-state") {
                if (params.count("public-key") == 0) {
                    error_handler(result::NOT_FOUND, ErrorFormat("public key %s not defined", method.c_str()));
                    return std::nullopt;
                }
                if (auto t = get_wallet_state(params["public-key"])) {
                    return std::any_cast<rpc::response>(t);
                }
            } else if (method == "get-wallet-transactions") {
                if (params.count("public-key") == 0) {
                    error_handler(result::NOT_FOUND, ErrorFormat("public key %s not defined", method.c_str()));
                    return std::nullopt;
                }
                int limit = 1;
                if (params.count("limit") > 0) {
                    limit = params["limit"];
                }
                if (auto t = get_wallet_transactions(params["public-key"], limit)) {
                    return std::any_cast<rpc::response>(t);
                }
            } else if (method == "send-transfer") {

                return transfer(this, method, params, error_handler);

            } else if (method == "send-emission") {

                return emission(this, method, params, error_handler);

            } else if (method == "register-node") {

                return register_node(this, method, params, error_handler);

            }
            else if (method == "unregister-node") {

                return unregister_node(this, method, params, error_handler);

            }else {

                error_handler(result::NOT_FOUND, ErrorFormat("Method %s not found", method.c_str()));

            }
        }
        catch (nlohmann::json::parse_error &e) {
            error_handler(result::EXCEPTION, ErrorFormat("Parser error: %s, %s", method.c_str(), e.what()));
        }
        catch (nlohmann::json::invalid_iterator &e) {
            error_handler(result::EXCEPTION, ErrorFormat("Parser error: %s, %s", method.c_str(), e.what()));
        } catch (nlohmann::json::type_error &e) {
            error_handler(result::EXCEPTION, ErrorFormat("Parser error: %s, %s", method.c_str(), e.what()));
        } catch (nlohmann::json::out_of_range &e) {
            error_handler(result::EXCEPTION, ErrorFormat("Parser error: %s, %s", method.c_str(), e.what()));
        } catch (nlohmann::json::other_error &e) {
            error_handler(result::EXCEPTION, ErrorFormat("Parser error: %s, %s", method.c_str(), e.what()));
        }
        catch (...) {
            error_handler(result::EXCEPTION, ErrorFormat("Unknown parser error: %s", method.c_str()));
        }

        return std::any();
    }

    static inline uint64_t generate_trx_id() {
        static time_t _trx_counter = 0;
        srand(time(NULL) + _trx_counter++);
        return rand();
    }

    static inline std::optional<milecsa::keys::Pair> get_private(
            const std::string &method,
            const milecsa::rpc::request &params,
            const ErrorHandler &error_handler){

        if (params.count("private-key") == 0) {
            error_handler(result::NOT_FOUND, ErrorFormat("source private key %s not defined", method.c_str()));
            return std::nullopt;
        }
        return milecsa::keys::Pair::FromPrivateKey(params["private-key"], error_handler);
    }

    std::any transfer(const Client *client,
                      const std::string &method,
                      const milecsa::rpc::request &params,
                      const ErrorHandler &error_handler) {

        if (params.count("to") == 0) {
            error_handler(result::NOT_FOUND,
                          ErrorFormat("destination public key %s not defined", method.c_str()));
            return std::nullopt;
        }

        auto ppk = get_private(method,params,error_handler);
        if (!ppk)  return std::nullopt;

        if (params.count("amount") == 0) {
            error_handler(result::NOT_FOUND, ErrorFormat("amount %s not defined", method.c_str()));
            return std::nullopt;
        }
        float amount = params["amount"];

        if (params.count("asset-code") == 0) {
            error_handler(result::NOT_FOUND, ErrorFormat("asset-code %s not defined", method.c_str()));
            return std::nullopt;
        }
        unsigned short asset_code = params["asset-code"];

        std::string description;

        if (params.count("description") > 0) {
            description = params["description"];
        }

        auto block_id = *client->get_current_block_id();
        auto asset = milecsa::assets::TokenFromCode(asset_code);

        auto request = transfer::CreateRequest(
                *ppk,
                params["to"],
                block_id,            // block id
                generate_trx_id(),   // trx id
                asset,               // asset
                amount,
                0.0,
                description,
                error_handler)->get_body();

        if (request) {

            auto json_body = request->dump();

            if (auto t = client->send_transaction(*ppk, *request)) {
                return std::any_cast<rpc::response>(t);
            }
        }

        return std::any();
    }

    std::any emission(
            const Client *client,
            const std::string &method,
            const milecsa::rpc::request &params,
            const ErrorHandler &error_handler) {

        auto ppk = get_private(method,params,error_handler);
        if (!ppk)  return std::nullopt;

        if (params.count("asset-code") == 0) {
            error_handler(result::NOT_FOUND, ErrorFormat("asset-code %s not defined", method.c_str()));
            return std::nullopt;
        }
        unsigned short asset_code = params["asset-code"];

        auto block_id = *client->get_current_block_id();
        auto asset = milecsa::assets::TokenFromCode(asset_code);

        auto request = emission::CreateRequest(
                *ppk,
                block_id,          // block id
                generate_trx_id(), // trx id
                asset,             // asset code
                0.0,
                error_handler)->get_body();

        if (request) {

            auto json_body = request->dump();

            if (auto t = client->send_transaction(*ppk, *request)) {
                return std::any_cast<rpc::response>(t);
            }
        }

        return std::any();
    }

    std::any register_node(
            const Client *client,
            const std::string &method,
            const milecsa::rpc::request &params,
            const ErrorHandler &error_handler) {

        auto ppk = get_private(method,params,error_handler);
        if (!ppk)  return std::nullopt;

        if ( params.count("amount") == 0) {
            error_handler(result::NOT_FOUND, ErrorFormat("amount %s not defined", method.c_str()));
            return std::nullopt;
        }
        float amount = params["amount"];

        if (params.count("address") == 0) {
            error_handler(result::NOT_FOUND, ErrorFormat("address %s not defined", method.c_str()));
            return std::nullopt;
        }
        auto address = params["address"];

        auto block_id = *client->get_current_block_id();

        auto request = node::CreateRegisterRequest(
                *ppk,
                address,
                block_id,
                generate_trx_id(),
                amount,
                0.0,
                error_handler)->get_body();

        if (request) {

            auto json_body = request->dump();

            if (auto t = client->send_transaction(*ppk, *request)) {
                return std::any_cast<rpc::response>(t);
            }
        }

        return std::any();
    }

    std::any unregister_node(
            const Client *client,
            const std::string &method,
            const milecsa::rpc::request &params,
            const ErrorHandler &error_handler) {

        auto ppk = get_private(method,params,error_handler);
        if (!ppk)  return std::nullopt;

        auto block_id = *client->get_current_block_id();

        auto request = node::CreateUnregisterRequest(
                *ppk,
                block_id,
                generate_trx_id(),
                0,
                error_handler)->get_body();

        if (request) {

            auto json_body = request->dump();

            if (auto t = client->send_transaction(*ppk, *request)) {
                return std::any_cast<rpc::response>(t);
            }
        }

        return std::any();
    }
}
