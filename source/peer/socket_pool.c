#include "socket_pool.h"

#include <assert.h>

#ifndef PEER_DISABLE_SYSTEM_SOCKETS
kit_status_t peer_pool_init(peer_socket_pool_t *const pool,
                            kit_allocator_t const     alloc) {
  assert(pool != NULL);

  if (pool == NULL)
    return PEER_ERROR_INVALID_POOL;

  DA_INIT(pool->nodes, 0, alloc);

  return PEER_ERROR_NOT_IMPLEMENTED;
}

kit_status_t peer_pool_destroy(peer_socket_pool_t *const pool) {
  assert(pool != NULL);

  if (pool == NULL)
    return PEER_ERROR_INVALID_POOL;

  DA_DESTROY(pool->nodes);

  return PEER_ERROR_NOT_IMPLEMENTED;
}

kit_status_t peer_pool_open(peer_socket_pool_t *const pool,
                            peer_t *const peer, int const protocol,
                            uint16_t const  port,
                            ptrdiff_t const count) {
  assert(pool != NULL);
  assert(peer != NULL);
  assert(count > 0);

  switch (protocol) {
    case PEER_UDP_IPv4: break;
    default: return PEER_ERROR_UNKNOWN_PROTOCOL;
  }

  return PEER_ERROR_NOT_IMPLEMENTED;
}

kit_status_t peer_pool_connect(peer_socket_pool_t *const pool,
                               peer_t *const peer, int const protocol,
                               kit_str_t const address,
                               uint16_t const  port) {
  assert(pool != NULL);
  assert(peer != NULL);
  assert(port != PEER_ANY_PORT);

  switch (protocol) {
    case PEER_UDP_IPv4: break;
    default: return PEER_ERROR_UNKNOWN_PROTOCOL;
  }

  return PEER_ERROR_NOT_IMPLEMENTED;
}

kit_status_t peer_pool_tick(peer_socket_pool_t *const pool,
                            peer_t *const             peer,
                            peer_time_t const         time_elapsed) {
  assert(pool != NULL);
  assert(peer != NULL);

  return PEER_ERROR_NOT_IMPLEMENTED;
}
#endif
