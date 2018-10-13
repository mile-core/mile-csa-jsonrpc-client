//
// Created by lotus mile on 13/10/2018.
//

#define BOOST_TEST_MODULE rpc_session

#include "crypto_types.h"
#include "milecsa_jsonrpc.hpp"

#include <optional>
#include <boost/test/included/unit_test.hpp>

struct SessionEval {

    using Url = milecsa::rpc::Url;

    milecsa::ErrorHandler error = [](milecsa::result code, const std::string &error){
        BOOST_TEST_MESSAGE("Session Error: " + error);
    };

    milecsa::http::ResponseHandler response_handler = [&](const milecsa::http::response &http){
        std::cerr << "Response error: " << http.result() << std::endl << http << std::endl;
    };

    std::shared_ptr<milecsa::rpc::detail::RpcSession> get_session(const std::string &u){
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

    bool get_block_history_state(const std::string &u = "http://127.0.0.1:9998/v1/api") {

        if (auto session = get_session(u)){

            auto command = session->next_command("get-block-history-state");
            auto respons = session->request(command, response_handler, error);

            if (!respons) {
                return false;
            }

            BOOST_TEST_MESSAGE(" -- ");
            BOOST_TEST_MESSAGE("get-block-history-state: " + respons->dump());
        }
        else {
            return false;
        }

        return true;
    }

    bool get_block_history(const std::string &u = "http://127.0.0.1:9998/v1/api") {

        if (auto session = get_session(u)){

            auto command = session->next_command("get-block-history", {{"first-id", 0}, {"limit", 11}});

            auto respons = session->request(command, response_handler, error);

            if (!respons) {
                return false;
            }

            BOOST_TEST_MESSAGE(" -- ");
            BOOST_TEST_MESSAGE("get-block-history: " + respons->dump());
        }
        else {
            return false;
        }

        return true;
    }
};


BOOST_FIXTURE_TEST_CASE( rpc_session, SessionEval )
{
    BOOST_CHECK(get_block_history_state());
    BOOST_CHECK(get_block_history());
}
