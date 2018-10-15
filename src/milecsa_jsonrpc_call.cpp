//
// Created by denn on 06/10/2018.
//

#include "milecsa_jsonrpc.hpp"
#include "milecsa_rpc_id.hpp"
#include "crypto_types.h"

namespace milecsa {

    namespace rpc {
        std::any Client::call(
                const std::string &method,
                const milecsa::rpc::request &params) const {

            if (method == "ping") {
                if (auto t = ping()) {
                    return std::any_cast<time_t>(*t);
                }
            }
            else if (method == "get-current-block-id") {
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
                if(StringToUInt256(sid, id, false)){
                    if (auto t = get_block(id)) {
                        return std::any_cast<rpc::response>(t);
                    }
                } else {
                    error_handler(result::FAIL,ErrorFormat(" %s could not convert to uint256_t", method.c_str()));
                }
            } else if (method == "get-wallet-state") {
                if(params.count("public-key")==0){
                    error_handler(result::NOT_FOUND,ErrorFormat("public key %s not defined", method.c_str()));
                    return std::nullopt;
                }
                if (auto t = get_wallet_state(params["public-key"])) {
                    return std::any_cast<rpc::response>(t);
                }
            } else if (method == "get-wallet-transactions") {
                if(params.count("public-key")==0){
                    error_handler(result::NOT_FOUND,ErrorFormat("public key %s not defined", method.c_str()));
                    return std::nullopt;
                }
                int limit = 1;
                if(params.count("limit")>0){
                    limit = params["limit"];
                }
                if (auto t = get_wallet_transactions(params["public-key"],limit)) {
                    return std::any_cast<rpc::response>(t);
                }
            } else if (
                    method == "send-transfer"
                    ||
                    method == "send-emission"
                    ) {

                if(params.count("to")==0){
                    error_handler(result::NOT_FOUND,ErrorFormat("destination public key %s not defined", method.c_str()));
                    return std::nullopt;
                }

                if(params.count("private-key")==0){
                    error_handler(result::NOT_FOUND,ErrorFormat("source private key %s not defined", method.c_str()));
                    return std::nullopt;
                }

                if(params.count("amount")==0){
                    error_handler(result::NOT_FOUND,ErrorFormat("amount %s not defined", method.c_str()));
                    return std::nullopt;
                }

                if(params.count("asset-code")==0){
                    error_handler(result::NOT_FOUND,ErrorFormat("asset-code %s not defined", method.c_str()));
                    return std::nullopt;
                }
				 
                std::string description;
                if(params.count("description")>0){
                    description = params["description"];
                }

                auto block_id = *get_current_block_id();

                auto ppk = milecsa::keys::Pair::FromPrivateKey(params["private-key"],error_handler);

                if (!ppk) {
                    return std::nullopt;
                }

                //using transaction = milecsa::transaction::Request<nlohmann::json>;

                using transfer = milecsa::transaction::Transfer<nlohmann::json>;
                using emission = milecsa::transaction::Emission<nlohmann::json>;

                unsigned short asset_code = params["asset-code"];
                uint64_t trx_id = rand();

                auto request = method == "send-transfer" ?

                               transfer::CreateRequest(
                                       *ppk,
                                       params["to"],
                                       block_id,   // block id
                                       trx_id,     // trx id
                                       asset_code, // asset code
                                       params["amount"],
                                       description,
                                       "",
                                       error_handler)->get_body() :

                               emission::CreateRequest(
                                       *ppk,
                                       params["to"],
                                       block_id,   // block id
                                       trx_id,     // trx id
                                       asset_code, // asset code
                                       params["amount"],
                                       description,
                                       "",
                                       error_handler)->get_body();

                if (request){

                    auto json_body = request->dump();

                    if(auto t = send_transaction(*ppk,*request)){
                        return std::any_cast<rpc::response>(t);
                    }
                }

            } else {
                error_handler(result::NOT_FOUND,ErrorFormat("Method %s not found", method.c_str()));
            }

            return std::nullopt;
        };
    }
}