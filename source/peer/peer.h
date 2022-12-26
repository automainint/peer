#ifndef PEER_PEER_H
#define PEER_PEER_H

#include "packet.h"

#include <kit/allocator.h>
#include <kit/status.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  peer_time_t time;
  ptrdiff_t   actor;
  ptrdiff_t   size;
  ptrdiff_t   offset;
} peer_message_entry_t;

typedef struct {
  ptrdiff_t id;

  /*  Application-level address representation.
   */
  ptrdiff_t address_size;
  uint8_t   address_data[PEER_ADDRESS_SIZE];
} peer_endpoint_t;

typedef enum {
  PEER_SLOT_EMPTY,
  PEER_SLOT_SESSION_REQUEST,
  PEER_SLOT_READY
} peer_slot_state_t;

typedef struct {
  peer_slot_state_t state;
  peer_endpoint_t   local;
  peer_endpoint_t   remote;
} peer_slot_t;

typedef KIT_DA(peer_slot_t) peer_slots_t;
typedef KIT_AR(ptrdiff_t) peer_ids_ref_t;

typedef KIT_DA(peer_message_entry_t) peer_queue_t;
typedef KIT_DA(uint8_t) peer_buffer_t;

typedef enum { PEER_HOST, PEER_CLIENT } peer_mode_t;

typedef struct {
  peer_mode_t   mode;
  peer_slots_t  slots;  /*  All sessions. */
  peer_queue_t  queue;  /*  Shared mutual message queue. */
  peer_buffer_t buffer; /*  Buffer for messages' data. */
} peer_t;

kit_status_t peer_init(peer_t *host, peer_mode_t mode,
                       kit_allocator_t alloc);
kit_status_t peer_open(peer_t *peer, peer_ids_ref_t ids);
kit_status_t peer_destroy(peer_t *peer);
kit_status_t peer_queue(peer_t *peer, peer_message_ref_t message);
kit_status_t peer_connect(peer_t *client, ptrdiff_t server_id);
kit_status_t peer_input(peer_t *peer, peer_packets_ref_t packets);

typedef struct {
  kit_status_t   status;
  peer_packets_t packets;
} peer_tick_result_t;

peer_tick_result_t peer_tick(peer_t *peer, peer_time_t time_elapsed);

#ifdef __cplusplus
}
#endif

#endif
