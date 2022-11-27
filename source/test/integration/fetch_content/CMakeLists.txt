cmake_minimum_required(VERSION 3.16)
set(NAME peer-integration-fetch-content)
project(${NAME} C)

include(FetchContent)
FetchContent_Declare(
  peer
  GIT_REPOSITORY https://github.com/automainint/peer.git
  GIT_TAG dev)
set(PEER_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(peer)

add_executable(${NAME})
target_sources(${NAME} PRIVATE main.c)
target_link_libraries(${NAME} PRIVATE peer::peer)

enable_testing()
add_test(NAME ${NAME}-run COMMAND ${NAME})
set_tests_properties(${NAME}-run PROPERTIES TIMEOUT "15")
