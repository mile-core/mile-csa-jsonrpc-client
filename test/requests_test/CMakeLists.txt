find_package (Threads)

file (GLOB TESTS_SOURCES ${TESTS_SOURCES}
        *.cpp
        )

set (TEST requests_test_${PROJECT_LIB})

add_executable(${TEST} ${TESTS_SOURCES})

target_link_libraries (
        ${TEST}
        ${PROJECT_LIB}
        ${MILECSA_LIB}
        ${OPENSSL_SSL_LIBRARY}
        ${OPENSSL_CRYPTO_LIBRARY}
        ${CMAKE_THREAD_LIBS_INIT}
        ${Boost_LIBRARIES})

add_test (NAME request_builder COMMAND ${TEST})
enable_testing ()
