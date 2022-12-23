#ifndef PEER_CIPHER_H
#define PEER_CIPHER_H

#include "packet.h"
#include <kit/status.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint8_t   key[PEER_MT64_KEY_SIZE];
  ptrdiff_t index;
} peer_cipher_t;

kit_status_t peer_cipher_init(peer_cipher_t *cipher,
                              uint8_t const *key);

typedef struct {
  kit_status_t   status;
  peer_packets_t packets;
} peer_cipher_result_t;

peer_cipher_result_t peer_encrypt(peer_cipher_t     *cipher,
                                  peer_packets_ref_t packets,
                                  kit_allocator_t    alloc);

peer_cipher_result_t peer_decrypt(peer_cipher_t     *cipher,
                                  peer_packets_ref_t packets,
                                  kit_allocator_t    alloc);

#ifdef __cplusplus
}
#endif

#endif
