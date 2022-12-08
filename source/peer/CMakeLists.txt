target_sources(
  peer
    PRIVATE
      packet.c peer.c
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/peer.h>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/packet.h>)
