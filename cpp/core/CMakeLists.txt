cmake_minimum_required(VERSION 2.8)

project(mandlebrot-loadbalancer)

find_package(aws-greengrass-core-sdk-c REQUIRED)

add_executable(mandlebrot-loadbalancer main.cpp)

target_include_directories(mandlebrot-loadbalancer PUBLIC ../library/include)
target_link_libraries(mandlebrot-loadbalancer aws-greengrass-core-sdk-c)
