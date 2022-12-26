#include "../../peer/packet.h"
#include "../../peer/serial.h"

#define KIT_TEST_FILE packet
#include <kit_test/test.h>

TEST("packet pack should always return at least one packet") {
  kit_allocator_t alloc = kit_alloc_default();

  peer_messages_ref_t const mref = { .size = 0, .values = NULL };

  peer_packets_t packets;
  DA_INIT(packets, 0, alloc);

  REQUIRE(peer_pack(0, 1, mref, &packets) == KIT_OK);
  REQUIRE(packets.size == 1);

  DA_DESTROY(packets);
}

TEST("packet pack and unpack several messages") {
  kit_allocator_t alloc = kit_alloc_default();

  uint8_t messages[] = {
    1, 2,  3, 4,  5, 6, 7, 8,  9, 10, 1, 2,  3,  4,  5,  6,
    7, 8,  9, 10, 1, 2, 3, 4,  5, 6,  7, 8,  9,  10, 1,  2,
    3, 4,  5, 6,  7, 8, 9, 10, 1, 2,  3, 4,  5,  6,  7,  8,
    9, 10, 1, 2,  3, 4, 5, 6,  7, 8,  9, 10, 11, 12, 1,  2,
    3, 4,  5, 6,  7, 8, 9, 10, 1, 2,  3, 4,  5,  6,  7,  8,
    9, 10, 1, 2,  3, 4, 5, 6,  7, 8,  9, 10, 11, 12, 13, 14
  };

  peer_write_message_size(messages, 30);
  peer_write_message_size(messages + 30, 32);
  peer_write_message_size(messages + 62, 34);

  peer_message_ref_t const mrefs[] = {
    { .size = 30, .values = messages },
    { .size = 32, .values = messages + 30 },
    { .size = 34, .values = messages + 62 }
  };

  peer_messages_ref_t const mref = { .size = 3, .values = mrefs };

  peer_packets_t packets;
  DA_INIT(packets, 0, alloc);

  REQUIRE(peer_pack(0, 1, mref, &packets) == KIT_OK);

  peer_packets_ref_t const pref = { .size   = packets.size,
                                    .values = packets.values };

  peer_messages_t foo;
  DA_INIT(foo, 0, alloc);

  REQUIRE(peer_unpack(pref, &foo) == KIT_OK);

  REQUIRE(foo.size == 3);
  REQUIRE(foo.size == 3 && AR_EQUAL(mrefs[0], foo.values[0]));
  REQUIRE(foo.size == 3 && AR_EQUAL(mrefs[1], foo.values[1]));
  REQUIRE(foo.size == 3 && AR_EQUAL(mrefs[2], foo.values[2]));

  DA_DESTROY(packets);
  for (ptrdiff_t i = 0; i < foo.size; i++) DA_DESTROY(foo.values[i]);
  DA_DESTROY(foo);
}

TEST("packet pack and unpack a lot of messages") {
  kit_allocator_t alloc = kit_alloc_default();

  uint8_t messages[2000];

  for (ptrdiff_t i = 0; i < sizeof messages; i++) messages[i] = i;

  for (ptrdiff_t i = 0; i < 10; i++)
    peer_write_message_size(messages + i * 200, 200);

  peer_message_ref_t const mrefs[] = {
    { .size = 200, .values = messages },
    { .size = 200, .values = messages + 200 },
    { .size = 200, .values = messages + 400 },
    { .size = 200, .values = messages + 600 },
    { .size = 200, .values = messages + 800 },
    { .size = 200, .values = messages + 1000 },
    { .size = 200, .values = messages + 1200 },
    { .size = 200, .values = messages + 1400 },
    { .size = 200, .values = messages + 1600 },
    { .size = 200, .values = messages + 1800 }
  };

  peer_messages_ref_t const mref = { .size = 10, .values = mrefs };

  peer_packets_t packets;
  DA_INIT(packets, 0, alloc);

  REQUIRE(peer_pack(0, 1, mref, &packets) == KIT_OK);

  peer_packets_ref_t const pref = { .size   = packets.size,
                                    .values = packets.values };

  peer_messages_t foo;
  DA_INIT(foo, 0, alloc);

  REQUIRE(peer_unpack(pref, &foo) == KIT_OK);

  REQUIRE(foo.size == 10);
  REQUIRE(foo.size == 10 && AR_EQUAL(mrefs[0], foo.values[0]));
  REQUIRE(foo.size == 10 && AR_EQUAL(mrefs[1], foo.values[1]));
  REQUIRE(foo.size == 10 && AR_EQUAL(mrefs[2], foo.values[2]));
  REQUIRE(foo.size == 10 && AR_EQUAL(mrefs[3], foo.values[3]));
  REQUIRE(foo.size == 10 && AR_EQUAL(mrefs[4], foo.values[4]));
  REQUIRE(foo.size == 10 && AR_EQUAL(mrefs[5], foo.values[5]));
  REQUIRE(foo.size == 10 && AR_EQUAL(mrefs[6], foo.values[6]));
  REQUIRE(foo.size == 10 && AR_EQUAL(mrefs[7], foo.values[7]));
  REQUIRE(foo.size == 10 && AR_EQUAL(mrefs[8], foo.values[8]));
  REQUIRE(foo.size == 10 && AR_EQUAL(mrefs[9], foo.values[9]));

  DA_DESTROY(packets);
  for (ptrdiff_t i = 0; i < foo.size; i++) DA_DESTROY(foo.values[i]);
  DA_DESTROY(foo);
}
