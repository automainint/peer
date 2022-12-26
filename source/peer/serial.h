#ifndef PEER_SERIAL_H
#define PEER_SERIAL_H

#include <assert.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-function"
#  pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif

/*  Endianness-independent data serialization and deserialization
 *  utilities.
 */

static uint8_t peer_read_u8(uint8_t const *const source) {
  assert(source != NULL);
  return source[0];
}

static uint16_t peer_read_u16(uint8_t const *const source) {
  assert(source != NULL);
  return ((uint16_t) source[0]) | (((uint16_t) source[1]) << 8);
}

static uint32_t peer_read_u32(uint8_t const *const source) {
  assert(source != NULL);
  return ((uint32_t) source[0]) | (((uint32_t) source[1]) << 8) |
         (((uint32_t) source[2]) << 16) |
         (((uint32_t) source[3]) << 24);
}

static uint64_t peer_read_u64(uint8_t const *const source) {
  assert(source != NULL);
  return ((uint64_t) source[0]) | (((uint64_t) source[1]) << 8) |
         (((uint64_t) source[2]) << 16) |
         (((uint64_t) source[3]) << 24) |
         (((uint64_t) source[4]) << 32) |
         (((uint64_t) source[5]) << 40) |
         (((uint64_t) source[6]) << 48) |
         (((uint64_t) source[7]) << 56);
}

static void peer_write_u8(uint8_t *const destination,
                          uint8_t const  x) {
  assert(destination != NULL);
  destination[0] = x;
}

static void peer_write_u16(uint8_t *const destination,
                           uint16_t const x) {
  assert(destination != NULL);
  destination[0] = (uint8_t) (x & 0xff);
  destination[1] = (uint8_t) (x >> 8);
}

static void peer_write_u32(uint8_t *const destination,
                           uint32_t const x) {
  assert(destination != NULL);
  destination[0] = (uint8_t) (x & 0xff);
  destination[1] = (uint8_t) (x >> 8);
  destination[2] = (uint8_t) (x >> 16);
  destination[3] = (uint8_t) (x >> 24);
}

static void peer_write_u64(uint8_t *const destination,
                           uint64_t const x) {
  assert(destination != NULL);
  destination[0] = (uint8_t) (x & 0xff);
  destination[1] = (uint8_t) (x >> 8);
  destination[2] = (uint8_t) (x >> 16);
  destination[3] = (uint8_t) (x >> 24);
  destination[4] = (uint8_t) (x >> 32);
  destination[5] = (uint8_t) (x >> 40);
  destination[6] = (uint8_t) (x >> 48);
  destination[7] = (uint8_t) (x >> 56);
}

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

#ifdef __cplusplus
}
#endif

#endif
