#ifndef PEER_OPTIONS_H
#define PEER_OPTIONS_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
  PEER_ID_UNDEFINED    = -1,
  PEER_ACTOR_SELF      = -1,
  PEER_ADDRESS_SIZE    = 80,
  PEER_PACKET_SIZE     = 512,
  PEER_CIPHER_KEY_SIZE = 256,

  /*  Packet is not encrypted.
   */

  PEER_PACKET_MODE_PLAIN = 0,

  /*  Packet is encrypted with mt64 cipher.
   */

  PEER_PACKET_MODE_MT64 = 1,

  /*  Service-level message.
   */

  PEER_MESSAGE_MODE_SERVICE = 0,

  /*  Application-level message.
   */

  PEER_MESSAGE_MODE_APPLICATION = 1,

  /*  Data offsets.
   */

  PEER_N_PACKET_INDEX    = 0, /* 8 bytes */
  PEER_N_PACKET_MODE     = 8, /* 2 bytes */
  PEER_N_PACKET_MESSAGES = 10,

  PEER_N_MESSAGE_CHECKSUM = 0,  /* 16 bytes */
  PEER_N_MESSAGE_INDEX    = 16, /*  8 bytes */
  PEER_N_MESSAGE_TIME     = 24, /*  8 bytes */
  PEER_N_MESSAGE_ACTOR    = 32, /*  8 bytes */
  PEER_N_MESSAGE_MODE     = 40, /*  2 bytes */
  PEER_N_MESSAGE_SIZE     = 42, /*  2 bytes */
  PEER_N_MESSAGE_DATA     = 44,

  PEER_MAX_MESSAGE_SIZE = PEER_PACKET_SIZE - PEER_N_PACKET_MESSAGES -
                          PEER_N_MESSAGE_DATA,

  /*  Error codes.
   */

  PEER_ERROR_BAD_ALLOC       = 1,
  PEER_ERROR_INVALID_CIPHER  = 2,
  PEER_ERROR_INVALID_KEY     = 3,
  PEER_ERROR_INVALID_PEER    = 4,
  PEER_ERROR_INVALID_MESSAGE = 5,
  PEER_ERROR_INVALID_IDS     = 6,
  PEER_ERROR_NO_FREE_SLOTS   = 7,
  PEER_ERROR_NOT_IMPLEMENTED = -1
};

#ifdef __cplusplus
}
#endif

#endif
