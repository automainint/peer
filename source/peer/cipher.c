#include "cipher.h"

#include <assert.h>
#include <string.h>

kit_status_t peer_cipher_init(peer_cipher_t *cipher,
                              uint8_t const *key) {
  assert(cipher != NULL);
  assert(key != NULL);

  if (cipher == NULL)
    return PEER_ERROR_INVALID_CIPHER;

  memset(cipher, 0, sizeof *cipher);

  if (key == NULL)
    return PEER_ERROR_INVALID_KEY;

  memcpy(cipher->key, key, PEER_CIPHER_KEY_SIZE);

  /*  TODO
   *  Initialize the cipher with mt64 random number generator.
   */

  return PEER_ERROR_NOT_IMPLEMENTED;
}

peer_cipher_result_t peer_encrypt(peer_cipher_t *const     cipher,
                                  peer_packets_ref_t const packets,
                                  kit_allocator_t const    alloc) {
  assert(cipher != NULL);

  peer_cipher_result_t result;
  memset(&result, 0, sizeof result);

  if (cipher == NULL) {
    result.status = PEER_ERROR_INVALID_CIPHER;
    return result;
  }

  /*  TODO
   */

  result.status = PEER_ERROR_NOT_IMPLEMENTED;
  return result;
}

peer_cipher_result_t peer_decrypt(peer_cipher_t *const     cipher,
                                  peer_packets_ref_t const packets,
                                  kit_allocator_t const    alloc) {
  assert(cipher != NULL);

  peer_cipher_result_t result;
  memset(&result, 0, sizeof result);

  if (cipher == NULL) {
    result.status = PEER_ERROR_INVALID_CIPHER;
    return result;
  }

  /*  TODO
   */

  result.status = PEER_ERROR_NOT_IMPLEMENTED;
  return result;
}
