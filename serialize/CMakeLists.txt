cmake_minimum_required(VERSION 3.26)

project(serialize)

set(CMAKE_CXX_STANDARD 17)

aux_source_directory(./ SRC)

file(GLOB HEADERS ./*.h)

add_library(serialize ${SRC} ${HEADERS})

target_link_libraries(serialize protobuf)
