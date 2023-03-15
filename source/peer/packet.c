#include "packet.h"

#include "serial.h"

kit_status_t peer_pack(ptrdiff_t const         source_id,
                       ptrdiff_t const         destination_id,
                       peer_chunks_ref_t const chunks,
                       peer_packets_t *const   out_packets) {
  /*  Pack chunks into packets.
   */

  assert(chunks.size >= 0);
  assert(chunks.size == 0 || chunks.values != NULL);
  assert(source_id != destination_id);
  assert(out_packets != NULL);

  ptrdiff_t previous_size = out_packets->size;
  ptrdiff_t offset        = PEER_PACKET_SIZE;

  for (ptrdiff_t i = 0; i < chunks.size; i++) {
    if (offset + chunks.values[i].size > PEER_PACKET_SIZE) {
      if (out_packets->size > previous_size) {
        /*  Write the previous packet header.
         */

        assert(offset < 65536);
        assert(offset < PEER_PACKET_SIZE - PEER_N_PACKET_MESSAGES);

        out_packets->values[out_packets->size - 1].size = offset;

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

    {
      /*  Make sure, the chunk size value is correct.
       */

      ptrdiff_t const chunk_size = (ptrdiff_t) peer_read_message_size(
          chunks.values[i].values);

      assert(chunks.values[i].size == chunk_size);

      if (chunks.values[i].size != chunk_size)
        return PEER_ERROR_INVALID_MESSAGE_SIZE;
    }

    {
      /*  Add chunk to the current packet.
       */

      assert(out_packets->size > 0);
      assert(chunks.values[i].size + PEER_N_PACKET_MESSAGES <
             PEER_PACKET_SIZE);

      ptrdiff_t const m = out_packets->size - 1;

      memcpy(out_packets->values[m].data + offset,
             chunks.values[i].values, chunks.values[i].size);
    }

    offset += chunks.values[i].size;
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

    out_packets->values[out_packets->size - 1].size = offset;

    uint8_t *const data =
        out_packets->values[out_packets->size - 1].data;

    peer_write_u8(data + PEER_N_PACKET_MODE, PEER_PACKET_MODE_PLAIN);
    peer_write_u16(data + PEER_N_PACKET_SIZE, (uint16_t) offset);
  }

  return KIT_OK;
}

kit_status_t peer_unpack(peer_packets_ref_t const packets,
                         peer_chunks_t *const     out_chunks) {
  /*  Unpack chunks from packets.
   */

  assert(packets.size >= 0);
  assert(packets.size == 0 || packets.values != NULL);
  assert(out_chunks != NULL);

  kit_status_t status = KIT_OK;

  for (ptrdiff_t i = 0; i < packets.size; i++) {
    if (packets.values[i].size == 0)
      continue;

    ptrdiff_t offset = PEER_N_PACKET_MESSAGES;

    assert(packets.values[i].size >= offset &&
           packets.values[i].size <= PEER_PACKET_SIZE);

    if (packets.values[i].size < offset ||
        packets.values[i].size > PEER_PACKET_SIZE) {
      status |= PEER_ERROR_INVALID_PACKET_SIZE;
      continue;
    }

    while (offset + PEER_N_MESSAGE_DATA <= packets.values[i].size) {
      ptrdiff_t const size = (ptrdiff_t) peer_read_message_size(
          packets.values[i].data + offset);

      if (size == 0)
        break;

      if (size < 0 || offset + size > PEER_PACKET_SIZE) {
        status |= PEER_ERROR_INVALID_MESSAGE_SIZE;
        break;
      }

      ptrdiff_t const n = out_chunks->size;
      DA_RESIZE(*out_chunks, n + 1);
      if (out_chunks->size != n + 1) {
        status |= PEER_ERROR_BAD_ALLOC;
        break;
      }

      DA_INIT(out_chunks->values[n], size, out_chunks->alloc);
      if (out_chunks->values[n].size != size) {
        status |= PEER_ERROR_BAD_ALLOC;
        break;
      }

      assert(out_chunks->values[n].values != NULL);

      memcpy(out_chunks->values[n].values,
             packets.values[i].data + offset, size);

      offset += size;
    }
  }

  return status;
}
