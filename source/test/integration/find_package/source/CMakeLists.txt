cmake_minimum_required(VERSION 3.16)
set(NAME peer_integration_find_package)
project(${NAME} C)

find_package(peer REQUIRED)

add_executable(${NAME})
target_sources(${NAME} PRIVATE main.c)
target_link_libraries(${NAME} PRIVATE peer::peer)
install(TARGETS ${NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
