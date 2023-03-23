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
    slot->actor     = peer->mode == PEER_HOST ? i : PEER_UNDEFINED;

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
                                 peer_chunk_ref_t const data,
                                 kit_allocator_t        alloc) {
  ptrdiff_t index = q->size;
  DA_RESIZE(*q, index + 1);
  if (q->size != index + 1)
    return PEER_ERROR_BAD_ALLOC;

  memset(q->values + index, 0, sizeof *q->values);

  q->values[index].is_ready = 1;
  q->values[index].time     = time;
  q->values[index].actor    = actor;

  DA_INIT(q->values[index].data, data.size, alloc);
  if (q->values[index].data.size != data.size) {
    DA_RESIZE(*q, index);
    return PEER_ERROR_BAD_ALLOC;
  }

  if (data.size > 0)
    memcpy(q->values[index].data.values, data.values, data.size);

  return KIT_OK;
}

static kit_status_t queue_insert(peer_queue_t *const q,
                                 ptrdiff_t const     index,
                                 peer_time_t time, ptrdiff_t actor,
                                 peer_chunk_ref_t const data,
                                 kit_allocator_t        alloc) {
  if (index == PEER_UNDEFINED)
    /*  Don't save unindexed messages.
     */
    return KIT_OK;

  assert(index >= 0);

  if (index < 0)
    return PEER_ERROR_INVALID_MESSAGE_INDEX;

  if (index < q->size && q->values[index].is_ready) {
    /*  FIXME
     *  Check if message is the same.
     */

  } else {
    /*  Add message to the mutual queue.
     */

    ptrdiff_t const n = q->size;

    if (index >= n) {
      DA_RESIZE(*q, index + 1);
      if (q->size != index + 1)
        return PEER_ERROR_BAD_ALLOC;

      memset(q->values + n, 0, (index + 1 - n) * sizeof *q->values);
    }

    DA_INIT(q->values[index].data, data.size, alloc);
    if (q->values[index].data.size != data.size)
      return PEER_ERROR_BAD_ALLOC;

    q->values[index].is_ready = 1;
    q->values[index].time     = time;
    q->values[index].actor    = actor;

    if (data.size > 0)
      memcpy(q->values[index].data.values, data.values, data.size);
  }

  return KIT_OK;
}

kit_status_t peer_queue(peer_t *const          peer,
                        peer_chunk_ref_t const message_data) {
  assert(peer != NULL);
  assert(message_data.size == 0 || message_data.values != NULL);

  if (peer == NULL)
    return PEER_ERROR_INVALID_PEER;
  if (message_data.size == 0 && message_data.values == NULL)
    return PEER_ERROR_INVALID_MESSAGE;

  peer_time_t const time  = 0;
  ptrdiff_t const   actor = peer->actor;

  switch (peer->mode) {
    case PEER_HOST:
      return queue_append(&peer->queue, time, actor, message_data,
                          peer->alloc);

    case PEER_CLIENT:
      if (peer->slots.size > 0)
        return queue_append(&peer->slots.values[0].queue, time, actor,
                            message_data, peer->alloc);

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

      peer_chunks_t chunks;
      DA_INIT(chunks, 0, peer->alloc);

      peer_packets_ref_t const ref = { .size = 1, .values = packet };

      status |= peer_unpack(ref, &chunks);

      for (ptrdiff_t k = 0; k < chunks.size; k++) {
        peer_chunk_t *const chunk = chunks.values + k;

        /*  FIXME
         *  Check the checksum.
         */

        ptrdiff_t const data_size = peer_read_message_data_size(
            chunk->values);
        uint8_t const message_mode = peer_read_message_mode(
            chunk->values);
        ptrdiff_t const index = peer_read_message_index(
            chunk->values);
        peer_time_t const time = peer_read_message_time(
            chunk->values);
        uint32_t const actor = peer_read_message_actor(chunk->values);

        if (peer->mode == PEER_HOST) {
          /*  Check the message time.
           *  Client's messages should not have time set.
           */

          assert(time == 0);

          if (time != 0) {
            status |= PEER_ERROR_INVALID_MESSAGE_TIME;
            continue;
          }

          /*  Check the actor id value.
           */

          assert(actor == slot->actor);

          if (actor != slot->actor) {
            status |= PEER_ERROR_INVALID_MESSAGE_ACTOR;
            continue;
          }

          /*  Process service-level messages.
           */

          int processed = 0;

          if (message_mode == PEER_MESSAGE_MODE_SERVICE &&
              data_size >= 1) {
            uint8_t const service_id = peer_read_u8(
                chunk->values + PEER_N_MESSAGE_DATA);

            switch (service_id) {
              case PEER_M_HEARTBEAT:
                /*  Heartbeat.
                 */
                processed = 1;
                break;

              default:;
            }
          }

          assert(message_mode != PEER_MESSAGE_MODE_SERVICE ||
                 processed == 1);

          if (message_mode == PEER_MESSAGE_MODE_SERVICE && !processed)
            status |= PEER_ERROR_UNKNOWN_SERVICE_ID;

          /*  Add message to the slot queue.
           */

          peer_chunk_ref_t const data = {
            .size   = data_size,
            .values = chunk->values + PEER_N_MESSAGE_DATA
          };

          status |= queue_insert(&slot->queue, index, time, actor,
                                 data, peer->alloc);
        }

        if (peer->mode == PEER_CLIENT) {
          /*  Process service-level messages.
           */

          int processed = 0;

          if (message_mode == PEER_MESSAGE_MODE_SERVICE &&
              data_size >= 1) {
            uint8_t const service_id = peer_read_u8(
                chunk->values + PEER_N_MESSAGE_DATA);

            switch (service_id) {
              case PEER_M_HEARTBEAT:
                /*  Heartbeat.
                 */
                processed = 1;
                break;

              case PEER_M_SESSION_RESPONSE: {
                /*  Update client's actor id and host remote address.
                 */
                assert(data_size - 1 <= PEER_ADDRESS_SIZE);
                peer->actor               = actor;
                slot->remote.address_size = data_size - 1;
                memcpy(slot->remote.address_data,
                       chunk->values + PEER_N_MESSAGE_DATA + 1,
                       data_size - 1);

                /*  Update actor id for old messages.
                 */
                for (ptrdiff_t k = 0; k < slot->queue.size; k++)
                  slot->queue.values[k].actor = actor;

                processed = 1;
              } break;

              default:;
            }
          }

          assert(message_mode != PEER_MESSAGE_MODE_SERVICE ||
                 processed == 1);

          if (message_mode == PEER_MESSAGE_MODE_SERVICE && !processed)
            status |= PEER_ERROR_UNKNOWN_SERVICE_ID;

          /*  Add message to the mutual queue.
           */

          peer_chunk_ref_t const data = {
            .size   = data_size,
            .values = chunk->values + PEER_N_MESSAGE_DATA
          };

          status |= queue_insert(&peer->queue, index, time, actor,
                                 data, peer->alloc);

          /*  Synchronize mutual time.
           */

          if (peer->time < time)
            peer->time = time;
        }
      }

      for (ptrdiff_t k = 0; k < chunks.size; k++)
        DA_DESTROY(chunks.values[k]);
      DA_DESTROY(chunks);

      slot_found = 1;
      break;
    }

    if (slot_found || peer->mode != PEER_HOST ||
        peer->slots.size == 0 ||
        peer->slots.values[0].remote.id != PEER_UNDEFINED)
      continue;

    /*  Assign a slot for the client.
     */

    /*  FIXME
     *  Check if the client sent a session request or a session resume
     *  message.
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

  peer_chunks_t     chunks;
  peer_chunk_refs_t refs;

  DA_INIT(chunks, size, alloc);
  DA_INIT(refs, size, alloc);

  assert(chunks.size == size);
  assert(refs.size == size);

  if (chunks.size != size || refs.size != size)
    return PEER_ERROR_BAD_ALLOC;

  if (chunks.size != 0)
    memset(chunks.values, 0, size * sizeof *chunks.values);

  /*  Prepare messages' data.
   */

  for (ptrdiff_t i = 0; i < size; i++) {
    assert(index + i >= 0 && index + i < q->size);

    peer_message_t const *const message = q->values + (index + i);

    ptrdiff_t const full_size = PEER_N_MESSAGE_DATA +
                                message->data.size;

    DA_INIT(chunks.values[i], full_size, alloc);
    if (chunks.values[i].size != full_size)
      return PEER_ERROR_BAD_ALLOC;

    peer_write_message(chunks.values[i].values,
                       PEER_MESSAGE_MODE_APPLICATION, index + i,
                       message->time, message->actor,
                       message->data.size, message->data.values);
  }

  /*  Pack messages into packets.
   */

  for (ptrdiff_t i = 0; i < size; i++) {
    refs.values[i].size   = chunks.values[i].size;
    refs.values[i].values = chunks.values[i].values;
  }

  peer_chunks_ref_t chref = { .size   = refs.size,
                              .values = refs.values };

  kit_status_t const result = peer_pack(source_id, destination_id,
                                        chref, out_packets);

  for (ptrdiff_t i = 0; i < chunks.size; i++)
    DA_DESTROY(chunks.values[i]);

  DA_DESTROY(chunks);
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

  if (peer->time_local > INT64_MAX - time_elapsed) {
    /*  FIXME
     *  Handle time overflow.
     */

    result.status = PEER_ERROR_TIME_OVERFLOW;
    return result;
  }

  result.status = KIT_OK;

  /*  Update clock.
   */

  peer->time_local += time_elapsed;

  for (ptrdiff_t i = 0; i < peer->slots.size; i++) {
    peer_slot_t *const slot = peer->slots.values + i;

    if (slot->clock_heartbeat > 0)
      slot->clock_heartbeat -= time_elapsed;
  }

  if (peer->mode == PEER_HOST) {
    /*  Synchronize time.
     */
    peer->time = peer->time_local;

    /*  Synchronize mutual message queue.
     */

    for (ptrdiff_t i = peer->queue_index; i < peer->queue.size; i++)
      peer->queue.values[i].time = peer->time;

    for (ptrdiff_t i = 1; i < peer->slots.size; i++) {
      peer_slot_t *const slot = peer->slots.values + i;

      for (; slot->in_index < slot->queue.size; slot->in_index++) {
        peer_message_t const *const message = slot->queue.values +
                                              slot->in_index;

        if (message->is_ready == 0)
          break;

        assert(slot->actor == message->actor);

        peer_chunk_ref_t const data = {
          .size = message->data.size, .values = message->data.values
        };

        kit_status_t const s = queue_append(
            &peer->queue, peer->time, slot->actor, data, peer->alloc);

        assert(s == KIT_OK);
        result.status |= s;
      }
    }

    peer->queue_index = peer->queue.size;

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

          peer_write_message(message, PEER_MESSAGE_MODE_SERVICE,
                             index, peer->time, slot->actor,
                             slot->local.address_size + 1, data);

          peer_chunk_ref_t const ref = {
            .size = PEER_N_MESSAGE_DATA + 1 +
                    slot->local.address_size,
            .values = message
          };

          peer_chunks_ref_t const chref = { .size   = 1,
                                            .values = &ref };

          result.status |= peer_pack(peer->slots.values[0].local.id,
                                     slot->remote.id, chref,
                                     &result.packets);

          slot->clock_heartbeat = PEER_TIMEOUT_HEARTBEAT;
          slot->state           = PEER_SLOT_READY;
        } break;

        case PEER_SLOT_READY: {
          if (slot->out_index < peer->queue.size) {
            /*  Send new messages.
             */

            kit_status_t const s = queue_pack(
                &peer->queue, slot->out_index, slot->local.id,
                slot->remote.id, &result.packets, peer->alloc);

            if (s == KIT_OK)
              slot->out_index = peer->queue.size;

            result.status |= s;

            slot->clock_heartbeat = PEER_TIMEOUT_HEARTBEAT;

          } else if (slot->clock_heartbeat <= 0) {
            /*  No new messages. Send heartbeat message.
             */

            uint8_t message[PEER_N_MESSAGE_DATA + 1];

            ptrdiff_t const index        = PEER_UNDEFINED;
            uint8_t const   id_heartbeat = PEER_M_HEARTBEAT;

            peer_write_message(message, PEER_MESSAGE_MODE_SERVICE,
                               index, peer->time, peer->actor,
                               sizeof id_heartbeat, &id_heartbeat);

            peer_chunk_ref_t const ref = { .size   = sizeof message,
                                           .values = message };

            peer_chunks_ref_t const refs = { .size   = 1,
                                             .values = &ref };

            result.status |= peer_pack(slot->local.id,
                                       slot->remote.id, refs,
                                       &result.packets);

            slot->clock_heartbeat = PEER_TIMEOUT_HEARTBEAT;
          }
        } break;

        default:
          assert(0);
          result.status |= PEER_ERROR_INVALID_SLOT_STATE;
      }
    }
  }

  if (peer->mode == PEER_CLIENT && peer->slots.size > 0) {
    peer_slot_t *const slot = peer->slots.values;

    if (slot->out_index < slot->queue.size) {
      /*  Send new messages.
       */

      kit_status_t const s = queue_pack(
          &slot->queue, slot->out_index, slot->local.id,
          slot->remote.id, &result.packets, peer->alloc);

      if (s == KIT_OK)
        slot->out_index = slot->queue.size;

      result.status |= s;
    } else if (slot->clock_heartbeat <= 0) {
      /*  No new messages. Send heartbeat message.
       */

      uint8_t message[PEER_N_MESSAGE_DATA + 1];

      ptrdiff_t const index        = PEER_UNDEFINED;
      uint8_t const   id_heartbeat = PEER_M_HEARTBEAT;

      peer_write_message(message, PEER_MESSAGE_MODE_SERVICE, index,
                         peer->time_local, peer->actor,
                         sizeof id_heartbeat, &id_heartbeat);

      peer_chunk_ref_t const ref = { .size   = sizeof message,
                                     .values = message };

      peer_chunks_ref_t const refs = { .size = 1, .values = &ref };

      result.status |= peer_pack(slot->local.id, slot->remote.id,
                                 refs, &result.packets);

      slot->clock_heartbeat = PEER_TIMEOUT_HEARTBEAT;
    }
  }

  return result;
}
