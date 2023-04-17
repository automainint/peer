#ifndef PEER_OPTIONS_H
#define PEER_OPTIONS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Undefined value for id, index and agent.
 */
#define PEER_UNDEFINED -1

enum {
  /*  Internal constants.
   */

  PEER_VERSION     = 1,
  PEER_DEVELOPMENT = 1,

  PEER_ADDRESS_SIZE =
      64, /* Address size should be big enough to contain IPv4 and
             IPv6 addresses and ports (6 bytes or 18 bytes). */

  /*  Protocol settings.
   */

  PEER_PACKET_SIZE =
      400, /* On average, peer sends
              PEER_PACKET_SIZE / 10 = 40 kB per second. */

  PEER_MT64_KEY_SIZE = 128, /* Key size for mt64 stream cipher. */

  PEER_TRAIL_SERIAL_SIZE =
      5, /* Number of recent messages to resend. */

  PEER_TRAIL_SCATTER_SIZE =
      5, /* Number of randomly picked previous messages to resend. */

  PEER_TRAIL_SCATTER_DISTANCE =
      100, /* Maximum distance of randomly picked previous messages to
              resend. */

  PEER_TIMEOUT_HEARTBEAT =
      10, /* Peer will send a heartbeat notification if no messages
             was sent in 10 ms. */

  PEER_TIMEOUT_PING =
      200, /* Peer will send a ping request every 200 ms. */

  PEER_TIMEOUT_CONNECTION =
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
  PEER_M_SESSION_RESUME   = 6,

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

  PEER_ERROR_BAD_ALLOC                      = 0x00000001,
  PEER_ERROR_INVALID_CIPHER                 = 0x00000002,
  PEER_ERROR_INVALID_KEY                    = 0x00000004,
  PEER_ERROR_INVALID_PEER                   = 0x00000008,
  PEER_ERROR_INVALID_MODE                   = 0x00000010,
  PEER_ERROR_INVALID_MESSAGE                = 0x00000020,
  PEER_ERROR_INVALID_ID                     = 0x00000040,
  PEER_ERROR_INVALID_TIME_ELAPSED           = 0x00000080,
  PEER_ERROR_INVALID_MESSAGE_SIZE           = 0x00000100,
  PEER_ERROR_INVALID_PACKET_SIZE            = 0x00000200,
  PEER_ERROR_INVALID_MESSAGE_INDEX          = 0x00000400,
  PEER_ERROR_INVALID_MESSAGE_TIME           = 0x00000800,
  PEER_ERROR_INVALID_MESSAGE_ACTOR          = 0x00001040,
  PEER_ERROR_NO_FREE_SLOTS                  = 0x00002000,
  PEER_ERROR_SLOT_NOT_FOUND                 = 0x00004000,
  PEER_ERROR_UNKNOWN_SERVICE_ID             = 0x00008000,
  PEER_ERROR_INVALID_OUT_INDEX              = 0x00010000,
  PEER_ERROR_TIME_OVERFLOW                  = 0x00020000,
  PEER_ERROR_INVALID_SLOT_STATE             = 0x00040000,
  PEER_ERROR_INVALID_POOL                   = 0x00080000,
  PEER_ERROR_UNKNOWN_PROTOCOL               = 0x00100000,
  PEER_ERROR_INVALID_COUNT                  = 0x00200000,
  PEER_ERROR_CREATE_SOCKET_FAILED           = 0x00400000,
  PEER_ERROR_MAKE_SOCKET_NONBLOCKING_FAILED = 0x00800000,
  PEER_ERROR_BIND_SOCKET_FAILED             = 0x01000000,
  PEER_ERROR_GET_SOCKET_NAME_FAILED         = 0x02000000,
  PEER_ERROR_INVALID_ADDRESS                = 0x04000000,
  PEER_ERROR_INVALID_PORT                   = 0x08000000,
  PEER_ERROR_SOCKET_RECEIVE_FAILED          = 0x10000000,
  PEER_ERROR_SOCKET_SEND_FAILED             = 0x20000000,
  PEER_ERROR_INVALID_SOCKET                 = 0x40000000,
  PEER_ERROR_NOT_IMPLEMENTED                = -1
};

typedef int64_t peer_time_t;

#ifdef __cplusplus
}
#endif

#endif
