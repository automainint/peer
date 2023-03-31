target_sources(
  peer
    PRIVATE
      cipher.c packet.c socket_pool.c peer.c
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/peer.h>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/socket_pool.h>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/sockets.h>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/serial.h>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/options.h>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/packet.h>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/cipher.h>)
