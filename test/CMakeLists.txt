# CMakeLists.txt for AwsTests
#
# © 2018 by Richard Walters

cmake_minimum_required(VERSION 3.8)
set(This AwsTests)

set(Sources
    src/ConfigTests.cpp
    src/SignApiTests.cpp
    src/S3Tests.cpp
)

add_executable(${This} ${Sources})
set_target_properties(${This} PROPERTIES
    FOLDER Tests
)

target_compile_definitions(${This} PRIVATE
    TEST_VECTOR_DIR=${AWS_SIG_4_TEST_SUITE}/aws-sig-v4-test-suite
)

target_include_directories(${This} PRIVATE ..)

target_link_libraries(${This} PUBLIC
    gtest_main
    Aws
    SystemAbstractions
)

add_test(
    NAME ${This}
    COMMAND ${This}
)
