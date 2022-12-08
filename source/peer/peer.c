#include "peer.h"

#include <string.h>

kit_status_t peer_init(peer_t *const         peer,
                       kit_allocator_t const alloc) {
  memset(peer, 0, sizeof *peer);
  DA_INIT(peer->sockets, 0, alloc);
  DA_INIT(peer->queue, 0, alloc);
  DA_INIT(peer->buffer, 0, alloc);
  return KIT_OK;
}

kit_status_t peer_init_host(peer_t *const         host,
                            kit_allocator_t const alloc) {
  return peer_init(host, alloc);
}

kit_status_t peer_init_client(peer_t *const         client,
                              kit_allocator_t const alloc) {
  return peer_init(client, alloc);
}

kit_status_t peer_open(peer_t *const              peer,
                       peer_addresses_ref_t const sockets) {
  ptrdiff_t const n = peer->sockets.size;

  DA_RESIZE(peer->sockets, n + sockets.size);
  if (peer->sockets.size != n + sockets.size)
    return PEER_ERROR_BAD_ALLOC;

  for (ptrdiff_t i = 0; i < sockets.size; i++)
    peer->sockets.values[n + i] = sockets.values[i];

  return KIT_OK;
}

kit_status_t peer_destroy(peer_t *const peer) {
  DA_DESTROY(peer->sockets);
  DA_DESTROY(peer->queue);
  DA_DESTROY(peer->buffer);
  return KIT_OK;
}

kit_status_t peer_queue(peer_t *const         peer,
                        peer_data_ref_t const data) {
  return PEER_ERROR_NOT_IMPLEMENTED;
}

kit_status_t peer_connect(peer_t *const        client,
                          peer_address_t const server_address) {
  return PEER_ERROR_NOT_IMPLEMENTED;
}

kit_status_t peer_input(peer_t *const            peer,
                        peer_packets_ref_t const packets) {
  return PEER_ERROR_NOT_IMPLEMENTED;
}

peer_tick_result_t peer_tick(peer_t *const     peer,
                             peer_time_t const time_elapsed) {
  peer_tick_result_t result;
  memset(&result, 0, sizeof result);

  result.status = PEER_ERROR_NOT_IMPLEMENTED;
  DA_INIT(result.packets, 0, peer->buffer.alloc);

  return result;
}

