# MILE Wallet JSON-RPC API and CLI  #

## Requirements
1. c++17
1. cmake
1. boost >= 1.66.0 installed, boost.beast (do not update to 1.68!)
1. https://github.com/mile-core/mile-csa-api


## Build
1. git clone https://github.com/mile-core/mile-csa-jsonrpc-client
1. cd ./mile-csa-jsonrpc-client; 
1. git submodule init; git submodule update --recursive;
1. mkdir build; cd ./build 
1. cmake ..; make -j4

## Buid on OS X with openssl
1. cmake -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl ..; make -j4

## Boost updates (if it needs)

    $ wget https://dl.bintray.com/boostorg/release/1.67.0/source/boost_1_67_0.tar.gz
    $ tar -xzf boost_1_*
    $ cd boost_1_*
    $ ./bootstrap.sh --prefix=/usr
    $ ./b2 install --prefix=/usr --with=all -j4

## Tested
1. Centos7 (gcc v7.0) 
1. Ubuntu 18.4
1. OSX 10.13, XCode10


### How to use ###

```
$ ./mile_cli_wallet --help
Allowed options:
  --help                                produce help message
  --debug                               print debug messages
  --no-keepalive                        do not asc keep-alive header
  -u [ --url ] arg (=http://node002.testnet.mile.global/v1/api)
                                        RPC url
  -m [ --method ] arg                   blockchain command
  -p [ --params ] arg (={})             blockchain params

Allowed methods: 
	get-blockchain-info 
	get-blockchain-state 
	get-network-state 
	get-nodes 
	get-current-block-id 
	get-block --params '{"id":1}'
	get-wallet-state --params '{"public-key": "...bHqSm8WTuY3gB9UXD..."}' 
	get-wallet-transactions --params '{"public-key": "...bHqSm8WTuY3gB9UXD..."}' 
	send-transfer --params '{"private-key": "...bHqSm8WTuY3gB9UXD...", "to":"...pXg1MF4qxZTEsL..", "amount": "100", "asset-code":0, "description":"send my money back!"}' 
    send-emission --params '{"private-key": "...bHqSm8WTuY3gB9UXD...", "to":"...pXg1MF4qxZTEsL..", "amount": "100", "asset-code":0, "description":"send my money back!"}'
```
    $ ./mile_cli_wallet -m send-transfer -p '{"private-key": "...", "to": "...", "asset-code": 0, amount:"1000", "description": "0x2039949102"}'

## Example 

```cpp

    #include "milecsa_jsonrpc.hpp"

    milecsa::ErrorHandler error = [](milecsa::result code, const std::string &error){
        BOOST_TEST_MESSAGE("Request Error: " + error);
    };

    milecsa::http::ResponseHandler response = [&](const milecsa::http::response &http){
        std::cerr << "Response error: " << http.result() << std::endl << http << std::endl;
    };
    
    auto u = "http://node.testnet.mile.global/v1/api";
    
    if (auto rpc = milecsa::rpc::Client::Connect(u, true, error)) {

            std::count << *rpc->ping(response)) << endl;

            auto last_block_id = *rpc->get_current_block_id(response);
            cout << + UInt256ToDecString(last_block_id) << endl;

            cout << rpc->get_blockchain_info(response)->dump()) << endl;

            cout << rpc->get_blockchain_state(response)->dump()) << endl;

            auto pk = "EUjuoTty9oHdF8h7ab4u3KCCci5dduFxvJbqAx5qXUUtk2Wnx";
            cout << rpc->get_wallet_state(pk, response)->dump()) << endl;

            cout << rpc->get_wallet_transactions(pk, 5, response)->dump()) << endl;

            cout << rpc->get_network_state(response)->dump()) << endl;

            cout << rpc->get_nodes(response)->dump()) << endl;

            cout << rpc->get_block(last_block_id, response)->dump()) << endl;
    }
```

## Sending tokens example

```cpp

    #include "milecsa.hpp"
    #include "milecsa_jsonrpc.hpp"
    
    std::any transfer_xdr(std::optional<milecsa::rpc::Client> rpc, 
                          const std::string &amount, const std:string &description = "send my coins back!") {
                
                //
                // you must to get current block id
                //
                auto block_id = *rpc->get_current_block_id(handler);

                //
                // restore wallet pair from private key
                //
                auto ppk = milecsa::keys::Pair::FromPrivateKey("...",error);

                if (!ppk) {
                    //
                    // Handle private key error
                    //
                    return std::nullopt;
                }

                using transfer = milecsa::transaction::Transfer<nlohmann::json>;

                //
                // choose asset code 
                //
                unsigned short asset_code = 0;
                uint64_t trx_id = rand();
                
                // 
                // Build signed transaction data to send to node
                //
                if (auto trx_body = transfer::CreateRequest(
                        *ppk,
                        "....",
                        block_id,   // block id
                        trx_id,     // trx id
                        asset_code, // asset code
                        amount,     // amount
                        description,// description
                        "",         // fee, can be skiped 
                        error)->get_body()){
                    
                    //
                    // send prepared and signed transaction
                    //                   
                    if(auto t = rpc->send_transaction(*ppk,*trx_body,handler)){
                        return std::any_cast<rpc::response>(t);
                    }
                }
    }
```
