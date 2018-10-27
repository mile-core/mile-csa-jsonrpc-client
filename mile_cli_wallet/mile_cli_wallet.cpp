//
// Created by denn on 04/10/2018.
//
#include <optional>
#include <functional>
#include "milecsa.hpp"
#include "milecsa_jsonrpc.hpp"
#include <boost/program_options.hpp>
#include <boost/chrono/chrono.hpp>

static std::string opt_mile_node_address = "http://lotus000.testnet.mile.global/v1/api";
static std::string opt_method= "";
static std::string opt_method_params = "{}";
static int opt_reconnections = 3;
static time_t opt_timeout = 3;

namespace po = boost::program_options;

static bool parse_cmdline(int ac, char *av[]);

int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "");

    if (!parse_cmdline(argc, argv))
        return 1;

    milecsa::http::ResponseHandler response_fail_handler;
    milecsa::ErrorHandler error_handler;
    int reconnections = 0;

    auto do_request = [&]{
        if (auto rpc = milecsa::rpc::Client::Connect(
                opt_mile_node_address,
                true,
                response_fail_handler,
                error_handler)){

            try {
                nlohmann::json params;
                try {
                    params = nlohmann::json::parse(opt_method_params);
                }
                catch (std::exception &e) {
                    std::cerr << "Params parser error: " << e.what() << "\n";
                }

                auto result = rpc->call(opt_method,params);

                if (!result.has_value()) {
                    std::cerr << "Rpc error: response does not have any result"<< std::endl;
                    exit(-1);
                }

                std::cout<< "Call " << opt_method << ": ";

                if (result.type() == typeid(time_t))
                    std::cout << std::any_cast<time_t>(result);


                else if (result.type() == typeid(uint256_t)){
                    std::cout << std::any_cast<uint256_t>(result);
                }

                else if (result.type() == typeid(milecsa::rpc::response)){
                    std::cout << std::any_cast<milecsa::rpc::response>(result)->dump();
                }

                std::cout << std::endl;
            }
            catch (std::exception &e) {
                std::cerr << "Method error: " << e.what() << "\n";
            }
        }
    };

    response_fail_handler = [](
            const milecsa::http::status code,
            const std::string &method,
            const milecsa::http::response &http){
        std::cerr << "Response["<<code<<"] "<<method<<" error: " << http.result() << std::endl << http << std::endl;
    };

    error_handler = [&](
            milecsa::result code,
            const std::string &error){
        std::cerr << "Call ["<<reconnections<<"] error: " << error << std::endl;

        if (code != milecsa::result::FAIL) {
            exit(-1);
        }

        if (++reconnections >= opt_reconnections ) {
            exit(-1);
        }

        std::this_thread::sleep_for(std::chrono::seconds(opt_timeout));

        do_request();
    };

    do_request();

    exit(0);
}

static bool parse_cmdline(int ac, char *av[]) {

    try {

        po::options_description desc("Allowed options");

        std::string wallet_phrase="";

        desc.add_options()
                ("help", "produce help message")

                ("debug", "print debug messages")

                ("wallet,w", po::value<std::string>(&wallet_phrase),
                 "create wallet from random (-w \"random\") or secret phrase")

                ("url,u", po::value<std::string>(&opt_mile_node_address)->
                         default_value(opt_mile_node_address),
                 "RPC url")

                ("method,m", po::value<std::string>(&opt_method)->
                         default_value(opt_method),
                 "blockchain command")

                ("params,p", po::value<std::string>(&opt_method_params)->
                         default_value(opt_method_params),
                 "blockchain params")

                ("reconnections,r", po::value<int>(&opt_reconnections)->
                         default_value(opt_reconnections),
                 "try to connect again reconnection times if any connection error occurred")
                ;

        std::string ext = ""\
        "Methods: \n"\
        "\tget-blockchain-info \n"\
        "\tget-blockchain-state \n"\
        "\tget-network-state \n"\
        "\tget-nodes \n"\
        "\tget-current-block-id \n"\
        "\tget-block --params '{\"id\":1}'\n"\
        "\tget-wallet-state --params '{\"public-key\": \"...bHqSm8WTuY3gB9UXD...\"}' \n"\
        "\tget-wallet-transactions --params '{\"public-key\": \"...bHqSm8WTuY3gB9UXD...\"}' \n"\
        "\tsend-transfer --params '{\"private-key\": \"...bHqSm8WTuY3gB9UXD...\", \"to\":\"...pXg1MF4qxZTEsL..\", \"amount\": \"100\", \"asset-code\":0, \"description\":\"send my money back!\"}' \n"\
         "\tsend-emission --params '{\"private-key\": \"...bHqSm8WTuY3gB9UXD...\", \"asset-code\":0}' \n"\
        "";

        po::variables_map vm;
        po::store(po::parse_command_line(ac, av, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            std::cout << ext << "\n";
            exit(0);
        }

        if (vm.count("wallet")) {
            std::optional<milecsa::keys::Pair> pair;
            if (wallet_phrase.empty() || wallet_phrase=="random"){
                pair = milecsa::keys::Pair::Random();
            }
            else {
                pair = milecsa::keys::Pair::WithSecret(wallet_phrase);
            }
            std::cout<<"Wallet public key:  " << pair->get_public_key().encode() << std::endl;
            std::cout<<"       private key: " << pair->get_private_key().encode() << std::endl;

            exit(0);
        }

        if (opt_method.empty()) {
            std::cout << desc << "\n";
            std::cout << ext << "\n";
            exit(0);
        }

        if (vm.count("debug")) {
            milecsa::rpc::detail::RpcSession::debug_on = true;
        }
    }

    catch (std::exception &e) {
        std::cerr << "error: " << e.what() << "\n";
        return false;
    }
    catch (...) {
        std::cerr << "Exception of unknown type!\n";
        return false;
    }

    return true;
}
