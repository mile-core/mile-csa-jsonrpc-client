cmake_minimum_required(VERSION 3.0)

include_directories (
       ./
)

add_subdirectory(utils_test)
add_subdirectory(requests_test)
add_subdirectory(http_test)
enable_testing ()
