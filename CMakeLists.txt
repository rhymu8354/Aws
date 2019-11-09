# CMakeLists.txt for Aws
#
# © 2018 by Richard Walters

cmake_minimum_required(VERSION 3.8)
set(This Aws)

set(Headers
    include/Aws/Config.hpp
    include/Aws/S3.hpp
    include/Aws/SignApi.hpp
)

set(Sources
    src/Config.cpp
    src/S3.cpp
    src/SignApi.cpp
)

add_library(${This} STATIC ${Sources} ${Headers})
set_target_properties(${This} PROPERTIES
    FOLDER Libraries
)

target_include_directories(${This} PUBLIC include)

target_link_libraries(${This} PUBLIC
    Hash
    Http
    Json
    MessageHeaders
    StringExtensions
    SystemAbstractions
    Uri
)

add_subdirectory(test)
