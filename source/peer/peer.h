#ifndef PEER_PEER_H
#define PEER_PEER_H

/*  TODO
 *
 *  Session version.
 */

#include "packet.h"

#include <kit/allocator.h>
#include <kit/status.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { PEER_ERROR_BAD_ALLOC = 1, PEER_ERROR_NOT_IMPLEMENTED = -1 };

typedef int64_t peer_time_t;

typedef struct {
  ptrdiff_t      size;
  uint8_t const *values;
} peer_data_ref_t;

typedef struct {
  peer_time_t time;
  ptrdiff_t   actor;
  ptrdiff_t   size;
  ptrdiff_t   offset;
} peer_data_entry_t;

typedef KIT_DA(peer_address_t) peer_addresses_t;
typedef KIT_AR(peer_address_t) peer_addresses_ref_t;

typedef KIT_DA(peer_data_entry_t) peer_queue_t;
typedef KIT_DA(uint8_t) peer_buffer_t;

typedef struct {
  peer_addresses_t sockets;
  peer_queue_t     queue;
  peer_buffer_t    buffer;
} peer_t;

kit_status_t peer_init_host(peer_t *host, kit_allocator_t alloc);
kit_status_t peer_init_client(peer_t *client, kit_allocator_t alloc);
kit_status_t peer_open(peer_t *peer, peer_addresses_ref_t sockets);
kit_status_t peer_destroy(peer_t *peer);
kit_status_t peer_queue(peer_t *peer, peer_data_ref_t data);
kit_status_t peer_connect(peer_t        *client,
                          peer_address_t server_address);
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
