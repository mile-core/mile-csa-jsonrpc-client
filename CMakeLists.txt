cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 17)

set (PROJECT_LIB milecsa_jsonrpc)

set (CMAKE_CXX_STANDARD 17)
set (Boost_USE_STATIC_LIBS ON)
set (Boost_USE_MULTITHREADED OFF)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

file(GLOB LIBRARY_FILE_LIST ${CMAKE_SOURCE_DIR}/vendor/mile-csa-api/*.*)
list(LENGTH LIBRARY_FILE_LIST NUMBER_OF_FILES)
if (NUMBER_OF_FILES EQUAL 0)
    message(STATUS "'mile-csa-api' library not found")
    execute_process(COMMAND git submodule update --init --recursive vendor/mile-csa-api
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
else ()
    message(STATUS "'mile-csa-api' library found")
endif()

set(MILECSA_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/mile-csa-api/include
        CACHE PATH "milecsa include directory")

set(MILECSA_CRYPTO_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/mile-csa-api/vendor/mile-crypto/include
        CACHE PATH "milecsa crypto include directory")

set(MILECSA_NLOHMANN_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/mile-csa-api/vendor/nlohmann
        CACHE PATH "milecsa nlohmann json sdk include directory")

set(MILECSA_LIB milecsa)

set (BOOST_COMPONENTS
        system
        regex
        unit_test_framework
        program_options
        )

file (GLOB SOURCES ${SOURCES}
        src/*.cpp
        )

project(${PROJECT_LIB})

find_package (Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})
find_package(OpenSSL)

include_directories(
        ./include
        ${Boost_INCLUDE_DIRS}
        ${MILECSA_INCLUDE_DIR}
        ${MILECSA_CRYPTO_INCLUDE_DIR}
        ${MILECSA_ED25519_INCLUDE_DIR}
        ${MILECSA_NLOHMANN_INCLUDE_DIR}
        ${OPENSSL_INCLUDE_DIR}
)


if (MSVC)
add_definitions (/W4)
elseif (CMAKE_COMPILER_IS_GNUCXX)
add_definitions (-Wall -pedantic)
else ()
message ("Unknown compiler")
endif ()

source_group ("Source Files" FILES ${SOURCES})

add_subdirectory(vendor/mile-csa-api ${CMAKE_BINARY_DIR}/mile-csa-api-build)

add_library(${PROJECT_LIB} ${SOURCES})

target_link_libraries (
        ${PROJECT_LIB} PUBLIC
        ${MILECSA_LIB}
        ${Boost_LIBRARIES}
        ${OPENSSL_SSL_LIBRARY}
        ${OPENSSL_CRYPTO_LIBRARY}
)

target_include_directories(
        ${PROJECT_LIB}
        PUBLIC
        ${MILECSA_INCLUDE_DIR}
        ${MILECSA_CRYPTO_INCLUDE_DIR}
        ${MILECSA_ED25519_INCLUDE_DIR}
        ${MILECSA_NLOHMANN_INCLUDE_DIR}
)


add_subdirectory(test)
add_subdirectory(mile_cli_wallet)
enable_testing ()

SET(CMAKE_INSTALL_PREFIX "/usr/local/milecsa")

FILE(GLOB INCLUDE_FILES
        include/*.hpp
        )

install(FILES ${INCLUDE_FILES} DESTINATION include)
install(TARGETS ${PROJECT_LIB} DESTINATION lib)
