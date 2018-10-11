
file (GLOB TESTS_SOURCES ${TESTS_SOURCES}
        *.cpp
        )

set (TEST url_test_${PROJECT_LIB})

add_executable(${TEST} ${TESTS_SOURCES})
target_link_libraries (${TEST} ${PROJECT_LIB} ${MILECSA_LIB} ${Boost_LIBRARIES})
add_test (NAME urls COMMAND ${TEST})
enable_testing ()
