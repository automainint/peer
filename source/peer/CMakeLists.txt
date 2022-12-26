target_sources(
  peer
    PRIVATE
      cipher.c packet.c udp_socket_pool.c serial.c peer.c
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/peer.h>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/serial.h>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/udp_socket_pool.h>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/options.h>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/packet.h>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/cipher.h>)
