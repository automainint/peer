cmake_minimum_required(VERSION 3.16)

option(PEER_ENABLE_TESTING "Enable testing" ON)

set(PEER_PROJECT peer)
set(PEER_LIBRARY peer)
set(PEER_TEST peer_test)
set(PEER_TARGETS peer-targets)

if(NOT DEFINED CMAKE_BUILD_PARALLEL_LEVEL)
  set(CMAKE_BUILD_PARALLEL_LEVEL 4)
endif()

project(
  ${PEER_PROJECT}
  VERSION 0.1.1
  DESCRIPTION "Peer"
  LANGUAGES C)

include(FetchContent)

find_package(Threads)
find_package(kit QUIET)

if(NOT kit_FOUND)
  FetchContent_Declare(
    kit
    GIT_REPOSITORY https://github.com/automainint/kit.git
    GIT_TAG dev)
  set(KIT_ENABLE_TESTING OFF)
  FetchContent_MakeAvailable(kit)
endif()

add_library(${PEER_LIBRARY} STATIC)
add_library(${PEER_PROJECT}::${PEER_LIBRARY} ALIAS ${PEER_LIBRARY})
target_include_directories(
  ${PEER_LIBRARY} INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source>)
target_compile_features(${PEER_LIBRARY} PUBLIC c_std_11)
target_link_libraries(${PEER_LIBRARY} PUBLIC Threads::Threads kit::kit)
if(WIN32)
  target_link_libraries(${PEER_LIBRARY} PUBLIC ws2_32)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  #target_compile_options(${PEER_LIBRARY} PUBLIC -pedantic -Wall -Werror)
endif()

enable_testing()

if(PEER_ENABLE_TESTING)
  find_package(Threads REQUIRED)

  add_executable(${PEER_TEST})
  add_executable(${PEER_PROJECT}::${PEER_TEST} ALIAS ${PEER_TEST})
  target_compile_features(${PEER_TEST} PRIVATE c_std_11)
  target_link_libraries(${PEER_TEST} PRIVATE ${PEER_LIBRARY} kit::kit_test Threads::Threads)

  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(
      ${PEER_LIBRARY} PUBLIC
      -fsanitize=undefined,address
      --coverage -O0 -g)
    target_link_options(
      ${PEER_LIBRARY} PUBLIC
      -fsanitize=undefined,address
      --coverage)
  endif()

  add_test(
    NAME ${PEER_LIBRARY}_unittests
    COMMAND ${PEER_TEST} --no-carriage-return)

  set_tests_properties(
    ${PEER_LIBRARY}_unittests
    PROPERTIES
    TIMEOUT "120")
endif()

add_subdirectory(source)

include(GNUInstallDirs)

file(
  GLOB_RECURSE headers_
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/source/peer
  ${CMAKE_CURRENT_SOURCE_DIR}/source/peer/*.h)

foreach(path_ ${headers_})
  get_filename_component(dir_ "${path_}" DIRECTORY)

  install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/source/peer/${path_}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/peer/${dir_})
endforeach()

unset(headers_)
unset(path_)
unset(dir_)

install(
  TARGETS ${PEER_LIBRARY} ${PEER_TEST_LIBRARY}
  EXPORT ${PEER_TARGETS}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(
  EXPORT ${PEER_TARGETS}
  FILE ${PEER_TARGETS}.cmake
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

export(
  EXPORT ${PEER_TARGETS}
  FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/${PEER_TARGETS}.cmake"
  NAMESPACE ${PROJECT_NAME}::)

include(CMakePackageConfigHelpers)

string(TOLOWER ${PROJECT_NAME} project_lower_)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/${project_lower_}-config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/${project_lower_}-config-version.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY AnyNewerVersion)

install(
  FILES
  "${CMAKE_CURRENT_BINARY_DIR}/${project_lower_}-config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/${project_lower_}-config-version.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

unset(project_lower_)
