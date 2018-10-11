
find_package (Threads)

file (GLOB MILE_CLI_WALLET_SOURCES ${MILE_CLI_WALLET_SOURCES}
        *.cpp
        )

set (MILE_CLI_WALLET mile_cli_wallet)

add_executable(${MILE_CLI_WALLET} ${MILE_CLI_WALLET_SOURCES})
target_link_libraries (
        ${MILE_CLI_WALLET}
        ${PROJECT_LIB}
        ${MILECSA_LIB}
        ${OPENSSL_SSL_LIBRARY}
        ${OPENSSL_CRYPTO_LIBRARY}
        ${CMAKE_THREAD_LIBS_INIT}
        ${Boost_LIBRARIES})
