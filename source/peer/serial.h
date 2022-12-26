#ifndef PEER_SERIAL_H
#define PEER_SERIAL_H

#include "options.h"

#include <assert.h>
#include <stddef.h>
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

static uint64_t peer_read_message_checksum(
    uint8_t const *const message) {
  assert(message != NULL);
  return peer_read_u64(message + PEER_N_MESSAGE_CHECKSUM);
}

static uint16_t peer_read_message_size(uint8_t const *const message) {
  assert(message != NULL);
  return ((uint16_t) message[PEER_N_MESSAGE_SIZE]) |
         (((uint16_t) message[PEER_N_MESSAGE_SIZE_AND_MODE] & 0x3)
          << 8);
}

static ptrdiff_t peer_read_message_data_size(
    uint8_t const *const message) {
  ptrdiff_t full_size = (ptrdiff_t) peer_read_message_size(message);
  assert(full_size >= PEER_N_MESSAGE_DATA);
  return full_size - PEER_N_MESSAGE_DATA;
}

static uint8_t peer_read_message_mode(uint8_t const *const message) {
  assert(message != NULL);
  return message[PEER_N_MESSAGE_SIZE_AND_MODE] >> 2;
}

static ptrdiff_t peer_read_message_index(
    uint8_t const *const message) {
  assert(message != NULL);
  return (ptrdiff_t) peer_read_u64(message + PEER_N_MESSAGE_INDEX);
}

static peer_time_t peer_read_message_time(
    uint8_t const *const message) {
  assert(message != NULL);
  return (peer_time_t) peer_read_u64(message + PEER_N_MESSAGE_TIME);
}

static uint32_t peer_read_message_actor(
    uint8_t const *const message) {
  assert(message != NULL);
  return peer_read_u32(message + PEER_N_MESSAGE_ACTOR);
}

static void peer_write_message_size(uint8_t *const message,
                                    uint16_t const size) {
  assert(message != NULL);
  assert((size & 0xfc00) == 0);

  message[PEER_N_MESSAGE_SIZE] = (uint8_t) (size & 0xff);
  message[PEER_N_MESSAGE_SIZE_AND_MODE] &= 0xfc;
  message[PEER_N_MESSAGE_SIZE_AND_MODE] |= (uint8_t) ((size >> 8) &
                                                      0x3);
}

static void peer_write_message_mode(uint8_t *const message,
                                    uint8_t const  mode) {
  assert(message != NULL);
  assert((mode & 0xc0) == 0);

  message[PEER_N_MESSAGE_SIZE_AND_MODE] &= 0x3;
  message[PEER_N_MESSAGE_SIZE_AND_MODE] |= mode << 2;
}

static void peer_write_message(uint8_t *const       destination,
                               uint8_t const        mode,
                               ptrdiff_t const      index,
                               peer_time_t const    time,
                               uint32_t const       actor,
                               ptrdiff_t const      data_size,
                               uint8_t const *const data) {
  assert(destination != NULL);
  assert(data_size >= 0 && data_size <= PEER_MAX_MESSAGE_SIZE);
  assert(data_size == 0 || data != NULL);

  /*  FIXME
   *  Calculate the checksum.
   */
  uint64_t const checksum  = 0;
  uint16_t const full_size = PEER_N_MESSAGE_DATA + data_size;

  peer_write_u64(destination + PEER_N_MESSAGE_CHECKSUM, checksum);
  peer_write_message_size(destination, full_size);
  peer_write_message_mode(destination, mode);
  peer_write_u64(destination + PEER_N_MESSAGE_INDEX, index);
  peer_write_u64(destination + PEER_N_MESSAGE_TIME, time);
  peer_write_u32(destination + PEER_N_MESSAGE_ACTOR, actor);

  if (data_size > 0)
    memcpy(destination + PEER_N_MESSAGE_DATA, data, data_size);
}

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

#ifdef __cplusplus
}
#endif

#endif
