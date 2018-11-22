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
1. Centos7 
   ([Devtoolset-7](https://www.softwarecollections.org/en/scls/rhscl/devtoolset-7/))
1. Ubuntu 18.4
1. OSX 10.13, XCode10


### How to use ###

```
$ ./mile_cli_wallet --help
Allowed options:
  --help                                produce help message
  --debug                               print debug messages
  -w [ --wallet ] arg                   create wallet from random (-w "random")
                                        or secret phrase
  -u [ --url ] arg (=http://lotus000.testnet.mile.global/v1/api)
                                        RPC url
  -m [ --method ] arg                   blockchain command
  -p [ --params ] arg (={})             blockchain params
  -r [ --reconnections ] arg (=3)       try to connect again reconnection times
                                        if any connection error occurred

Methods: 
	get-blockchain-info 
	get-blockchain-state 
	get-network-state 
	get-nodes 
	get-current-block-id 
	get-block --params '{"id":1}'
	get-wallet-state --params '{"public-key": "...bHqSm8WTuY3gB9UXD..."}' 
	get-wallet-transactions --params '{"public-key": "...bHqSm8WTuY3gB9UXD..."}' 
	send-transfer --params '{"private-key": "...bHqSm8WTuY3gB9UXD...", "to":"...pXg1MF4qxZTEsL..", "amount": "100", "asset-code":0, "description":"send my money back!"}' 
	send-emission --params '{"private-key": "...bHqSm8WTuY3gB9UXD...", "asset-code":0}'
	register-node --params '{"private-key": "...bHqSm8WTuY3gB9UXD....", "amount": 10000.00, "address": "xxx.yyy.zzz.www"}'
	unregister-node --params '{"private-key": "...bHqSm8WTuY3gB9UXD...."}'
	vote-for-rate -p '{"private-key": "...bHqSm8WTuY3gB9UXD....", "amount": 1.21}'
```
    $ ./mile_cli_wallet -m send-transfer -p '{"private-key": "...", "to": "...", "asset-code": 0, amount:"1000", "description": "0x2039949102"}'
    
### Create master node wallet file

    $ ./mile_cli_wallet -o -w random > node.wallet
    $ scp ./node.wallet root@xxx.yyy.zzz.www:/etc/mile/wallets/

## Example 

```cpp

    #include "milecsa_jsonrpc.hpp"

    milecsa::ErrorHandler error_handler = [](milecsa::result code, const std::string &error){
        if (code == milecsa::result::FAIL) {
		//
		// Error handling...
		//
	}
    };

    milecsa::http::ResponseHandler response_fail_handler = [](
              const milecsa::http::status code,
              const std::string &method,
              const milecsa::http::response &http){
          std::cerr << "Response["<<code<<"] "<<method<<" error: " << http.result() << std::endl << http << std::endl;
      };
    
    auto u = "http://node.testnet.mile.global/v1/api";
    
    if (auto rpc = milecsa::rpc::Client::Connect(u, true, response_fail_handler, error_handler)) {

            std::count << *rpc->ping()) << endl;

            auto last_block_id = *rpc->get_current_block_id();
            cout << + UInt256ToDecString(last_block_id) << endl;

            cout << rpc->get_blockchain_info()->dump()) << endl;

            cout << rpc->get_blockchain_state()->dump()) << endl;

            auto pk = "EUjuoTty9oHdF8h7ab4u3KCCci5dduFxvJbqAx5qXUUtk2Wnx";
            cout << rpc->get_wallet_state(pk)->dump()) << endl;

            cout << rpc->get_wallet_transactions(pk, 5)->dump()) << endl;

            cout << rpc->get_network_state()->dump()) << endl;

            cout << rpc->get_nodes()->dump()) << endl;

            cout << rpc->get_block(last_block_id)->dump()) << endl;
    }
```

## Sending tokens example

```cpp

    #include "milecsa.hpp"
    #include "milecsa_jsonrpc.hpp"
    
    std::any transfer_xdr(std::optional<milecsa::rpc::Client> rpc, 
                          float &amount, 
                          const std:string &description = "send my coins back!") {
                
                //
                // you must to get current block id
                //
                auto block_id = *rpc->get_current_block_id();

                //
                // restore wallet pair from private key
                //
                auto ppk = milecsa::keys::Pair::FromPrivateKey("...",error_handler);

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
                auto asset = milecsa::assets::XDR;
                uint64_t trx_id = rand();
                
                // 
                // Build signed transaction data to send to node
                //
                if (auto trx_body = transfer::CreateRequest(
                        *ppk,
                        "....",
                        block_id,     // block id
                        trx_id,       // trx id
                        asset,        // asset
                        amount,       // amount
                        0.0,          // fee, allways 0.0
                        description,  // description
                        error_handler)->get_body()){
                    
                    //
                    // send prepared and signed transaction
                    //                   
                    if(auto t = rpc->send_transaction(*ppk,*trx_body)){
                        return std::any_cast<rpc::response>(t);
                    }
                }
    }
```

# MILE Explorer JSON-RPC API

## Proxy common API

In case of method (GET/PUT/POST/...) no or not processed http code 405 Method Not Allowed.

**Ping proxy service**

##### http request
```
POST https://wallet.mile.global/v1/api
Accept: */*
Cache-Control: no-cache
Content-Type: application/json

{"method":"ping","params":{}, "id": 1, "jsonrpc": "2.0", "version": "10"}
```
##### http response
```
HTTP/1.1 200 OK
Content-Type: application/json

{"jsonrpc":"2.0","version":"10","id":1,"result":true}
```

##### http error
```
HTTP/1.1 500 Internal Server Error
Content-Type: application/json

{"jsonrpc":"2.0","version":"1.0","id":1,"error":{"message":"unknown RPC method","code":-1}}
```

### Blockchain calls

#### Getting block-chain current info (block-chain configuration)
##### http request
```
POST https://wallet.mile.global/v1/api
Accept: */*
Cache-Control: no-cache
Content-Type: application/json

{"method":"get-blockchain-info","params":[], "id": 1, "jsonrpc": "2.0", "version": "1.0"}
```

##### http response
```
HTTP/1.1 200 OK
Content-Type: application/json

{"jsonrpc":"2.0","version":"1.0","id":1,
"result":{"project":"Mile",
"version":"1",
"supported-transaction-types":["RegisterNodeTransactionWithAmount","UnregisterNodeTransaction","TransferAssetsTransaction","CreatePollSetTokenCourse","VotingCoursePoll","VotingCourseCount","EmissionTransaction"],
"supported-assets":[{"name":"XDR tokens","code":0},
{"name":"Mile tokens","code":1}]}}
```

##### http error
```
HTTP/1.1 500 Internal Server Error
Content-Type: application/json

{"jsonrpc":"2.0","version":"1.0","id":1,"error":{"message":"unknown RPC method","code":-1}}
```

### Getting block-chain current state (block-chain configuration)
##### http request
```
POST https://wallet.mile.global/v1/api
Accept: */*
Cache-Control: no-cache
Content-Type: application/json

{"method":"get-blockchain-state","params":[], "id": 1, "jsonrpc": "2.0", "version": "1.0"}
```

##### http response
```
HTTP/1.1 200 OK
Content-Type: application/json

{"jsonrpc":"2.0","version":"1.0","id":1,
"result":{"project":"Mile",
"version":"1",
"block-count":4242,
"node-count":10000,
"voting-transaction-count":10,
"pending-transaction-count":10,
"blockchain-state":"Master",
"consensus-round":0,
"supported-assets": [{"name": "XDR tokens","code": "0"},{"name": "Mile tokens", "code": "1"}]}}
```

##### http error
```
HTTP/1.1 500 Internal Server Error
Content-Type: application/json

{"jsonrpc":"2.0","version":"1.0","id":1,"error":{"message":"unknown RPC method","code":-1}}
```

###Get block by id
##### http request

```
POST https://wallet.mile.global/v1/api
Accept: */*
Cache-Control: no-cache
Content-Type: application/json

{"method":"get-block-by-id","params":{"id": 42}, "id": 1, "jsonrpc": "2.0", "version": "10"}
```

##### http response
```
HTTP/1.1 200 OK
Content-Type: application/json

{"jsonrpc":"2.0","version":"10","id":1,"result":{....}}
```

##### http error
```
HTTP/1.1 500 Internal Server Error
Content-Type: application/json

{"jsonrpc":"2.0","version":"1.0","id":1,"error":{"message":"unknown RPC method","code":-1}}
```

##### http error
```
HTTP/1.1 400 Bad Request
Content-Type: application/json

{"jsonrpc":"2.0","version":"1.0","id":1,"error":{"message":couldn't find block id: 1","code":100x}}
```

### Getting block-chain network state (network configuration)


### Get common network states
##### http request

```
POST https://wallet.mile.global/v1/api
Accept: */*
Cache-Control: no-cache
Content-Type: application/json

{"method":"get-network-state","params":{}, "id": 1, "jsonrpc": "2.0", "version": "10"}
```

##### http response
```
HTTP/1.1 200 OK
Content-Type: application/json

{"jsonrpc":"2.0","version":"10","id":1,"result":{ "nodes": {"count":10}, "xxx": {"count":42}}}
```

### Get consensus node list with limit
##### http request

```
POST https://wallet.mile.global/v1/api
Accept: */*
Cache-Control: no-cache
Content-Type: application/json

{"method":"get-nodes","params":{}, "id": 1, "jsonrpc": "2.0", "version": "1.0"}
```

##### http response
```
HTTP/1.1 200 OK
Content-Type: application/json

{"jsonrpc":"2.0","version":"1.0","id":1,
 "result":[
   {"public-key":"1qaz2wsx3edc4rfv5tgb....", "address":"42.42.42.42", "node-id": "1"},
   {"public-key":"2daz2af3sx3ed51234xe....", "address":"242.242.242.242", "node-id": "10"}
 ]
}
```

### Wallet calls

#### Get wallet state
##### http request
```
POST https://wallet.mile.global/v1/api
Accept: */*
Cache-Control: no-cache
Content-Type: application/json

{"method":"get-wallet-state",
"params":{"public-key": "2mMN2bHqSm8WTuY3gB9UXDJsKnuZKKyDTpXg1MF4qxZTEsLHr3"}, "id": 1,
 "jsonrpc": "2.0", "version": "10"}         
```

##### http response
```
HTTP/1.1 200 OK
Content-Type: application/json

{"jsonrpc":"2.0","version":"10","id":1,
"result":{"balance":[{"asset-code":0,"amount":"9"},{"asset-code":1,"amount":"0.1"}],
"tags":"","node-address":"","last-transaction-id":"7","exist":1}}
```

##### http error
```
HTTP/1.1 500 Internal Server Error
Content-Type: application/json

{"jsonrpc":"2.0","version":"1.0","id":1,"error":{"message":"unknown RPC method","code":-1}}
```

##### http error
```
HTTP/1.1 400 Bad Request
Content-Type: application/json

{"jsonrpc":"2.0","version":"1.0","id":1,"error":{"message":couldn't decode wallet pulic key: base58 check string decode error","code":1001}}
```

#### Get wallet transactions
##### http request
```
POST https://wallet.mile.global/v1/api
Accept: */*
Cache-Control: no-cache
Content-Type: application/json

{"method":"get-wallet-transactions","params":{"public-key": "2mMN2bHqSm8WTuY3gB9UXDJsKnuZKKyDTpXg1MF4qxZTEsLHr3", "limit":1000}, "id": 1, "jsonrpc": "2.0", "version": "1"}         
```

##### http response
```
HTTP/1.1 200 OK
Content-Type: application/json

{"jsonrpc":"2.0","version":"10","id":1,
"result":{"transactions":[{"description":{
"type":"TransferAssetsTransaction",
"id":"10",
"from":"2mMN2bHqSm8WTuY3gB9UXDJsKnuZKKyDTpXg1MF4qxZTEsLHr3",
"to":"Z4mMN2bHqSm8WTuY3gB9UXDJsKnuZKKyDTpXg1MF4qxZTEsLHr3",
"asset-code":0,"amount":"20"},
"digest":"Q1QDzehqqBrtEqRNb2nzJnrn8nUPv1owQQenGjxXDWWFDGZbP","type":"approved"}]}}
```

##### http error
```
HTTP/1.1 500 Internal Server Error
Content-Type: application/json

{"jsonrpc":"2.0","version":"1.0","id":1,"error":{"message":"unknown RPC method","code":-1}}
```

##### http error
```
HTTP/1.1 400 Bad Request
Content-Type: application/json

{"jsonrpc":"2.0","version":"1.0","id":1,"error":{"message":couldn't decode wallet pulic key: base58 check string decode error","code":1001}}
```

#### Send signed transactions
##### http request

```
POST https://wallet.mile.global/v1/api
Accept: */*
Cache-Control: no-cache
Content-Type: application/json

{"method":"send-transaction","params":{...signed transaction body}, "id": 1, "jsonrpc": "2.0", "version": "10"}
```

##### http response
```
HTTP/1.1 200 OK
Content-Type: application/json

{"jsonrpc":"2.0","version":"10","id":1,
"result":true}
```

##### http error
```
HTTP/1.1 500 Internal Server Error
Content-Type: application/json

{"jsonrpc":"2.0","version":"1.0","id":1,"error":{"message":"unknown RPC method","code":-1}}
```

##### http error
```
HTTP/1.1 400 Bad Request
Content-Type: application/json

{"jsonrpc":"2.0","version":"1.0","id":1,"error":{"message":couldn't decode trx data","code":1002}}
```

#### Get current block id
##### http request

```
POST https://wallet.mile.global/v1/api
Accept: */*
Cache-Control: no-cache
Content-Type: application/json

{"method":"get-current-block-id","params":{}, "id": 1, "jsonrpc": "2.0", "version": "10"}
```

##### http response
```
HTTP/1.1 200 OK
Content-Type: application/json

{"jsonrpc":"2.0","version":"10","id":1,
"result":{ current-block-id": "22" }}
```

##### http error
```
HTTP/1.1 500 Internal Server Error
Content-Type: application/json

{"jsonrpc":"2.0","version":"1.0","id":1,"error":{"message":"unknown RPC method","code":-1}}
```

##### http error
```
HTTP/1.1 400 Bad Request
Content-Type: application/json

{"jsonrpc":"2.0","version":"1.0","id":1,"error":{"message":couldn't decode trx data","code":1002}}
```




# TODO

## Errors code (EXAMPLE CODES!!!)


    ```
    -1 - Unknown error
    0 - Ok
    1001 - Encoding error
    ```

```
-32700 	Parse error 	Invalid JSON was received by the server. An error occurred on the server while parsing the JSON text.
-32600 	Invalid Request 	The JSON sent is not a valid Request object.
-32601 	Method not found 	The method does not exist / is not available.
-32602 	Invalid params 	Invalid method parameter(s).
-32603 	Internal error 	Internal JSON-RPC error.
-32000 to -32099 	Server error 	Reserved for implementation-defined server-errors.
```

```
{"jsonrpc": "2.0", "error": {"code": -32601, "message": "Method not found"}, "id": "1"}
```

## Batch
    ```json
    
    [
        {},
        {}
    ]
    
    ```
