#ifndef PEER_UDP_SOCKET_POOL_H
#define PEER_UDP_SOCKET_POOL_H

#include "peer.h"
#include "sockets.h"
#include <kit/dynamic_array.h>
#include <kit/string_ref.h>

#ifndef PEER_DISABLE_SYSTEM_SOCKETS
#  ifdef __cplusplus
extern "C" {
#  endif

enum { PEER_ANY_PORT = (uint16_t) -1 };

typedef struct {
  socket_t  socket;
  uint16_t  local_port;
  uint16_t  remote_port;
  ptrdiff_t remote_address_size;
  uint8_t   remote_address[PEER_ADDRESS_SIZE - 2];
} peer_udp_node_t;

typedef KIT_DA(peer_udp_node_t) peer_udp_nodes_t;

typedef struct {
  kit_allocator_t  alloc;
  peer_udp_nodes_t nodes;
} peer_udp_socket_pool_t;

/*  Helper functions to glue the peer library and system sockets.
 */

kit_status_t peer_udp_init(peer_udp_socket_pool_t *pool,
                           kit_allocator_t         alloc);

kit_status_t peer_udp_destroy(peer_udp_socket_pool_t *pool);

kit_status_t peer_udp_ipv4_open(peer_udp_socket_pool_t *pool,
                                peer_t *peer, uint16_t port,
                                ptrdiff_t count);

kit_status_t peer_udp_ipv6_open(peer_udp_socket_pool_t *pool,
                                peer_t *peer, uint16_t port,
                                ptrdiff_t count);

kit_status_t peer_udp_ipv4_connect(peer_udp_socket_pool_t *pool,
                                   peer_t *peer, kit_str_t address,
                                   uint16_t port);

kit_status_t peer_udp_ipv6_connect(peer_udp_socket_pool_t *pool,
                                   peer_t *peer, kit_str_t address,
                                   uint16_t port);

kit_status_t peer_udp_tick(peer_udp_socket_pool_t *pool, peer_t *peer,
                           peer_time_t time_elapsed);

#  ifdef __cplusplus
}
#  endif
#endif

#endif
