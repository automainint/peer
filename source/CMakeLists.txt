add_subdirectory(peer)

if(PEER_ENABLE_TESTING)
  add_subdirectory(test)
endif()
