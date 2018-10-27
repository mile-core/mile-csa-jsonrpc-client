//
// Created by lotus mile on 18/10/2018.
//
#include <optional>
#include <functional>
#include "milecsa.hpp"
#include "milecsa_jsonrpc.hpp"
#include <boost/program_options.hpp>
#include <termios.h>

#define MAXPW ePrivateKeySize

static std::string opt_mile_node_address = "http://lotus001.testnet.mile.global/v1/api";
static std::string opt_to = "";
static std::string opt_private = "";
static unsigned short opt_asset_code = 100;
static float opt_amount = 0.0f;
static std::string opt_memo = "";
static int opt_reconnections = 3;
static bool opt_test = false;
static time_t opt_timeout = 3;

namespace po = boost::program_options;

static bool parse_cmdline(int ac, char *av[]);
ssize_t getpasswd (std::string &pwd, size_t sz, int mask, FILE *fp);

int main(int argc, char *argv[]) {

    setlocale(LC_ALL, "");

    if (!parse_cmdline(argc, argv))
        return 1;

    bool is_valid = false;

    if (opt_private.empty()){
        FILE *fp = stdin;
        for (int j = 0; j < 3; ++j) {
            printf ( "Enter wallet private key: ");
            getpasswd (opt_private, MAXPW, '*', fp);
            if(!milecsa::keys::Pair::ValidatePrivateKey(opt_private)){
                printf ( "\nPrivate key is not valid\n");
                continue;
            } else {
                is_valid = true;
                printf ( "\n");
                break;
            }
        }
    }
    else if(milecsa::keys::Pair::ValidatePrivateKey(opt_private))
            is_valid = true;

    if (!is_valid){
        printf ( "\nPrivate key is not valid\n");
        exit(-1);
    }

    milecsa::http::ResponseHandler response_fail_handler;
    milecsa::ErrorHandler error_handler;

    auto ppk = milecsa::keys::Pair::FromPrivateKey(opt_private,error_handler);
    using transfer = milecsa::transaction::Transfer<nlohmann::json>;

    auto do_request = [&]{
        if (auto rpc = milecsa::rpc::Client::Connect(
                opt_mile_node_address,
                true,
                response_fail_handler,
                error_handler)){

            ///
            /// to avoid duble spend and  keep high performance capability
            /// get last block id to add transaction siganture
            ///
            auto block_id = rpc->get_current_block_id();

            if(!block_id)
                return;

            auto asset = milecsa::assets::TokenFromCode(opt_asset_code);
            uint64_t trx_id = rand();

            auto request =
                    transfer::CreateRequest(
                            *ppk,        // wallet keys pair
                            opt_to,      // destination address: public key of recipient
                            *block_id,   // block id
                            trx_id,      // trx id
                            asset,       // asset code
                            opt_amount,  // amount
                            0.0,         // fee can be empty
                            opt_memo,    // transaction description
                            error_handler// error handler
                            )->get_body() ;

            if (request){

                if (opt_test) {
                    std::cout << request->dump() << std::endl;
                    return;
                }

                auto json_body = request->dump();

                if(auto t = rpc->send_transaction(*ppk,*request)){
                    std::cout<< "Send transfer: ";
                    std::cout << t->dump() << std::endl;;
                }
            }
        }
    };

    response_fail_handler = [](
            const milecsa::http::status code,
            const std::string &method,
            const milecsa::http::response &http){
        std::cerr << "Response["<<code<<"] "<<method<<" error: " << http.result() << std::endl << http << std::endl;
    };

    int reconnections=0;
    error_handler = [&](
            milecsa::result code,
            const std::string &error){
        std::cerr << "Call ["<<reconnections<<"] error: " << error << std::endl;

        if ((++reconnections >= opt_reconnections) || opt_test) {
            exit(-1);
        }

        if (code == milecsa::result::FAIL) {
            std::this_thread::sleep_for(std::chrono::seconds(opt_timeout));
            do_request();
        }
    };

    do_request();
}


/* read a string from fp into pw masking keypress with mask char.
getpasswd will read upto sz - 1 chars into pw, null-terminating
the resulting string. On success, the number of characters in
pw are returned, -1 otherwise.
*/
ssize_t getpasswd (std::string &pw, size_t sz, int mask, FILE *fp)
{
    if (!sz || !fp) return -1;       /* validate input   */
#ifdef MAXPW
    if (sz > MAXPW) sz = MAXPW;
#endif

    if (!pw.empty()) {
        pw.clear();
    }

    size_t idx = 0;         /* index, number of chars in read   */
    int c = 0;

    struct termios old_kbd_mode;    /* orig keyboard settings   */
    struct termios new_kbd_mode;

    if (tcgetattr (0, &old_kbd_mode)) { /* save orig settings   */
        fprintf (stderr, "%s() error: tcgetattr failed.\n", __func__);
        return -1;
    }   /* copy old to new */
    memcpy (&new_kbd_mode, &old_kbd_mode, sizeof(struct termios));

    new_kbd_mode.c_lflag &= ~(ICANON | ECHO);  /* new kbd flags */
    new_kbd_mode.c_cc[VTIME] = 0;
    new_kbd_mode.c_cc[VMIN] = 1;
    if (tcsetattr (0, TCSANOW, &new_kbd_mode)) {
        fprintf (stderr, "%s() error: tcsetattr failed.\n", __func__);
        return -1;
    }

    /* read chars from fp, mask if valid char specified */
    while (((c = fgetc (fp)) != '\n' && c != EOF && idx < sz - 1) ||
           (idx == sz - 1 && c == 127))
    {
        if (c != 127) {
            if (31 < mask && mask < 127)    /* valid ascii char */
                fputc (mask, stdout);
            pw += c;
        }
        else if (idx > 0) {         /* handle backspace (del)   */
            if (31 < mask && mask < 127) {
                fputc (0x8, stdout);
                fputc (' ', stdout);
                fputc (0x8, stdout);
            }
            pw[--idx] = '\0';
        }
    }

    /* reset original keyboard  */
    if (tcsetattr (0, TCSANOW, &old_kbd_mode)) {
        fprintf (stderr, "%s() error: tcsetattr failed.\n", __func__);
        return -1;
    }

    if (idx == sz - 1 && c != '\n') /* warn if pw truncated */
        fprintf (stderr, " (%s() warning: truncated at %zu chars.)\n",
                 __func__, sz - 1);

    return idx; /* number of chars in passwd    */
}


static bool parse_cmdline(int ac, char *av[]) {

    try {

        po::options_description desc("Allowed options");

        std::string wallet_phrase="";

        desc.add_options()
                ("help", "produce help message")

                ("debug", "print debug messages")

                ("test", "only print transaction body to console")

                ("url,u", po::value<std::string>(&opt_mile_node_address)->
                         default_value(opt_mile_node_address),
                 "RPC url")

                ("to,t", po::value<std::string>(&opt_to),
                 "destination public key")

                ("asset,c", po::value<unsigned short>(&opt_asset_code),
                 "asset code XDR:0, MILE:1")

                ("amount,a", po::value<float>(&opt_amount)->
                         default_value(opt_amount),
                 "transfer amount")

                ("description,d", po::value<std::string>(&opt_memo)->
                         default_value(opt_memo),
                 "add transaction description")

                ("private_key,p", po::value<std::string>(&opt_private)->
                         default_value(opt_private),
                 "destination public key")

                ("reconnections,r", po::value<int>(&opt_reconnections)->
                         default_value(opt_reconnections),
                 "try to connect again reconnection times if any connection error occurred")
                ;

        po::variables_map vm;
        po::store(po::parse_command_line(ac, av, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
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

        if (opt_to.empty()) {
            std::cout << desc << "\n";
            exit(0);
        }

        if (opt_asset_code==100){
            std::cout << desc << "\n";
            exit(0);
        }

        if (opt_amount <= 0 ){
            std::cout << desc << "\n";
            exit(0);
        }

        if (vm.count("debug")) {
            milecsa::rpc::detail::RpcSession::debug_on = true;
        }

        if (vm.count("test")) {
            opt_test = true;
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
