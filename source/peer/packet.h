#ifndef PEER_PACKET_H
#define PEER_PACKET_H

#include "options.h"
#include <kit/array_ref.h>
#include <kit/dynamic_array.h>
#include <kit/status.h>

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  ptrdiff_t source_id;
  ptrdiff_t destination_id;
  ptrdiff_t size;
  uint8_t   data[PEER_PACKET_SIZE];
} peer_packet_t;

typedef KIT_DA(peer_packet_t) peer_packets_t;
typedef KIT_AR(peer_packet_t) peer_packets_ref_t;
typedef KIT_DA(uint8_t) peer_message_t;
typedef KIT_AR(uint8_t) peer_message_ref_t;
typedef KIT_DA(peer_message_t) peer_messages_t;
typedef KIT_AR(peer_message_ref_t) peer_messages_ref_t;

typedef struct {
  kit_status_t   status;
  peer_packets_t packets;
} peer_pack_result_t;

peer_pack_result_t peer_pack(peer_messages_ref_t messages,
                             kit_allocator_t     alloc);

typedef struct {
  kit_status_t    status;
  peer_messages_t messages;
} peer_unpack_result_t;

peer_unpack_result_t peer_unpack(peer_packets_ref_t packets,
                                 kit_allocator_t    alloc);

#ifdef __cplusplus
}
#endif

#endif
