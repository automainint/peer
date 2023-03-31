#ifndef PEER_SOCKET_POOL_H
#define PEER_SOCKET_POOL_H

#include "peer.h"
#include "sockets.h"
#include <kit/dynamic_array.h>
#include <kit/string_ref.h>

#ifndef PEER_DISABLE_SYSTEM_SOCKETS
#  ifdef __cplusplus
extern "C" {
#  endif

enum {
  PEER_UDP_IPv4,
  PEER_UDP_IPv6,
  PEER_TCP_IPv4,
  PEER_TCP_IPv6,
  PEER_ANY_PORT = 0xffff
};

typedef struct {
  socket_t  socket;
  int       protocol;
  uint16_t  local_port;
  uint16_t  remote_port;
  ptrdiff_t remote_address_size;
  uint8_t   remote_address[PEER_ADDRESS_SIZE - 2];
} peer_node_t;

typedef KIT_DA(peer_node_t) peer_nodes_t;

typedef struct {
  kit_allocator_t alloc;
  peer_nodes_t    nodes;
} peer_socket_pool_t;

/*  Helper functions to glue the peer library and system sockets.
 */

kit_status_t peer_pool_init(peer_socket_pool_t *pool,
                            kit_allocator_t     alloc);

kit_status_t peer_pool_destroy(peer_socket_pool_t *pool);

kit_status_t peer_pool_open(peer_socket_pool_t *pool, peer_t *peer,
                            int protocol, uint16_t port,
                            ptrdiff_t count);

kit_status_t peer_pool_connect(peer_socket_pool_t *pool, peer_t *peer,
                               int protocol, kit_str_t address,
                               uint16_t port);

kit_status_t peer_pool_tick(peer_socket_pool_t *pool, peer_t *peer,
                            peer_time_t time_elapsed);

#  ifdef __cplusplus
}
#  endif
#endif

#endif
