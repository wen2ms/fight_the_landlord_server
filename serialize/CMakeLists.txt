cmake_minimum_required(VERSION 3.16)

project(serialize)

aux_source_directory(./ SRC)

file(GLOB HEADERS ./*.h)

add_library(serialize ${SRC} ${HEADERS})

target_link_libraries(serialize protobuf)
