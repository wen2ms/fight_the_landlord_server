cmake_minimum_required(VERSION 3.26)

project(thread)

set(CMAKE_CXX_STANDARD 17)

aux_source_directory(./ SRC)

add_library(thread ${SRC})

target_link_libraries(thread pthread)