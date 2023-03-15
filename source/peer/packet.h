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
typedef KIT_DA(uint8_t) peer_chunk_t;
typedef KIT_AR(uint8_t) peer_chunk_ref_t;
typedef KIT_DA(peer_chunk_t) peer_chunks_t;
typedef KIT_AR(peer_chunk_ref_t) peer_chunks_ref_t;
typedef KIT_DA(peer_chunk_ref_t) peer_chunk_refs_t;

kit_status_t peer_pack(ptrdiff_t source_id, ptrdiff_t destination_id,
                       peer_chunks_ref_t chunks,
                       peer_packets_t   *out_packets);

kit_status_t peer_unpack(peer_packets_ref_t packets,
                         peer_chunks_t     *out_chunks);

#ifdef __cplusplus
}
#endif

#endif
