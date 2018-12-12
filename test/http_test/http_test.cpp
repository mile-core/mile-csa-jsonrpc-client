//
// Created by lotus mile on 2018-12-12.
//

#define BOOST_TEST_MODULE http

#include "milecsa_http.hpp"

#include <optional>
#include <boost/test/included/unit_test.hpp>

std::string url = "https://raw.githubusercontent.com/mile-core/mile-files/master/genesis_block.txt";

struct HttpEval {

    using Url = milecsa::rpc::Url;

    milecsa::ErrorHandler errorHandler = [](milecsa::result code, std::string error){
        BOOST_TEST_MESSAGE("Http error: " + error);
    };

    bool test(const std::string &u = url) {

        if (auto client = milecsa::http::Client::Connect(u, true, errorHandler)) {
            if (auto body = client->get()) {
                std::cout<<body->c_str()<<std::endl;
                return true;
            }
        }

        return false;
    }
};


BOOST_FIXTURE_TEST_CASE( http, HttpEval ){
    BOOST_CHECK(test());
}