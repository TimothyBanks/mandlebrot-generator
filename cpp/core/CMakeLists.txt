cmake_minimum_required(VERSION 2.8)

project(on-boarding-project-lambda)

find_package(aws-greengrass-core-sdk-c REQUIRED)

add_executable(on-boarding-project-lambda main.cpp)

target_link_libraries(on-boarding-project-lambda aws-greengrass-core-sdk-c)
