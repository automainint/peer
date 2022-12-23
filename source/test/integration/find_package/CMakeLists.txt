cmake_minimum_required(VERSION 3.16)

project(peer_integration_find_package_root)

include(ExternalProject)

ExternalProject_Add(
  peer
  GIT_REPOSITORY https://github.com/automainint/peer.git
  GIT_TAG dev
  CMAKE_ARGS
  -D CMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/install
  -D PEER_ENABLE_TESTING=OFF)

ExternalProject_Add(
  peer_integration_find_package
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source
  CMAKE_ARGS
  -D CMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/install)

ExternalProject_Add_StepDependencies(peer_integration_find_package build peer)

include(GNUInstallDirs)

enable_testing()

add_test(
  NAME peer_integration_find_package-run
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/install/${CMAKE_INSTALL_BINDIR}/peer_integration_find_package${CMAKE_EXECUTABLE_SUFFIX})

set_tests_properties(
  peer_integration_find_package-run
  PROPERTIES
  TIMEOUT "15")
