#ifndef PEER_PEER_H
#define PEER_PEER_H

#include "packet.h"

#include <kit/allocator.h>
#include <kit/options.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { PEER_ERROR_NOT_IMPLEMENTED = -1 };

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

KIT_DA_TYPE(peer_queue_t, peer_data_entry_t);
KIT_DA_TYPE(peer_buffer_t, uint8_t);

typedef struct {
  kit_status_t  status;
  peer_queue_t  queue;
  peer_buffer_t buffer;
} peer_t;

kit_status_t peer_init_host(peer_t *host, peer_address_t address,
                            kit_allocator_t alloc);
kit_status_t peer_init_client(peer_t *client, peer_address_t address,
                              kit_allocator_t alloc);
kit_status_t peer_destroy(peer_t *peer);
kit_status_t peer_queue(peer_t *peer, peer_data_ref_t data);
kit_status_t peer_connect(peer_t        *client,
                          peer_address_t server_address);
kit_status_t peer_input(peer_t *peer, peer_packets_ref_t packets);
peer_packets_t peer_tick(peer_t *peer, peer_time_t time_elapsed);

#ifdef __cplusplus
}
#endif

#endif
