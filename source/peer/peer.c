#include "peer.h"

#include "serial.h"
#include <assert.h>
#include <string.h>

static_assert(((ptrdiff_t) -1) == PEER_UNDEFINED,
              "Undefine id sanity check");
static_assert(PEER_N_PACKET_MESSAGES + PEER_N_MESSAGE_DATA <
                  PEER_PACKET_SIZE,
              "We should be able to put messages in packets");
static_assert(
    2 + PEER_MT64_KEY_SIZE < PEER_MAX_MESSAGE_SIZE,
    "We should be able to send a message with a cipher key");
static_assert(PEER_PACKET_SIZE < 65536, "Packet size sanity check");
static_assert(PEER_MAX_MESSAGE_SIZE < 1024,
              "Max message size sanity check");

kit_status_t peer_init(peer_t *const peer, peer_mode_t const mode,
                       kit_allocator_t const alloc) {
  assert(peer != NULL);
  assert(mode == PEER_HOST || mode == PEER_CLIENT);

  if (peer == NULL)
    return PEER_ERROR_INVALID_PEER;

  memset(peer, 0, sizeof *peer);
  peer->mode = mode;
  DA_INIT(peer->slots, 0, alloc);
  DA_INIT(peer->queue, 0, alloc);
  DA_INIT(peer->buffer, 0, alloc);
  return KIT_OK;
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
    slot->remote.id = PEER_UNDEFINED;
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

  if (peer->mode == PEER_HOST) {
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
     *  Determine correct time and actor values.
     */

    memset(peer->queue.values + index, 0, sizeof *peer->queue.values);

    peer->queue.values[index].time   = 0;
    peer->queue.values[index].actor  = 0;
    peer->queue.values[index].size   = message.size;
    peer->queue.values[index].offset = offset;
  }

  if (peer->mode == PEER_CLIENT) {
    /*  FIXME
     *  Put messages to the personal queue for the client mode.
     */

    return PEER_ERROR_NOT_IMPLEMENTED;
  }

  return KIT_OK;
}

kit_status_t peer_connect(peer_t *const   client,
                          ptrdiff_t const server_id) {
  assert(client != NULL);

  if (client == NULL)
    return PEER_ERROR_INVALID_PEER;

  for (ptrdiff_t i = 0; i < client->slots.size; i++) {
    peer_slot_t *const slot = client->slots.values + i;

    if (slot->remote.id == PEER_UNDEFINED) {
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

  kit_allocator_t const alloc = peer->buffer.alloc;

  kit_status_t status = KIT_OK;

  for (ptrdiff_t i = 0; i < packets.size; i++) {
    peer_packet_t const *const packet = packets.values + i;

    int slot_found = 0;

    for (ptrdiff_t j = 0; j < peer->slots.size; j++) {
      peer_slot_t *const slot = peer->slots.values + j;

      if (slot->local.id != packet->destination_id ||
          slot->remote.id != packet->source_id)
        continue;

      peer_messages_t messages;
      DA_INIT(messages, 0, alloc);

      peer_packets_ref_t const ref = { .size = 1, .values = packet };

      status |= peer_unpack(ref, &messages);

      for (ptrdiff_t k = 0; k < messages.size; k++) {
        peer_message_t *const message = messages.values + k;

        /*  FIXME
         *  Check the checksum.
         */
        ptrdiff_t const data_size = peer_read_message_data_size(
            message->values);
        uint8_t const mode = peer_read_message_mode(message->values);
        ptrdiff_t const index = peer_read_message_index(
            message->values);
        peer_time_t const time = peer_read_message_time(
            message->values);
        uint32_t const actor = peer_read_message_actor(
            message->values);

        if (mode == PEER_MESSAGE_MODE_SERVICE && data_size >= 1) {
          uint8_t const service_id = peer_read_u8(
              message->values + PEER_N_MESSAGE_DATA);

          if (service_id == PEER_M_SESSION_RESPONSE) {
            assert(data_size - 1 <= PEER_ADDRESS_SIZE);
            slot->remote.address_size = data_size - 1;
            memcpy(slot->remote.address_data,
                   message->values + PEER_N_MESSAGE_DATA + 1,
                   data_size - 1);
          }
        }

        if (index != PEER_UNDEFINED) {
          ptrdiff_t const offset = peer->buffer.size;

          DA_RESIZE(peer->buffer, offset + data_size);
          if (peer->buffer.size != offset + data_size) {
            status |= PEER_ERROR_BAD_ALLOC;
            break;
          }

          if (data_size > 0)
            memcpy(peer->buffer.values + offset,
                   message->values + PEER_N_MESSAGE_DATA, data_size);

          ptrdiff_t const n = peer->queue.size;

          if (index >= n) {
            DA_RESIZE(peer->queue, index + 1);
            if (peer->queue.size != index + 1) {
              status |= PEER_ERROR_BAD_ALLOC;
              break;
            }

            memset(peer->queue.values + n, 0,
                   (index + 1 - n) * sizeof *peer->queue.values);
          }

          peer->queue.values[index].time   = time;
          peer->queue.values[index].actor  = actor;
          peer->queue.values[index].size   = data_size;
          peer->queue.values[index].offset = offset;
        }
      }

      for (ptrdiff_t k = 0; k < messages.size; k++)
        DA_DESTROY(messages.values[k]);
      DA_DESTROY(messages);

      slot_found = 1;
      break;
    }

    if (slot_found || peer->mode != PEER_HOST ||
        peer->slots.size == 0 ||
        peer->slots.values[0].remote.id != PEER_UNDEFINED)
      continue;

    /*  Assign a slot for the client.
     */

    for (ptrdiff_t j = 1; j < peer->slots.size; j++) {
      peer_slot_t *const slot = peer->slots.values + j;

      if (slot->remote.id == PEER_UNDEFINED &&
          slot->local.address_size > 0) {
        slot->state     = PEER_SLOT_SESSION_REQUEST;
        slot->remote.id = packet->source_id;
        break;
      }
    }
  }

  return status;
}

peer_tick_result_t peer_tick(peer_t *const     peer,
                             peer_time_t const time_elapsed) {
  assert(peer != NULL);
  assert(time_elapsed >= 0);

  kit_allocator_t const alloc = peer->buffer.alloc;

  peer_tick_result_t result;
  memset(&result, 0, sizeof result);
  DA_INIT(result.packets, 0, alloc);

  if (peer == NULL) {
    result.status = PEER_ERROR_INVALID_PEER;
    return result;
  }

  if (time_elapsed < 0) {
    result.status = PEER_ERROR_INVALID_TIME_ELAPSED;
    return result;
  }

  result.status = KIT_OK;

  if (peer->mode == PEER_HOST) {
    ptrdiff_t const size = peer->queue.size;

    DA(peer_message_t) messages;
    DA(peer_message_ref_t) refs;

    DA_INIT(messages, size, alloc);
    DA_INIT(refs, size, alloc);

    if (messages.size == size && refs.size == size) {
      memset(messages.values, 0, size * sizeof *messages.values);

      for (ptrdiff_t i = 0; i < size; i++) {
        ptrdiff_t const full_size = PEER_N_MESSAGE_DATA +
                                    peer->queue.values[i].size;

        DA_INIT(messages.values[i], full_size, alloc);
        if (messages.values[i].size != full_size) {
          result.status = PEER_ERROR_BAD_ALLOC;
          break;
        }

        peer_write_message(
            messages.values[i].values, PEER_MESSAGE_MODE_APPLICATION,
            i, 0, 0, peer->queue.values[i].size,
            peer->buffer.values + peer->queue.values[i].offset);
      }

      for (ptrdiff_t i = 0; i < size; i++) {
        refs.values[i].size   = messages.values[i].size;
        refs.values[i].values = messages.values[i].values;
      }

      peer_messages_ref_t mref = { .size   = refs.size,
                                   .values = refs.values };

      for (ptrdiff_t i = 1; i < peer->slots.size; i++) {
        peer_slot_t const *const slot = peer->slots.values + i;

        if (slot->state == PEER_SLOT_READY)
          result.status |= peer_pack(slot->local.id, slot->remote.id,
                                     mref, &result.packets);
      }

    } else
      result.status = PEER_ERROR_BAD_ALLOC;

    for (ptrdiff_t i = 0; i < messages.size; i++)
      DA_DESTROY(messages.values[i]);

    DA_DESTROY(messages);
    DA_DESTROY(refs);

    for (ptrdiff_t i = 1; i < peer->slots.size; i++) {
      peer_slot_t *const slot = peer->slots.values + i;

      if (slot->state == PEER_SLOT_SESSION_REQUEST) {
        uint8_t message[PEER_N_MESSAGE_DATA + 1 + PEER_ADDRESS_SIZE];
        uint8_t data[1 + PEER_ADDRESS_SIZE];
        data[0] = PEER_M_SESSION_RESPONSE;
        memcpy(data + 1, slot->local.address_data,
               slot->local.address_size);
        peer_write_message(message, PEER_MESSAGE_MODE_SERVICE,
                           PEER_UNDEFINED, 0, 0,
                           slot->local.address_size + 1, data);

        peer_message_ref_t const ref = {
          .size = PEER_N_MESSAGE_DATA + 1 + slot->local.address_size,
          .values = message
        };

        peer_messages_ref_t const mref = { .size   = 1,
                                           .values = &ref };

        result.status |= peer_pack(peer->slots.values[0].local.id,
                                   slot->remote.id, mref,
                                   &result.packets);

        slot->state = PEER_SLOT_READY;
      }
    }
  }

  if (peer->mode == PEER_CLIENT) {
    peer_messages_ref_t mref = { .size = 0, .values = NULL };

    if (peer->slots.size > 0)
      result.status |= peer_pack(peer->slots.values[0].local.id,
                                 peer->slots.values[0].remote.id,
                                 mref, &result.packets);
  }

  return result;
}
