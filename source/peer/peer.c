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
} packet_header_t;

typedef struct {
  uint64_t checksum[2];
  uint64_t index;
  uint64_t time;
  uint64_t actor;
  uint16_t mode;
  uint16_t size;
} message_header_t;

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
  assert(ids.size == 0 || ids.values != NULL);

  if (peer == NULL)
    return PEER_ERROR_INVALID_PEER;
  if (ids.size == 0)
    return KIT_OK;
  if (ids.values == NULL)
    return PEER_ERROR_INVALID_IDS;

  ptrdiff_t const n = peer->slots.size;

  DA_RESIZE(peer->slots, n + ids.size);
  if (peer->slots.size != n + ids.size)
    return PEER_ERROR_BAD_ALLOC;

  memset(peer->slots.values + n, 0,
         ids.size * sizeof *peer->slots.values);

  for (ptrdiff_t i = 0; i < ids.size; i++) {
    peer_slot_t *slot = peer->slots.values + (n + i);

    slot->local.id  = ids.values[i];
    slot->remote.id = PEER_ID_UNDEFINED;
  }

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
                        peer_message_ref_t const message) {
  assert(peer != NULL);
  assert(message.size == 0 || message.values != NULL);

  if (peer == NULL)
    return PEER_ERROR_INVALID_PEER;
  if (message.size == 0)
    return KIT_OK;
  if (message.values == NULL)
    return PEER_ERROR_INVALID_MESSAGE;

  /*  FIXME
   *
   *  Only host should be able to add messages to the shared queue.
   */

  ptrdiff_t const offset = peer->buffer.size;

  DA_RESIZE(peer->buffer, offset + message.size);
  if (peer->buffer.size != offset + message.size)
    return PEER_ERROR_BAD_ALLOC;

  if (message.size > 0)
    memcpy(peer->buffer.values + offset, message.values,
           message.size);

  ptrdiff_t index = peer->queue.size;
  DA_RESIZE(peer->queue, index + 1);
  if (peer->queue.size != index + 1)
    return PEER_ERROR_BAD_ALLOC;

  /*  FIXME
   *
   *  Determine correct time and actor values.
   */

  memset(peer->queue.values + index, 0, sizeof *peer->queue.values);

  peer->queue.values[index].time   = 0;
  peer->queue.values[index].actor  = 0;
  peer->queue.values[index].size   = message.size;
  peer->queue.values[index].offset = offset;

  return KIT_OK;
}

kit_status_t peer_connect(peer_t *const   client,
                          ptrdiff_t const server_id) {
  assert(client != NULL);

  if (client == NULL)
    return PEER_ERROR_INVALID_PEER;

  for (ptrdiff_t i = 0; i < client->slots.size; i++) {
    peer_slot_t *const slot = client->slots.values + i;

    if (slot->remote.id == PEER_ID_UNDEFINED) {
      slot->remote.id           = server_id;
      slot->remote.address_size = 0;

      return KIT_OK;
    }
  }

  return PEER_ERROR_NO_FREE_SLOTS;
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

