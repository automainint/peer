#include "peer.h"

#include <assert.h>
#include <string.h>

static_assert(PEER_CIPHER_KEY_SIZE < PEER_MAX_MESSAGE_SIZE,
              "We should be able to send a message with cipher key");
static_assert(PEER_MAX_MESSAGE_SIZE < 65536,
              "Max message size sanity check");

typedef struct {
  uint64_t index;
  uint16_t mode;
} peer_packet_header_t;

typedef struct {
  uint64_t checksum[2];
  uint64_t index;
  uint64_t actor;
  uint32_t id;
  uint16_t mode;
  uint16_t size;
} peer_message_header_t;

kit_status_t peer_init(peer_t *const         peer,
                       kit_allocator_t const alloc) {
  assert(peer != NULL);

  if (peer == NULL)
    return PEER_ERROR_INVALID_PEER;

  memset(peer, 0, sizeof *peer);
  DA_INIT(peer->slots, 0, alloc);
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

kit_status_t peer_open(peer_t *const peer, peer_ids_ref_t const ids) {
  assert(peer != NULL);

  if (peer == NULL)
    return PEER_ERROR_INVALID_PEER;

  ptrdiff_t const n = peer->slots.size;

  DA_RESIZE(peer->slots, n + ids.size);
  if (peer->slots.size != n + ids.size)
    return PEER_ERROR_BAD_ALLOC;

  for (ptrdiff_t i = 0; i < ids.size; i++)
    peer->slots.values[n + i].local.id = ids.values[i];

  return KIT_OK;
}

kit_status_t peer_destroy(peer_t *const peer) {
  assert(peer != NULL);

  if (peer == NULL)
    return PEER_ERROR_INVALID_PEER;

  DA_DESTROY(peer->slots);
  DA_DESTROY(peer->queue);
  DA_DESTROY(peer->buffer);
  return KIT_OK;
}

kit_status_t peer_queue(peer_t *const            peer,
                        peer_message_ref_t const data) {
  assert(peer != NULL);

  if (peer == NULL)
    return PEER_ERROR_INVALID_PEER;

  return PEER_ERROR_NOT_IMPLEMENTED;
}

kit_status_t peer_connect(peer_t *const   client,
                          ptrdiff_t const server_id) {
  assert(client != NULL);

  if (client == NULL)
    return PEER_ERROR_INVALID_PEER;

  return PEER_ERROR_NOT_IMPLEMENTED;
}

kit_status_t peer_input(peer_t *const            peer,
                        peer_packets_ref_t const packets) {
  assert(peer != NULL);

  if (peer == NULL)
    return PEER_ERROR_INVALID_PEER;

  return PEER_ERROR_NOT_IMPLEMENTED;
}

peer_tick_result_t peer_tick(peer_t *const     peer,
                             peer_time_t const time_elapsed) {
  assert(peer != NULL);

  peer_tick_result_t result;
  memset(&result, 0, sizeof result);

  if (peer == NULL) {
    result.status = PEER_ERROR_INVALID_PEER;
    return result;
  }

  result.status = PEER_ERROR_NOT_IMPLEMENTED;
  DA_INIT(result.packets, 0, peer->buffer.alloc);

  return result;
}

