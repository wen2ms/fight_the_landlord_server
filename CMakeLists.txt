cmake_minimum_required(VERSION 3.26)

project(fight_the_landlord_server)

find_package(OpenSSL REQUIRED)

set(CMAKE_CXX_STANDARD 17)

include_directories(Ï
    ${PROJECT_SOURCE_DIR}/common
    ${PROJECT_SOURCE_DIR}/http
    ${PROJECT_SOURCE_DIR}/reactor
    ${PROJECT_SOURCE_DIR}/tcp
    ${PROJECT_SOURCE_DIR}/thread
    ${PROJECT_SOURCE_DIR}/crypto
    ${PROJECT_SOURCE_DIR}/serialize
    ${PROJECT_SOURCE_DIR}/game)

add_subdirectory(common)
add_subdirectory(http)
add_subdirectory(reactor)
add_subdirectory(tcp)
add_subdirectory(thread)
add_subdirectory(crypto)
add_subdirectory(serialize)
add_subdirectory(game)

link_libraries(common http reactor tcp thread crypto serialize game)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

add_executable(fight_the_landlord_server main.cpp)