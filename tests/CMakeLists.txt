include(FetchContent)

# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif ()

# GoogleTest
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/35d0c365609296fa4730d62057c487e3cfa030ff.zip
)
FetchContent_MakeAvailable(googletest)


include(GoogleTest)

# example of how to set up a test with GTest
add_executable(hello_test hello_test.cc)
target_link_libraries(hello_test GTest::gtest_main)
gtest_discover_tests(hello_test)

# tests for neighbour list - n.b. not in public headers
add_executable(test_nl test_neighbour_list.cpp)
target_link_libraries(test_nl GTest::gtest_main mace_testing)
gtest_discover_tests(test_nl)
