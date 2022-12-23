#ifndef PEER_UDP_SOCKET_POOL_H
#define PEER_UDP_SOCKET_POOL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int x;
} peer_udp_socket_pool_t;

#ifndef PEER_DISABLE_SHORT_NAMES
#  define udp_socket_pool_t peer_udp_socket_pool_t
#endif

#ifdef __cplusplus
}
#endif

#endif
