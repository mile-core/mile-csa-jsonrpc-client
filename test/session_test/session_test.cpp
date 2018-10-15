//
// Created by lotus mile on 13/10/2018.
//

#define BOOST_TEST_MODULE rpc_session

#include "crypto_types.h"
#include "milecsa_jsonrpc.hpp"

#include <optional>
#include <fstream>
#include <boost/test/included/unit_test.hpp>

static const nlohmann::json tests = R"([
  {"method": "help",                     "params": {} },
  {"method": "get-block-history-state",  "params": {} },
  {"method": "get-block-history",        "params": {"first-id":0, "limit":3}},
  {"method": "get-block",                "params": {"id":2}},
  {"method": "get-network-state",        "params": {}},
  {"method": "get-nodes",                "params": {"first-id":0, "limit":3}},
  {"method": "get-transaction-history-state","params":{}},
  {"method": "get-transaction-history",  "params": {"first-id":100,"limit":10}},
  {"method": "get-transaction-info",     "params": {"public-key":"111111111111111111111111111111115RyRTN", "id":10}},
  {"method": "get-wallet-history-state", "params": {"public-key":"2cv1HqiCE8BeHc4Y7vucckMysx3PV7KND2mQiMdXqDJg812Fd5"}},
  {"method": "get-wallet-history-blocks","params": {"public-key":"zVG4iPaggWUUaDEkyEyFBv8dNYSaFMm2C7WS8nSMKWLsSh9x", "first-id": 0, "limit": 2}},
  {"method": "get-wallet-history-transactions", "params": {"public-key":"2cv1HqiCE8BeHc4Y7vucckMysx3PV7KND2mQiMdXqDJg812Fd5", "first-id": 0, "limit": 3}}
])"_json;

struct SessionEval {

    using Url = milecsa::rpc::Url;

    milecsa::ErrorHandler error = [](milecsa::result code, const std::string &error){
        BOOST_TEST_MESSAGE("Session Error: " + error);
    };

    milecsa::http::ResponseHandler response_handler = [&](const milecsa::http::status code, const std::string &method, const milecsa::http::response &http){
        BOOST_TEST_MESSAGE("Response Error: ");
        BOOST_TEST_MESSAGE("       Calling: " + method);
        BOOST_TEST_MESSAGE(http.result());
        BOOST_TEST_MESSAGE(http);
    };

    std::shared_ptr<milecsa::rpc::detail::RpcSession> get_session(const std::string &u){

       // milecsa::rpc::detail::RpcSession::debug_on = true;

        if (auto url = Url::Parse(u,error)){

            auto session = std::shared_ptr<milecsa::rpc::detail::RpcSession>(
                    new milecsa::rpc::detail::RpcSession(
                            url->get_host(),
                            url->get_port(),
                            url->get_path(),
                            url->get_protocol(),
                            false)
            );

            session->connect(error);

            return session;

        }

        return nullptr;
    }

    bool run_tests(const std::string &input_method="", const std::string &u = "http://127.0.0.1:9998/v1/api"){
        try {
            if (tests.is_discarded()){
                return false;
            }

            for (auto& element : tests) {
                std::string method = element["method"];

                if (!input_method.empty() && method!=input_method)
                    continue;

                nlohmann::json params = element["params"];

                if (auto session = get_session(u)){

                    auto command = session->next_command(method, params);
                    auto respons = session->request(command, response_handler, error);

                    if (!respons) {
                        return false;
                    }

                    BOOST_TEST_MESSAGE(" -- ");
                    BOOST_TEST_MESSAGE(method + ": " + respons->dump());
                }
                else {
                    return false;
                }
            }
        }
        catch (nlohmann::detail::parse_error &e) {
            std::cerr<< "Parser error: " << e.what() << std::endl;
            return false;
        }

        return true;
    }
};


BOOST_FIXTURE_TEST_CASE( help, SessionEval )
{
    BOOST_CHECK(run_tests("help"));
}

BOOST_FIXTURE_TEST_CASE( get_block_history_state, SessionEval )
{
    BOOST_CHECK(run_tests("get-block-history-state"));
}

BOOST_FIXTURE_TEST_CASE( get_block_history, SessionEval )
{
    BOOST_CHECK(run_tests("get-block-history"));
}

BOOST_FIXTURE_TEST_CASE( get_block, SessionEval )
{
    BOOST_CHECK(run_tests("get-block"));
}

BOOST_FIXTURE_TEST_CASE( get_network_state, SessionEval )
{
    BOOST_CHECK(run_tests("get-network-state"));
}

BOOST_FIXTURE_TEST_CASE( get_nodes, SessionEval )
{
    BOOST_CHECK(run_tests("get-nodes"));
}

BOOST_FIXTURE_TEST_CASE( get_transaction_history_state, SessionEval )
{
    BOOST_CHECK(run_tests("get-transaction-history-state"));
}

BOOST_FIXTURE_TEST_CASE( get_transaction_history, SessionEval )
{
    BOOST_CHECK(run_tests("get-transaction-history"));
}

BOOST_FIXTURE_TEST_CASE( get_transaction_info, SessionEval )
{
    BOOST_CHECK(run_tests("get-transaction-info"));
}

BOOST_FIXTURE_TEST_CASE( get_wallet_history_state, SessionEval )
{
    BOOST_CHECK(run_tests("get-wallet-history-state"));
}

BOOST_FIXTURE_TEST_CASE( get_wallet_history_blocks, SessionEval )
{
    BOOST_CHECK(run_tests("get-wallet-history-blocks"));
}

BOOST_FIXTURE_TEST_CASE( get_wallet_history_transactions, SessionEval )
{
    BOOST_CHECK(run_tests("get-wallet-history-transactions"));
}