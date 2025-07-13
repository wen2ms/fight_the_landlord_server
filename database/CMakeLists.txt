cmake_minimum_required(VERSION 3.26)

project(database)

include_directories(/usr/include/)

link_directories(/usr/lib/x86_64-linux-gnu)

file(GLOB HEADERS ./*.h)

aux_source_directory(./ SRC)

add_library(database ${SRC} ${HEADERS})

target_link_libraries(database mysqlclient)