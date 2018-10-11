//
// Created by lotus mile on 29/09/2018.
//

#define BOOST_TEST_MODULE url

#include <optional>
#include "milecsa_url.hpp"
#include <boost/test/included/unit_test.hpp>

struct UrlEval {

    using Url = milecsa::rpc::Url;

    milecsa::ErrorHandler errorHandler = [](milecsa::result code, std::string error){
        BOOST_TEST_MESSAGE("Url parsing error: " + error);
    };

    bool test(const std::string &u = "http://node002.testnet.mile.global/v1/api") {
        if (auto url = Url::Parse(u,errorHandler)) {
            BOOST_TEST_MESSAGE("Protocol: " + StringFormat("%i",url->get_protocol()));
            BOOST_TEST_MESSAGE("Host:  " + url->get_host());
            BOOST_TEST_MESSAGE("Port:  " + StringFormat("%i",url->get_port()));
            BOOST_TEST_MESSAGE("Path:  " + url->get_path());
            BOOST_TEST_MESSAGE("Query: " + url->get_query());
            BOOST_TEST_MESSAGE("Frag:  " + url->get_fragment());
            return true;
        }
        return false;
    }
};


BOOST_FIXTURE_TEST_CASE( Url, UrlEval )
{
    BOOST_CHECK(test());
    BOOST_CHECK(test("http://node002.testnet.mile.global/v1/api?foo=1&bar=_whatsup"));
    BOOST_CHECK(test("http://mile.global:8080/v1/api?foo=1&bar=_whatsup"));
    BOOST_CHECK(test("https://mile.global/v1/api?foo=1&bar=_whatsup"));
    BOOST_CHECK(test("https://mile.global:994/v1/api?foo=1&bar=_whatsup"));
}

BOOST_FIXTURE_TEST_CASE( UrlFails, UrlEval )
{
    BOOST_CHECK(!test("gopher://mile.global"));
    BOOST_CHECK(!test("ftp://mile.global:994"));
    BOOST_CHECK(!test("http://mile.global:-1"));
    BOOST_CHECK(!test("http://mile.global:65536"));
}