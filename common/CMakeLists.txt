cmake_minimum_required(VERSION 3.26)

project(common)

set(CMAKE_CXX_STANDARD 17)

aux_source_directory(./ SRC)

add_library(common ${SRC})

target_link_libraries(common jsoncpp)