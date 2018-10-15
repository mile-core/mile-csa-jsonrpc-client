//
// Created by denn on 05/10/2018.
//

#include "milecsa_url.hpp"
#include <string>
#include <iostream>
#include <cctype>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>


namespace milecsa::rpc {

    Url::Url(const milecsa::rpc::Url &u):
            url_string_(u.url_string_),
            path_(u.path_),
            port_(u.port_),
            query_(u.query_),
            protocol_(u.protocol_),
            domain_(u.domain_) {
    }

    Url::Url(const std::string &urlString):
    url_string_(urlString),
    path_(""),
    port_(0),
    query_(""),
    protocol_(unknown),
    domain_(""){}

    std::optional<Url> Url::Parse(const std::string &urlString, const milecsa::ErrorHandler &error) {

        if (urlString.empty()) {
            error(result::EMPTY,ErrorFormat("Url string is empty"));
            return std::nullopt;
        }

        boost::regex ex("(https?|ftp)://([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
        boost::cmatch what;

        Url url(urlString);

        if(regex_match(urlString.c_str(), what, ex))
        {
            std::string proto = std::string(what[1].first, what[1].second);
            boost::to_upper(proto);

            if ( proto == "HTTP") {
                url.protocol_ = protocol::http;
            }
            else if (proto == "HTTPS") {
                url.protocol_ = protocol::https;
            }
            else {
                error(result::NOT_SUPPORTED, ErrorFormat("Url protocol %s is not supported yet", proto.c_str()));
                return std::nullopt;
            }
            url.domain_ = string(what[2].first, what[2].second);
            std::string port = string(what[3].first, what[3].second);

            if (port.empty()){
                switch (url.protocol_){
                    case https:
                        url.port_ = 443;
                        break;
                    default:
                        url.port_ = 80;
                }
            }
            else {
                int port_num = std::stoi(port, nullptr, 0);
                if (port_num>=0 && port_num <= 65535) {
                    url.port_ = (uint16_t) port_num;
                } else {
                    error(result::NOT_SUPPORTED, ErrorFormat("Url port number %s is out of range", port.c_str()));
                    return std::nullopt;
                }
            }

            url.path_ = string(what[4].first, what[4].second);
            url.query_ = string(what[5].first, what[5].second);
            url.fragment_ = string(what[6].first, what[6].second);
        }
        else {
            error(result::NOT_SUPPORTED,ErrorFormat("Url string format error"));
            return std::nullopt;
        }

        return std::make_optional(url);
    }
}