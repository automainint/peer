#ifndef PEER_OPTIONS_H
#define PEER_OPTIONS_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
  PEER_VERSION = -65535, /* Development version. */

  PEER_ID_UNDEFINED = -1,
  PEER_ACTOR_SELF   = -1,

  PEER_MT64_KEY_SIZE = 128, /* Key size for mt64 stream cipher. */

  PEER_ADDRESS_SIZE =
      20, /* Address size should be big enough to contain IPv4 and
             IPv6 addresses and ports (6 bytes or 18 bytes). */

  PEER_PACKET_SIZE =
      400, /* On average, peer sends
              PEER_PACKET_SIZE / 10 = 40 kB per second. */

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
  PEER_N_PACKET_MESSAGES = 13,

  PEER_N_MESSAGE_CHECKSUM      = 0,  /* 8 bytes */
  PEER_N_MESSAGE_INDEX         = 8,  /* 8 bytes */
  PEER_N_MESSAGE_TIME          = 16, /* 8 bytes */
  PEER_N_MESSAGE_ACTOR         = 24, /* 4 bytes */
  PEER_N_MESSAGE_MODE_AND_SIZE = 28, /* 1 byte */
  PEER_N_MESSAGE_SIZE          = 29, /* 1 byte */
  PEER_N_MESSAGE_DATA          = 30,

  PEER_MAX_MESSAGE_SIZE =
      PEER_PACKET_SIZE - PEER_N_PACKET_MESSAGES -
      PEER_N_MESSAGE_DATA, /* Message size acquires 10 bits, so max
                              possible value for it is 1023. */

  /*  Error codes.
   */

  PEER_ERROR_BAD_ALLOC            = 1,
  PEER_ERROR_INVALID_CIPHER       = 2,
  PEER_ERROR_INVALID_KEY          = 3,
  PEER_ERROR_INVALID_PEER         = 4,
  PEER_ERROR_INVALID_MESSAGE      = 5,
  PEER_ERROR_INVALID_IDS          = 6,
  PEER_ERROR_INVALID_TIME_ELAPSED = 7,
  PEER_ERROR_NO_FREE_SLOTS        = 8,
  PEER_ERROR_NOT_IMPLEMENTED      = -1
};

#ifdef __cplusplus
}
#endif

#endif
