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
  {"method": "get-transaction-info",     "params": {"public-key":"111111111111111111111111111111115RyRTN", "id":10}},
  {"method": "get-wallet-history-blocks","params": {"public-key":"zVG4iPaggWUUaDEkyEyFBv8dNYSaFMm2C7WS8nSMKWLsSh9x", "first-id": 0, "limit": 2}},
  {"method": "get-wallet-history-state", "params": {"public-key":"2cv1HqiCE8BeHc4Y7vucckMysx3PV7KND2mQiMdXqDJg812Fd5"}},
  {"method": "get-wallet-history-transactions", "params": {"public-key":"2cv1HqiCE8BeHc4Y7vucckMysx3PV7KND2mQiMdXqDJg812Fd5", "first-id": 0, "limit": 3}}
])"_json;

struct SessionEval {

    using Url = milecsa::rpc::Url;

    milecsa::ErrorHandler error = [](milecsa::result code, const std::string &error){
        BOOST_TEST_MESSAGE("Session Error: " + error);
    };

    milecsa::http::ResponseHandler response_handler = [&](const milecsa::http::response &http){
        BOOST_TEST_MESSAGE("Response Error: ");
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
        else {
            return nullptr;
        }

        return nullptr;
    }

    bool run_all_test(const std::string &u = "http://127.0.0.1:9998/v1/api"){
        try {
            if (tests.is_discarded()){
                return false;
            }

            for (auto& element : tests) {
                std::string method = element["method"];
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


BOOST_FIXTURE_TEST_CASE( rpc_session, SessionEval )
{
    BOOST_CHECK(run_all_test());
}
