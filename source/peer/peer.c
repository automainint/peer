#include "peer.h"

#include "serial.h"
#include <assert.h>
#include <string.h>

static_assert(((ptrdiff_t) -1) == PEER_UNDEFINED,
              "Undefined id sanity check");
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
  if (mode != PEER_HOST && mode != PEER_CLIENT)
    return PEER_ERROR_INVALID_MODE;

  memset(peer, 0, sizeof *peer);
  peer->alloc = alloc;
  peer->mode  = mode;

  DA_INIT(peer->slots, 0, alloc);
  DA_INIT(peer->queue, 0, alloc);

  if (mode == PEER_HOST) {
    /*  Actor id is a host's slot index corresponding to the peer.
     *  First slot is always reserved for host itself.
     */
    peer->actor = 0;
  } else {
    /*  Client doesn't have an actor id at startup.
     */
    peer->actor = PEER_UNDEFINED;
  }

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
    slot->actor     = i;

    DA_INIT(slot->queue, 0, peer->alloc);
  }

  return KIT_OK;
}

static void queue_destroy(peer_queue_t *const q) {
  for (ptrdiff_t i = 0; i < q->size; i++)
    DA_DESTROY(q->values[i].data);
  DA_DESTROY(*q);
}

kit_status_t peer_destroy(peer_t *const peer) {
  assert(peer != NULL);

  if (peer == NULL)
    return PEER_ERROR_INVALID_PEER;

  for (ptrdiff_t i = 0; i < peer->slots.size; i++)
    queue_destroy(&peer->slots.values[i].queue);

  DA_DESTROY(peer->slots);

  queue_destroy(&peer->queue);

  return KIT_OK;
}

static kit_status_t queue_append(peer_queue_t *const q,
                                 peer_time_t time, ptrdiff_t actor,
                                 peer_message_ref_t const message,
                                 kit_allocator_t          alloc) {
  ptrdiff_t index = q->size;
  DA_RESIZE(*q, index + 1);
  if (q->size != index + 1)
    return PEER_ERROR_BAD_ALLOC;

  memset(q->values + index, 0, sizeof *q->values);

  q->values[index].time  = time;
  q->values[index].actor = actor;

  DA_INIT(q->values[index].data, message.size, alloc);
  if (q->values[index].data.size != message.size) {
    DA_RESIZE(*q, index);
    return PEER_ERROR_BAD_ALLOC;
  }

  if (message.size > 0)
    memcpy(q->values[index].data.values, message.values,
           message.size);

  return KIT_OK;
}

kit_status_t peer_queue(peer_t *const            peer,
                        peer_message_ref_t const message) {
  assert(peer != NULL);
  assert(message.size == 0 || message.values != NULL);

  if (peer == NULL)
    return PEER_ERROR_INVALID_PEER;
  if (message.size == 0 && message.values == NULL)
    return PEER_ERROR_INVALID_MESSAGE;

  peer_time_t const time  = 0;
  ptrdiff_t const   actor = peer->actor;

  switch (peer->mode) {
    case PEER_HOST:
      return queue_append(&peer->queue, time, actor, message,
                          peer->alloc);

    case PEER_CLIENT:
      if (peer->slots.size > 0)
        return queue_append(&peer->slots.values[0].queue, time, actor,
                            message, peer->alloc);

    default:;
  }

  return PEER_ERROR_INVALID_MODE;
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
      DA_INIT(messages, 0, peer->alloc);

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

        if (mode == PEER_MESSAGE_MODE_SERVICE &&
            peer->mode == PEER_CLIENT && data_size >= 1) {
          uint8_t const service_id = peer_read_u8(
              message->values + PEER_N_MESSAGE_DATA);

          if (service_id == PEER_M_SESSION_RESPONSE) {
            assert(data_size - 1 <= PEER_ADDRESS_SIZE);
            peer->actor               = actor;
            slot->remote.address_size = data_size - 1;
            memcpy(slot->remote.address_data,
                   message->values + PEER_N_MESSAGE_DATA + 1,
                   data_size - 1);
          }
        }

        if (index == PEER_UNDEFINED)
          continue;

        assert(index >= 0);

        if (index < 0) {
          status |= PEER_ERROR_INVALID_MESSAGE_INDEX;
          continue;
        }

        if (index < peer->queue.size &&
            peer->queue.values[index].is_ready) {
          /*  FIXME
           *  Check if message is the same.
           */

        } else {
          /*  Add message to the queue.
           */

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

          DA_INIT(peer->queue.values[index].data, data_size,
                  peer->alloc);
          if (peer->queue.values[index].data.size != data_size) {
            status |= PEER_ERROR_BAD_ALLOC;
            break;
          }

          peer->queue.values[index].is_ready = 1;
          peer->queue.values[index].time     = time;
          peer->queue.values[index].actor    = actor;

          if (data_size > 0)
            memcpy(peer->queue.values[index].data.values,
                   message->values + PEER_N_MESSAGE_DATA, data_size);
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
        slot->actor     = j;
        break;
      }
    }
  }

  return status;
}

static kit_status_t queue_pack(peer_queue_t const *const q,
                               ptrdiff_t const           index,
                               ptrdiff_t                 source_id,
                               ptrdiff_t             destination_id,
                               peer_packets_t *const out_packets,
                               kit_allocator_t       alloc) {
  ptrdiff_t const size = q->size - index;

  assert(size >= 0);

  if (size < 0)
    return PEER_ERROR_INVALID_OUT_INDEX;

  if (size == 0)
    return KIT_OK;

  peer_messages_t     messages;
  peer_message_refs_t refs;

  DA_INIT(messages, size, alloc);
  DA_INIT(refs, size, alloc);

  assert(messages.size == size);
  assert(refs.size == size);

  if (messages.size != size || refs.size != size)
    return PEER_ERROR_BAD_ALLOC;

  if (messages.size != 0)
    memset(messages.values, 0, size * sizeof *messages.values);

  /*  Prepare messages' data.
   */

  for (ptrdiff_t i = 0; i < size; i++) {
    assert(index + i >= 0 && index + i < q->size);

    peer_message_entry_t const *const entry = q->values + (index + i);

    ptrdiff_t const full_size = PEER_N_MESSAGE_DATA +
                                entry->data.size;

    DA_INIT(messages.values[i], full_size, alloc);
    if (messages.values[i].size != full_size)
      return PEER_ERROR_BAD_ALLOC;

    peer_write_message(messages.values[i].values,
                       PEER_MESSAGE_MODE_APPLICATION, index + i,
                       entry->time, entry->actor, entry->data.size,
                       entry->data.values);
  }

  /*  Pack messages into packets.
   */

  for (ptrdiff_t i = 0; i < size; i++) {
    refs.values[i].size   = messages.values[i].size;
    refs.values[i].values = messages.values[i].values;
  }

  peer_messages_ref_t mref = { .size   = refs.size,
                               .values = refs.values };

  kit_status_t const result = peer_pack(source_id, destination_id,
                                        mref, out_packets);

  for (ptrdiff_t i = 0; i < messages.size; i++)
    DA_DESTROY(messages.values[i]);

  DA_DESTROY(messages);
  DA_DESTROY(refs);

  return result;
}

peer_tick_result_t peer_tick(peer_t *const     peer,
                             peer_time_t const time_elapsed) {
  assert(peer != NULL);
  assert(time_elapsed >= 0);

  peer_tick_result_t result;
  memset(&result, 0, sizeof result);
  DA_INIT(result.packets, 0, peer->alloc);

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
    /*  Send messages to clients.
     */

    for (ptrdiff_t i = 1; i < peer->slots.size; i++) {
      peer_slot_t *const slot = peer->slots.values + i;

      switch (slot->state) {
        case PEER_SLOT_SESSION_REQUEST: {
          /*  Send the session response message.
           */

          uint8_t
              message[PEER_N_MESSAGE_DATA + 1 + PEER_ADDRESS_SIZE];
          uint8_t data[1 + PEER_ADDRESS_SIZE];
          data[0] = PEER_M_SESSION_RESPONSE;
          memcpy(data + 1, slot->local.address_data,
                 slot->local.address_size);

          ptrdiff_t const index = PEER_UNDEFINED;
          ptrdiff_t const time  = 0;

          peer_write_message(message, PEER_MESSAGE_MODE_SERVICE,
                             index, time, slot->actor,
                             slot->local.address_size + 1, data);

          peer_message_ref_t const ref = {
            .size = PEER_N_MESSAGE_DATA + 1 +
                    slot->local.address_size,
            .values = message
          };

          peer_messages_ref_t const mref = { .size   = 1,
                                             .values = &ref };

          result.status |= peer_pack(peer->slots.values[0].local.id,
                                     slot->remote.id, mref,
                                     &result.packets);

          slot->state = PEER_SLOT_READY;
        } break;

        case PEER_SLOT_READY: {
          kit_status_t const s = queue_pack(
              &peer->queue, slot->out_index, slot->local.id,
              slot->remote.id, &result.packets, peer->alloc);

          if (s == KIT_OK)
            slot->out_index = peer->queue.size;

          result.status |= s;
        } break;

        default:;
      }
    }
  }

  if (peer->mode == PEER_CLIENT && peer->slots.size > 0) {
    peer_slot_t *const slot = peer->slots.values;

    if (slot->out_index < slot->queue.size) {
      kit_status_t const s = queue_pack(
          &slot->queue, slot->out_index, slot->local.id,
          slot->remote.id, &result.packets, peer->alloc);

      if (s == KIT_OK)
        slot->out_index = slot->queue.size;

      result.status |= s;
    } else {
      peer_messages_ref_t mref = { .size = 0, .values = NULL };

      result.status |= peer_pack(slot->local.id, slot->remote.id,
                                 mref, &result.packets);
    }
  }

  return result;
}
