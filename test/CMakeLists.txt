
set (MILE_TEST_LIB milecsa_jsonrpc_test)

include_directories (
       ./
)

file (GLOB SOURCES ${SOURCES}
        *.cpp
        )

add_library(${MILE_TEST_LIB} ${SOURCES})

add_subdirectory(utils_test)
add_subdirectory(requests_test)
enable_testing ()
