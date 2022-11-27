#ifndef PEER_PACKET_H
#define PEER_PACKET_H

#include <kit/array_ref.h>
#include <kit/dynamic_array.h>

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { PEER_PACKET_SIZE = 0x1000 };

typedef ptrdiff_t peer_address_t;

typedef struct {
  peer_address_t source;
  peer_address_t destination;
  uint8_t        data[PEER_PACKET_SIZE];
} peer_packet_t;

KIT_DA_TYPE(peer_packets_t, peer_packet_t);
KIT_AR_TYPE(peer_packets_ref_t, peer_packet_t);

#ifdef __cplusplus
}
#endif

#endif
