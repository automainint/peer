#include "../../peer/peer.h"

#define KIT_TEST_FILE_NAME peer
#include <kit_test/test.h>

TEST("peer update state host to client") {
  peer_t             host, client;
  peer_packets_t     packets;
  peer_packets_ref_t packets_ref;

  /*  Initialize host and client.
   */
  REQUIRE(peer_init_host(&host, 1, kit_alloc_default()) == KIT_OK);
  REQUIRE(peer_init_client(&client, 2, kit_alloc_default()) ==
          KIT_OK);

  /*  Put data to the host.
   */
  uint8_t         data[]      = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  peer_data_ref_t data_ref[3] = { { .size = 2, .values = data },
                                  { .size = 4, .values = data + 2 },
                                  { .size = 3, .values = data + 6 } };

  REQUIRE(peer_queue(&host, data_ref[0]) == KIT_OK);
  REQUIRE(peer_queue(&host, data_ref[1]) == KIT_OK);
  REQUIRE(peer_queue(&host, data_ref[2]) == KIT_OK);

  /*  Check if host's data was updated.
   */
  REQUIRE(host.queue.size == 3 && host.queue.values[0].time == 0);
  REQUIRE(host.queue.size == 3 && host.queue.values[0].actor == 0);
  REQUIRE(host.queue.size == 3 && host.queue.values[0].size == 2);
  REQUIRE(host.queue.size == 3 && host.queue.values[0].offset == 0);
  REQUIRE(host.queue.size == 3 && host.queue.values[1].time == 0);
  REQUIRE(host.queue.size == 3 && host.queue.values[1].actor == 0);
  REQUIRE(host.queue.size == 3 && host.queue.values[1].size == 4);
  REQUIRE(host.queue.size == 3 && host.queue.values[1].offset == 2);
  REQUIRE(host.queue.size == 3 && host.queue.values[2].time == 0);
  REQUIRE(host.queue.size == 3 && host.queue.values[2].actor == 0);
  REQUIRE(host.queue.size == 3 && host.queue.values[2].size == 3);
  REQUIRE(host.queue.size == 3 && host.queue.values[2].offset == 6);
  REQUIRE(host.queue.size == 3 && host.buffer.size >= 9 &&
          kit_ar_equal_bytes(1, 9, data, 1, 9, host.buffer.values));

  /*  Initialize client to host connection.
   */
  REQUIRE(peer_connect(&client, 1) == KIT_OK);

  /*  Client will generate packets which must be sent to the host.
   */
  packets            = peer_tick(&client, 0);
  packets_ref.size   = packets.size;
  packets_ref.values = packets.values;
  REQUIRE(peer_input(&host, packets_ref) == KIT_OK);
  DA_DESTROY(packets);
  REQUIRE(client.status == KIT_OK);

  /*  Host will generate packets which must be sent to the client.
   */
  packets            = peer_tick(&host, 0);
  packets_ref.size   = packets.size;
  packets_ref.values = packets.values;
  REQUIRE(peer_input(&client, packets_ref) == KIT_OK);
  DA_DESTROY(packets);
  REQUIRE(host.status == KIT_OK);

  /*  Check if client's data was updated.
   */
  REQUIRE(client.queue.size == 3 && client.queue.values[0].time == 0);
  REQUIRE(client.queue.size == 3 &&
          client.queue.values[0].actor == 0);
  REQUIRE(client.queue.size == 3 && client.queue.values[0].size == 2);
  REQUIRE(client.queue.size == 3 &&
          client.queue.values[0].offset == 0);
  REQUIRE(client.queue.size == 3 && client.queue.values[1].time == 0);
  REQUIRE(client.queue.size == 3 &&
          client.queue.values[1].actor == 0);
  REQUIRE(client.queue.size == 3 && client.queue.values[1].size == 4);
  REQUIRE(client.queue.size == 3 &&
          client.queue.values[1].offset == 2);
  REQUIRE(client.queue.size == 3 && client.queue.values[2].time == 0);
  REQUIRE(client.queue.size == 3 &&
          client.queue.values[2].actor == 0);
  REQUIRE(client.queue.size == 3 && client.queue.values[2].size == 3);
  REQUIRE(client.queue.size == 3 &&
          client.queue.values[2].offset == 6);
  REQUIRE(client.queue.size == 3 && client.buffer.size >= 9 &&
          kit_ar_equal_bytes(1, 9, data, 1, 9, client.buffer.values));

  /*  Destroy the host and the client.
   */
  REQUIRE(peer_destroy(&host) == KIT_OK);
  REQUIRE(peer_destroy(&client) == KIT_OK);
}

