#ifndef PEER_OPTIONS_H
#define PEER_OPTIONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

enum {
  /*  Internal constants.
   */

  PEER_VERSION     = 1,
  PEER_DEVELOPMENT = 1,

  PEER_ADDRESS_SIZE =
      20, /* Address size should be big enough to contain IPv4 and
             IPv6 addresses and ports (6 bytes or 18 bytes). */

  PEER_UNDEFINED = -1, /* Undefined value for id, index and agent. */

  /*  Protocol settings.
   */

  PEER_PACKET_SIZE =
      400, /* On average, peer sends
              PEER_PACKET_SIZE / 10 = 40 kB per second. */

  PEER_MT64_KEY_SIZE = 128, /* Key size for mt64 stream cipher. */

  PEER_HEARTBEAT_TIMEOUT =
      10, /* Peer will send a heartbeat notification if no messages
             was sent in 10 ms. */

  PEER_PING_TIMEOUT =
      200, /* Peer will send a ping request every 200 ms. */

  PEER_CONNECTION_TIMEOUT =
      2000, /* Peer will change the connection status to lost after 2
               seconds of silence. */

  /*  Packet mode values.
   */

  PEER_PACKET_MODE_PLAIN = 0, /* Packet is not encrypted. */
  PEER_PACKET_MODE_MT64  = 1, /* Packet is encrypted with mt64
                                 cipher. */

  /*  Message mode values.
   */

  PEER_MESSAGE_MODE_SERVICE     = 0, /* Service-level message. */
  PEER_MESSAGE_MODE_APPLICATION = 1, /* Application-level message. */

  /*  Service-level message ids.
   */

  PEER_M_HEARTBEAT        = 1,
  PEER_M_PING             = 2,
  PEER_M_PONG             = 3,
  PEER_M_SESSION_REQUEST  = 4,
  PEER_M_SESSION_RESPONSE = 5,

  /*  Data offsets.
   */

  PEER_N_PACKET_SESSION  = 0,  /* 4 bytes */
  PEER_N_PACKET_INDEX    = 4,  /* 8 bytes */
  PEER_N_PACKET_MODE     = 12, /* 1 byte */
  PEER_N_PACKET_SIZE     = 13, /* 2 bytes */
  PEER_N_PACKET_MESSAGES = 15,

  PEER_N_MESSAGE_CHECKSUM      = 0,  /* 8 bytes */
  PEER_N_MESSAGE_SIZE          = 8,  /* 1 byte */
  PEER_N_MESSAGE_SIZE_AND_MODE = 9,  /* 1 byte */
  PEER_N_MESSAGE_INDEX         = 10, /* 8 bytes */
  PEER_N_MESSAGE_TIME          = 18, /* 8 bytes */
  PEER_N_MESSAGE_ACTOR         = 26, /* 4 bytes */
  PEER_N_MESSAGE_DATA          = 30,

  PEER_MAX_MESSAGE_SIZE =
      PEER_PACKET_SIZE - PEER_N_PACKET_MESSAGES -
      PEER_N_MESSAGE_DATA, /* Message size acquires 10 bits, so max
                              possible value for it is 1023. */

  /*  Error codes.
   */

  PEER_ERROR_BAD_ALLOC            = 1,
  PEER_ERROR_INVALID_CIPHER       = 2,
  PEER_ERROR_INVALID_KEY          = 4,
  PEER_ERROR_INVALID_PEER         = 8,
  PEER_ERROR_INVALID_MESSAGE      = 16,
  PEER_ERROR_INVALID_IDS          = 32,
  PEER_ERROR_INVALID_TIME_ELAPSED = 64,
  PEER_ERROR_NO_FREE_SLOTS        = 128,
  PEER_ERROR_INVALID_MESSAGE_SIZE = 256,
  PEER_ERROR_INVALID_PACKET_SIZE  = 512,
  PEER_ERROR_NOT_IMPLEMENTED      = -1
};

typedef int64_t peer_time_t;

#ifdef __cplusplus
}
#endif

#endif
