cmake_minimum_required(VERSION 3.16)

project(peer_integration_find_package C)

find_package(peer REQUIRED)

add_executable(peer_integration_find_package)
target_sources(peer_integration_find_package PRIVATE main.c)
target_link_libraries(peer_integration_find_package PRIVATE peer::peer)
install(TARGETS peer_integration_find_package RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
