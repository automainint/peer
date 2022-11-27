target_sources(
  ${PEER_LIBRARY}
    PRIVATE
      peer.c
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/peer.h>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/packet.h>)
