cmake_minimum_required(VERSION 3.26)

project(crypto)

set(CMAKE_CXX_STANDARD 17)

aux_source_directory(./ SRC)

file(GLOB HEADERS ./*.h)

add_library(crypto ${SRC} ${HEADERS})

target_link_libraries(crypto OpenSSL::SSL OpenSSL::Crypto)
