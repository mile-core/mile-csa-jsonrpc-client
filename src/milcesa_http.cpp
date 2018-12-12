//
// Created by lotus mile on 2018-12-12.
//

#include "milecsa_http.hpp"

namespace milecsa::http {
    time_t Client::timeout = 3;

    using namespace boost::asio::ip;
    namespace ssl = boost::asio::ssl;

    std::optional<std::string> Client::get() {

        boost::beast::http::request<boost::beast::http::empty_body> req;

        try {

            req.version(11);
            req.method(boost::beast::http::verb::get);
            req.target(session->get_target());
            req.set(boost::beast::http::field::host, session->get_host());
            req.set(boost::beast::http::field::user_agent, user_agent);

            if (!session->write(req,error_handler))
                return std::nullopt;

            boost::beast::http::response<boost::beast::http::string_body> res;

            if (!session->read(res,error_handler))
                return std::nullopt;

            auto status = res.result();

            if (status == boost::beast::http::status::ok) {
                return std::make_optional(res.body().data());
            }

            return std::nullopt;
        }
        catch(std::exception const& e)
        {
            error_handler(result::FAIL,ErrorFormat("http request: %s: %s:%s", e.what() , session->get_host().c_str(), session->get_port().c_str()));
            return std::nullopt;
        }
        catch (...) {
            error_handler(milecsa::result::EXCEPTION, ErrorFormat("http request: unknown error"));
            return std::nullopt;
        }
    }


    Client& Client::operator = (const Client& client) {
        url_ = client.url_;
        verify_ssl_=client.verify_ssl_;
        session=std::move(client.session);
        error_handler=client.error_handler;
        return *this;
    }

    Client::Client(const Client &client):
            url_(client.url_),
            verify_ssl_(client.verify_ssl_),
            session(std::move(client.session)),
            error_handler(client.error_handler){}

    std::optional<Client> Client::Connect(
            const std::string &urlString,
            bool verify_ssl,
            const milecsa::ErrorHandler &error_handler) {
        if(auto url = Url::Parse(urlString, error_handler)){

            auto client = Client(*url, verify_ssl, error_handler);

            if (!client.session->connect(error_handler)) {
                return std::nullopt;
            }

            return std::move(client);
        }
        return std::nullopt;
    }

    Client::Client(
            const milecsa::rpc::Url &url,
            bool verify_ssl,
            const ErrorHandler &error_handler) :
            url_(url),
            verify_ssl_(verify_ssl),
            error_handler(error_handler)
    {

        session = std::shared_ptr<milecsa::http::Session>(
                new milecsa::http::Session(
                        url_->get_host(),
                        url_->get_port(),
                        url_->get_path(),
                        url_->get_protocol(),
                        verify_ssl,
                        Client::timeout));
    }

    Client::~Client(){
        session.reset();
    };
}