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

  /*  We don't need strong unpredictable random numbers.
   */
  uint64_t const seed = 12345;

  mt64_init(&peer->mt64, seed);
  mt64_rotate(&peer->mt64);

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
    return PEER_ERROR_INVALID_ID;

  ptrdiff_t const n = peer->slots.size;

  DA_RESIZE(peer->slots, n + ids.size);
  assert(peer->slots.size == n + ids.size);

  if (peer->slots.size != n + ids.size) {
    DA_RESIZE(peer->slots, n);
    return PEER_ERROR_BAD_ALLOC;
  }

  memset(peer->slots.values + n, 0,
         ids.size * sizeof *peer->slots.values);

  for (ptrdiff_t i = 0; i < ids.size; i++) {
    peer_slot_t *slot = peer->slots.values + (n + i);

    slot->local.id             = ids.values[i];
    slot->local.is_id_resolved = 1;
    slot->remote.id            = PEER_UNDEFINED;
    slot->actor = peer->mode == PEER_HOST ? i : PEER_UNDEFINED;

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
      slot->remote.id             = server_id;
      slot->remote.is_id_resolved = 1;
      slot->remote.address_size   = 0;

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

    /*  Skip if packet not intended for this peer.
     */
    for (ptrdiff_t j = 0; j < peer->slots.size; j++)
      if (peer->slots.values[j].local.id == packet->destination_id) {
        slot_found = 1;
        break;
      }
    if (!slot_found)
      continue;

    slot_found = 0;

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

          if (message_mode != PEER_MESSAGE_MODE_SERVICE) {
            /*  Add message to the slot queue.
             */

            peer_chunk_ref_t const data = {
              .size   = data_size,
              .values = chunk->values + PEER_N_MESSAGE_DATA
            };

            status |= queue_insert(&slot->queue, index, time, actor,
                                   data, peer->alloc);
          }
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
                peer->actor = actor;

                /*  We need new id for new remote port.
                 */
                slot->remote.is_id_resolved = 0;
                slot->remote.address_size   = data_size - 1;
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

          if (message_mode != PEER_MESSAGE_MODE_SERVICE) {
            /*  Add message to the mutual queue.
             */

            peer_chunk_ref_t const data = {
              .size   = data_size,
              .values = chunk->values + PEER_N_MESSAGE_DATA
            };

            status |= queue_insert(&peer->queue, index, time, actor,
                                   data, peer->alloc);
          }

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
        peer->slots.values[0].remote.id != PEER_UNDEFINED) {
      continue;
    }

    /*  Assign a slot for the client.
     */

    /*  FIXME
     *  Check if the client sent a session request message.
     */

    for (ptrdiff_t j = 1; j < peer->slots.size; j++) {
      peer_slot_t *const slot = peer->slots.values + j;

      if (slot->remote.id == PEER_UNDEFINED &&
          slot->local.address_size > 0) {
        slot->state                 = PEER_SLOT_SESSION_REQUEST;
        slot->remote.id             = packet->source_id;
        slot->remote.is_id_resolved = 1;
        slot->actor                 = j;

        slot_found = 1;
        break;
      }
    }

    if (!slot_found)
      status |= PEER_ERROR_NO_FREE_SLOTS;
  }

  return status;
}

static kit_status_t chunks_append_trail(
    mt64_state_t *const rng, peer_queue_t const *const q,
    ptrdiff_t const index, peer_chunks_t *const out_chunks) {
  assert(rng != NULL);
  assert(q != NULL);
  assert(out_chunks != NULL);
  assert(index >= 0 && index <= q->size);

  kit_allocator_t const alloc = out_chunks->alloc;

  ptrdiff_t trail_size = PEER_TRAIL_SERIAL_SIZE;

  if (trail_size > index ||
      index - trail_size < PEER_TRAIL_SCATTER_DISTANCE)
    trail_size = index;

  if (trail_size > 0) {
    ptrdiff_t const offset = out_chunks->size;
    DA_RESIZE(*out_chunks, offset + trail_size);
    assert(out_chunks->size == offset + trail_size);
    if (out_chunks->size != offset + trail_size)
      return PEER_ERROR_BAD_ALLOC;

    ptrdiff_t const trail_begin = index - trail_size;

    for (ptrdiff_t i = 0; i < trail_size; i++) {
      peer_message_t const *const message = q->values +
                                            (trail_begin + i);
      peer_chunk_t *const chunk = out_chunks->values + (offset + i);
      ptrdiff_t const     chunk_size = PEER_N_MESSAGE_DATA +
                                   message->data.size;

      DA_INIT(*chunk, chunk_size, alloc);

      if (chunk->size != chunk_size) {
        for (ptrdiff_t j = 0; j <= i; j++)
          DA_DESTROY(out_chunks->values[offset + j]);
        DA_RESIZE(*out_chunks, offset);
        return PEER_ERROR_BAD_ALLOC;
      }

      peer_write_message(chunk->values, PEER_MESSAGE_MODE_APPLICATION,
                         trail_begin + i, message->time,
                         message->actor, message->data.size,
                         message->data.values);
    }
  }

  ptrdiff_t scatter_trail_distance = PEER_TRAIL_SCATTER_DISTANCE;
  ptrdiff_t scatter_trail_size     = PEER_TRAIL_SCATTER_SIZE;

  if (scatter_trail_distance > index - trail_size)
    scatter_trail_distance = index - trail_size;
  if (scatter_trail_size > scatter_trail_distance)
    scatter_trail_size = scatter_trail_distance;

  if (scatter_trail_size > 0) {
    ptrdiff_t const offset = out_chunks->size;
    DA_RESIZE(*out_chunks, offset + scatter_trail_size);
    assert(out_chunks->size == offset + scatter_trail_size);
    if (out_chunks->size != offset + scatter_trail_size)
      return PEER_ERROR_BAD_ALLOC;

    ptrdiff_t const trail_begin = index - scatter_trail_distance;

    for (ptrdiff_t i = 0; i < scatter_trail_size; i++) {
      ptrdiff_t const message_index =
          trail_begin +
          (ptrdiff_t) (mt64_generate(rng) %
                       (uint64_t) scatter_trail_distance);

      peer_message_t const *const message = q->values + message_index;
      peer_chunk_t *const chunk = out_chunks->values + (offset + i);
      ptrdiff_t const     chunk_size = PEER_N_MESSAGE_DATA +
                                   message->data.size;

      DA_INIT(*chunk, chunk_size, alloc);

      if (chunk->size != chunk_size) {
        for (ptrdiff_t j = 0; j <= i; j++)
          DA_DESTROY(out_chunks->values[offset + j]);
        DA_RESIZE(*out_chunks, offset);
        return PEER_ERROR_BAD_ALLOC;
      }

      peer_write_message(chunk->values, PEER_MESSAGE_MODE_APPLICATION,
                         message_index, message->time, message->actor,
                         message->data.size, message->data.values);
    }
  }

  return KIT_OK;
}

typedef struct {
  kit_status_t      status;
  peer_chunk_refs_t refs;
  peer_chunks_ref_t ref;
} chunks_wrap_t;

static chunks_wrap_t chunks_wrap(peer_chunks_t const *const chunks,
                                 kit_allocator_t const      alloc) {
  assert(chunks != NULL);

  chunks_wrap_t result;
  memset(&result, 0, sizeof result);

  ptrdiff_t const size = chunks->size;

  DA_INIT(result.refs, size, alloc);
  assert(result.refs.size == size);
  if (result.refs.size != size) {
    result.status = PEER_ERROR_BAD_ALLOC;
    return result;
  }

  for (ptrdiff_t i = 0; i < size; i++) {
    result.refs.values[i].size   = chunks->values[i].size;
    result.refs.values[i].values = chunks->values[i].values;
  }

  result.ref.size   = result.refs.size;
  result.ref.values = result.refs.values;
  return result;
}

static kit_status_t queue_pack(mt64_state_t *const       rng,
                               peer_queue_t const *const q,
                               ptrdiff_t const           index,
                               ptrdiff_t                 source_id,
                               ptrdiff_t             destination_id,
                               peer_packets_t *const out_packets,
                               kit_allocator_t       alloc) {
  ptrdiff_t const size = q->size - index;

  assert(rng != NULL);
  assert(q != NULL);
  assert(size >= 0);

  if (size < 0)
    return PEER_ERROR_INVALID_OUT_INDEX;
  if (size == 0)
    return KIT_OK;

  peer_chunks_t chunks;
  DA_INIT(chunks, size, alloc);
  assert(chunks.size == size);
  if (chunks.size != size)
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

  /*  Append trail messages.
   */

  kit_status_t result = chunks_append_trail(rng, q, index, &chunks);

  /*  Pack messages into packets.
   */

  chunks_wrap_t const wrap = chunks_wrap(&chunks, alloc);

  if (wrap.status != KIT_OK) {
    result |= wrap.status;
    return result;
  }

  result |= peer_pack(source_id, destination_id, wrap.ref,
                      out_packets);

  for (ptrdiff_t i = 0; i < chunks.size; i++)
    DA_DESTROY(chunks.values[i]);

  DA_DESTROY(chunks);
  DA_DESTROY(wrap.refs);

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
        case PEER_SLOT_EMPTY: break;

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
                &peer->mt64, &peer->queue, slot->out_index,
                slot->local.id, slot->remote.id, &result.packets,
                peer->alloc);

            if (s == KIT_OK)
              slot->out_index = peer->queue.size;

            result.status |= s;

            slot->clock_heartbeat = PEER_TIMEOUT_HEARTBEAT;

          } else if (slot->clock_heartbeat <= 0) {
            /*  No new messages. Send heartbeat message.
             */

            peer_chunks_t chunks;
            DA_INIT(chunks, 1, peer->alloc);
            assert(chunks.size == 1);
            if (chunks.size != 1) {
              result.status |= PEER_ERROR_BAD_ALLOC;
              break;
            }

            DA_INIT(chunks.values[0], PEER_N_MESSAGE_DATA + 1,
                    peer->alloc);
            assert(chunks.values[0].size == PEER_N_MESSAGE_DATA + 1);

            if (chunks.values[0].size != PEER_N_MESSAGE_DATA + 1) {
              DA_DESTROY(chunks.values[0]);
              DA_RESIZE(chunks, 0);
              result.status |= PEER_ERROR_BAD_ALLOC;
            } else {
              ptrdiff_t const index        = PEER_UNDEFINED;
              uint8_t const   id_heartbeat = PEER_M_HEARTBEAT;

              peer_write_message(chunks.values[0].values,
                                 PEER_MESSAGE_MODE_SERVICE, index,
                                 peer->time, peer->actor,
                                 sizeof id_heartbeat, &id_heartbeat);
            }

            result.status |= chunks_append_trail(
                &peer->mt64, &peer->queue, slot->out_index, &chunks);

            chunks_wrap_t const wrap = chunks_wrap(&chunks,
                                                   peer->alloc);

            result.status |= wrap.status;

            result.status |= peer_pack(slot->local.id,
                                       slot->remote.id, wrap.ref,
                                       &result.packets);

            slot->clock_heartbeat = PEER_TIMEOUT_HEARTBEAT;

            for (ptrdiff_t i = 0; i < chunks.size; i++)
              DA_DESTROY(chunks.values[i]);

            DA_DESTROY(chunks);
            DA_DESTROY(wrap.refs);
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
          &peer->mt64, &slot->queue, slot->out_index, slot->local.id,
          slot->remote.id, &result.packets, peer->alloc);

      if (s == KIT_OK)
        slot->out_index = slot->queue.size;

      result.status |= s;
    } else if (slot->clock_heartbeat <= 0) {
      /*  No new messages. Send heartbeat message.
       */

      peer_chunks_t chunks;
      DA_INIT(chunks, 1, peer->alloc);
      assert(chunks.size == 1);

      if (chunks.size != 1) {
        result.status |= PEER_ERROR_BAD_ALLOC;
        return result;
      }

      DA_INIT(chunks.values[0], PEER_N_MESSAGE_DATA + 1, peer->alloc);
      assert(chunks.values[0].size == PEER_N_MESSAGE_DATA + 1);

      if (chunks.values[0].size != PEER_N_MESSAGE_DATA + 1) {
        DA_DESTROY(chunks.values[0]);
        DA_RESIZE(chunks, 0);
        result.status |= PEER_ERROR_BAD_ALLOC;
      } else {
        ptrdiff_t const   index        = PEER_UNDEFINED;
        peer_time_t const time         = 0;
        uint8_t const     id_heartbeat = PEER_M_HEARTBEAT;

        peer_write_message(
            chunks.values[0].values, PEER_MESSAGE_MODE_SERVICE, index,
            time, peer->actor, sizeof id_heartbeat, &id_heartbeat);
      }

      result.status |= chunks_append_trail(&peer->mt64, &peer->queue,
                                           slot->out_index, &chunks);

      chunks_wrap_t const wrap = chunks_wrap(&chunks, peer->alloc);

      result.status |= wrap.status;

      result.status |= peer_pack(slot->local.id, slot->remote.id,
                                 wrap.ref, &result.packets);

      slot->clock_heartbeat = PEER_TIMEOUT_HEARTBEAT;

      for (ptrdiff_t i = 0; i < chunks.size; i++)
        DA_DESTROY(chunks.values[i]);

      DA_DESTROY(chunks);
      DA_DESTROY(wrap.refs);
    }
  }

  return result;
}
