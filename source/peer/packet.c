#include "packet.h"

#include <string.h>

peer_pack_result_t peer_pack(peer_messages_ref_t const messages,
                             kit_allocator_t const     alloc) {
  peer_pack_result_t result;
  memset(&result, 0, sizeof result);

  /*  TODO
   */

  result.status = PEER_ERROR_NOT_IMPLEMENTED;
  return result;
}

peer_unpack_result_t peer_unpack(peer_packets_ref_t const packets,
                                 kit_allocator_t const    alloc) {
  peer_unpack_result_t result;
  memset(&result, 0, sizeof result);

  /*  TODO
   */

  result.status = PEER_ERROR_NOT_IMPLEMENTED;
  return result;
}
