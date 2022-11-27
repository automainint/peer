#include "peer.h"

kit_status_t peer_init_host(peer_t *host, peer_address_t address,
                            kit_allocator_t alloc) {
  return PEER_ERROR_NOT_IMPLEMENTED;
}

kit_status_t peer_init_client(peer_t *client, peer_address_t address,
                              kit_allocator_t alloc) {
  return PEER_ERROR_NOT_IMPLEMENTED;
}

kit_status_t peer_destroy(peer_t *peer) {
  return PEER_ERROR_NOT_IMPLEMENTED;
}

kit_status_t peer_queue(peer_t *peer, peer_data_ref_t data) {
  return PEER_ERROR_NOT_IMPLEMENTED;
}

kit_status_t peer_connect(peer_t        *client,
                          peer_address_t server_address) {
  return PEER_ERROR_NOT_IMPLEMENTED;
}

kit_status_t peer_input(peer_t *peer, peer_packets_ref_t packets) {
  return PEER_ERROR_NOT_IMPLEMENTED;
}

peer_packets_t peer_tick(peer_t *peer, peer_time_t time_elapsed) {
  peer_packets_t packets = { .size     = 0,
                             .capacity = 0,
                             .values   = NULL };

  peer->status = PEER_ERROR_NOT_IMPLEMENTED;

  return packets;
}

