cmake_minimum_required(VERSION 3.16)

project(peer_integration_fetch_content C)

include(FetchContent)
FetchContent_Declare(
  peer
  GIT_REPOSITORY https://github.com/automainint/peer.git
  GIT_TAG dev)
set(PEER_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(peer)

add_executable(peer_integration_fetch_content)
target_sources(peer_integration_fetch_content PRIVATE main.c)
target_link_libraries(peer_integration_fetch_content PRIVATE peer::peer)

enable_testing()
add_test(NAME peer_integration_fetch_content-run COMMAND peer_integration_fetch_content)
set_tests_properties(peer_integration_fetch_content-run PROPERTIES TIMEOUT "15")
