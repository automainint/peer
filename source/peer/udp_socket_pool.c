#ifndef PEER_DISABLE_SYSTEM_SOCKETS
#  include "udp_socket_pool.h"

#  include <assert.h>

kit_status_t peer_udp_init(peer_udp_socket_pool_t *const pool,
                           kit_allocator_t const         alloc) {
  assert(pool != NULL);

  return PEER_ERROR_NOT_IMPLEMENTED;
}

kit_status_t peer_udp_destroy(peer_udp_socket_pool_t *const pool) {
  assert(pool != NULL);

  return PEER_ERROR_NOT_IMPLEMENTED;
}

kit_status_t peer_udp_ipv4_open(peer_udp_socket_pool_t *const pool,
                                peer_t *const                 peer,
                                uint16_t const                port,
                                ptrdiff_t const               count) {
  assert(pool != NULL);
  assert(peer != NULL);
  assert(count > 0);

  return PEER_ERROR_NOT_IMPLEMENTED;
}

kit_status_t peer_udp_ipv6_open(peer_udp_socket_pool_t *const pool,
                                peer_t *const                 peer,
                                uint16_t const                port,
                                ptrdiff_t const               count) {
  assert(pool != NULL);
  assert(peer != NULL);
  assert(count > 0);

  return PEER_ERROR_NOT_IMPLEMENTED;
}

kit_status_t peer_udp_ipv4_connect(peer_udp_socket_pool_t *const pool,
                                   peer_t *const                 peer,
                                   kit_str_t const address,
                                   uint16_t const  port) {
  assert(pool != NULL);
  assert(peer != NULL);
  assert(port != PEER_ANY_PORT);

  return PEER_ERROR_NOT_IMPLEMENTED;
}

kit_status_t peer_udp_ipv6_connect(peer_udp_socket_pool_t *const pool,
                                   peer_t *const                 peer,
                                   kit_str_t const address,
                                   uint16_t const  port) {
  assert(pool != NULL);
  assert(peer != NULL);
  assert(address.size > 0 && address.values != NULL);
  assert(port != PEER_ANY_PORT);

  return PEER_ERROR_NOT_IMPLEMENTED;
}

kit_status_t peer_udp_tick(peer_udp_socket_pool_t *const pool,
                           peer_t *const                 peer,
                           peer_time_t const time_elapsed) {
  assert(pool != NULL);
  assert(peer != NULL);

  return PEER_ERROR_NOT_IMPLEMENTED;
}
#endif
