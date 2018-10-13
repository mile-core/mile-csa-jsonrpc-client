//
// Created by denn on 06/10/2018.
//

#define BOOST_TEST_MODULE requests

#include "crypto_types.h"
#include "milecsa_jsonrpc.hpp"

#include <optional>
#include <boost/test/included/unit_test.hpp>

struct RequestsEval {

    using Url = milecsa::rpc::Url;

    milecsa::ErrorHandler error = [](milecsa::result code, std::string error){
        BOOST_TEST_MESSAGE("Request Error: " + error);
    };

    milecsa::http::ResponseHandler response = [&](const milecsa::http::response &http){
        std::cerr << "Response error: " << http.result() << std::endl << http << std::endl;
    };

    bool getzeroblock(const std::string &u = "http://node002.testnet.mile.global/v1/api") {

        uint256_t start_position_1(0);
        auto client_tt = milecsa::rpc::Client::Connect(u,false);
        milecsa::rpc::response r = client_tt->get_block(start_position_1);

        BOOST_TEST_MESSAGE(" -- ");
        BOOST_TEST_MESSAGE("Zero Block: " + r->dump());

        return true;
    }

    bool test(const std::string &u = "http://node002.testnet.mile.global/v1/api") {
        if (auto rpc = milecsa::rpc::Client::Connect(u, true, error)) {

            BOOST_TEST_MESSAGE("Ping : " + StringFormat("%l", *rpc->ping(response, error)));

            auto last_block_id = *rpc->get_current_block_id(response);
            BOOST_TEST_MESSAGE(" -- ");
            BOOST_TEST_MESSAGE("Id   : " + UInt256ToDecString(last_block_id));

            BOOST_TEST_MESSAGE(" -- ");
            BOOST_TEST_MESSAGE("Info : " + rpc->get_blockchain_info(response, error)->dump());

            BOOST_TEST_MESSAGE(" -- ");
            BOOST_TEST_MESSAGE("State: " + rpc->get_blockchain_state(response, error)->dump());

            auto pk = "EUjuoTty9oHdF8h7ab4u3KCCci5dduFxvJbqAx5qXUUtk2Wnx";
            BOOST_TEST_MESSAGE(" -- ");
            BOOST_TEST_MESSAGE("Wallet: " + rpc->get_wallet_state(pk, response, error)->dump());

            BOOST_TEST_MESSAGE(" -- ");
            BOOST_TEST_MESSAGE("Trxs  : " + rpc->get_wallet_transactions(pk, 5, response, error)->dump());

            BOOST_TEST_MESSAGE(" -- ");
            BOOST_TEST_MESSAGE("NW State: " + rpc->get_network_state(response, error)->dump());

            BOOST_TEST_MESSAGE(" -- ");
            BOOST_TEST_MESSAGE("NW Nodes: " + rpc->get_nodes(response, error)->dump());

            BOOST_TEST_MESSAGE(" -- ");
            BOOST_TEST_MESSAGE("Block: " + rpc->get_block(last_block_id, response, error)->dump());

            return true;
        }
        return false;
    }
};


BOOST_FIXTURE_TEST_CASE( requests, RequestsEval )
{
    BOOST_CHECK(test());
    BOOST_CHECK(getzeroblock());
}
