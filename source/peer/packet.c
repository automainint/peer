#include "packet.h"

#include "serial.h"

kit_status_t peer_pack(ptrdiff_t const           source_id,
                       ptrdiff_t const           destination_id,
                       peer_messages_ref_t const messages,
                       peer_packets_t *const     out_packets) {
  /*  Pack messages into packets.
   */

  assert(messages.size >= 0);
  assert(messages.size == 0 || messages.values != NULL);
  assert(source_id != destination_id);
  assert(out_packets != NULL);

  ptrdiff_t previous_size = out_packets->size;
  ptrdiff_t offset        = PEER_PACKET_SIZE;

  for (ptrdiff_t i = 0; i < messages.size; i++) {
    if (offset + messages.values[i].size > PEER_PACKET_SIZE) {
      if (out_packets->size > previous_size) {
        /*  Write the previous packet header.
         */

        assert(offset < 65536);
        assert(offset < PEER_PACKET_SIZE - PEER_N_PACKET_MESSAGES);

        uint8_t *const data =
            out_packets->values[out_packets->size - 1].data;

        peer_write_u8(data + PEER_N_PACKET_MODE,
                      PEER_PACKET_MODE_PLAIN);
        peer_write_u16(data + PEER_N_PACKET_SIZE, (uint16_t) offset);
      }

      /*  Add a new packet.
       */

      ptrdiff_t const n = out_packets->size;
      DA_RESIZE(*out_packets, n + 1);

      if (out_packets->size != n + 1)
        return PEER_ERROR_BAD_ALLOC;

      memset(out_packets->values + n, 0, sizeof *out_packets->values);
      out_packets->values[n].source_id      = source_id;
      out_packets->values[n].destination_id = destination_id;

      offset = PEER_N_PACKET_MESSAGES;
    }

    /*  Make sure, the message size value is correct.
     */

    assert(
        messages.values[i].size ==
        (((uint16_t) peer_read_u8(messages.values[i].values +
                                  PEER_N_MESSAGE_SIZE)) |
         ((((uint16_t) peer_read_u8(messages.values[i].values +
                                    PEER_N_MESSAGE_SIZE_AND_MODE)) &
           0x03)
          << 8)));

    /*  Add message to the current packet.
     */

    assert(out_packets->size > 0);
    assert(messages.values[i].size + PEER_N_PACKET_MESSAGES <
           PEER_PACKET_SIZE);

    ptrdiff_t const m = out_packets->size - 1;

    memcpy(out_packets->values[m].data + offset,
           messages.values[i].values, messages.values[i].size);

    offset += messages.values[i].size;
  }

  if (out_packets->size == previous_size) {
    /*  Make sure to create at least 1 packet.
     */

    DA_RESIZE(*out_packets, previous_size + 1);

    if (out_packets->size != previous_size + 1)
      return PEER_ERROR_BAD_ALLOC;

    memset(out_packets->values, 0, sizeof *out_packets->values);
    out_packets->values[0].source_id      = source_id;
    out_packets->values[0].destination_id = destination_id;

    offset = 0;
  }

  {
    /*  Write the last packet header.
     */

    assert(out_packets->size > 0);
    assert(offset < 65536);
    assert(offset < PEER_PACKET_SIZE - PEER_N_PACKET_MESSAGES);

    uint8_t *const data =
        out_packets->values[out_packets->size - 1].data;

    peer_write_u8(data + PEER_N_PACKET_MODE, PEER_PACKET_MODE_PLAIN);
    peer_write_u16(data + PEER_N_PACKET_SIZE, (uint16_t) offset);
  }

  return KIT_OK;
}

kit_status_t peer_unpack(peer_packets_ref_t const packets,
                         peer_messages_t *const   out_messages) {
  /*  Unpack messages from packets.
   */

  assert(packets.size >= 0);
  assert(packets.size == 0 || packets.values != NULL);
  assert(out_messages != NULL);

  kit_status_t status = KIT_OK;

  for (ptrdiff_t i = 0; i < packets.size; i++) {
    ptrdiff_t offset = PEER_N_PACKET_MESSAGES;

    while (offset + PEER_N_MESSAGE_DATA <= PEER_PACKET_SIZE) {
      uint8_t const s0 = peer_read_u8(packets.values[i].data +
                                      offset + PEER_N_MESSAGE_SIZE);
      uint8_t const s1 = peer_read_u8(packets.values[i].data +
                                      offset +
                                      PEER_N_MESSAGE_SIZE_AND_MODE) &
                         0x3;
      ptrdiff_t const size = (ptrdiff_t) (((uint16_t) s0) |
                                          (((uint16_t) s1) << 8));

      if (size == 0)
        break;

      if (size < 0 || offset + size > PEER_PACKET_SIZE) {
        status |= PEER_ERROR_INVALID_MESSAGE_SIZE;
        break;
      }

      ptrdiff_t const n = out_messages->size;
      DA_RESIZE(*out_messages, n + 1);
      if (out_messages->size != n + 1) {
        status |= PEER_ERROR_BAD_ALLOC;
        break;
      }

      DA_INIT(out_messages->values[n], size, out_messages->alloc);
      if (out_messages->values[n].size != size) {
        status |= PEER_ERROR_BAD_ALLOC;
        break;
      }

      assert(out_messages->values[n].values != NULL);

      memcpy(out_messages->values[n].values,
             packets.values[i].data + offset, size);

      offset += size;
    }
  }

  return status;
}
