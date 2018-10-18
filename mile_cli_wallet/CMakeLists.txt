
find_package (Threads)

file (GLOB MILE_CLI_WALLET_SOURCES ${MILE_CLI_WALLET_SOURCES}
        mile_cli_wallet.cpp
        )

file (GLOB MILE_CLI_SIGNATURE_SOURCES ${MILE_CLI_SIGNATURE_SOURCES}
        mile_cli_transfer.cpp
        )

set (MILE_CLI_WALLET mile_cli_wallet)
set (MILE_CLI_SIGNATURE mile_cli_transfer)

add_executable(${MILE_CLI_WALLET} ${MILE_CLI_WALLET_SOURCES})
add_executable(${MILE_CLI_SIGNATURE} ${MILE_CLI_SIGNATURE_SOURCES})

set(LIBS ${PROJECT_LIB}
        ${MILECSA_LIB}
        ${OPENSSL_SSL_LIBRARY}
        ${OPENSSL_CRYPTO_LIBRARY}
        ${CMAKE_THREAD_LIBS_INIT}
        ${Boost_LIBRARIES})

target_link_libraries (
        ${MILE_CLI_WALLET}
        ${LIBS}
       )

target_link_libraries (
        ${MILE_CLI_SIGNATURE}
        ${LIBS})

install(TARGETS ${MILE_CLI_WALLET} DESTINATION bin)
install(TARGETS ${MILE_CLI_SIGNATURE} DESTINATION bin)