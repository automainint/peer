#include "../../peer/peer.h"

#define KIT_TEST_FILE_NAME peer
#include <kit_test/test.h>

/*  TODO
 *  - Update state host to client.
 *  - Update state client to client.
 *  - Actors.
 *  - Heartbeat packets.
 *  - Ping.
 *  - Connection timeout.
 *  - Multiple clients.
 *  - Relay.
 *  - Lobby.
 *  - Session token.
 *  - Session version.
 *  - Reconnect.
 *  - Encryption.
 *  - Predefined public keys.
 */

TEST("peer update state host to client") {
  /*  Initialize host and client.
   */
  peer_t host, client;

  REQUIRE(peer_init(&host, PEER_HOST, kit_alloc_default()) == KIT_OK);
  REQUIRE(peer_init(&client, PEER_CLIENT, kit_alloc_default()) ==
          KIT_OK);

  /*  Open sockets.
   */
  ptrdiff_t const      sockets[]      = { 1, 2, 3 };
  peer_ids_ref_t const host_sockets   = { .size   = 2,
                                          .values = sockets },
                       client_sockets = { .size   = 1,
                                          .values = sockets + 2 };

  REQUIRE(peer_open(&host, host_sockets) == KIT_OK);
  REQUIRE(host.slots.size == 2 && host.slots.values[0].local.id == 1);
  REQUIRE(host.slots.size == 2 && host.slots.values[1].local.id == 2);

  REQUIRE(peer_open(&client, client_sockets) == KIT_OK);
  REQUIRE(client.slots.size == 1 &&
          client.slots.values[0].local.id == 3);

  /*  Put data to the host.
   */
  uint8_t            data[]      = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  peer_message_ref_t data_ref[3] = {
    { .size = 2, .values = data },
    { .size = 4, .values = data + 2 },
    { .size = 3, .values = data + 6 }
  };

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

  /*  Initialize client-to-host connection.
   */
  REQUIRE(peer_connect(&client, host_sockets.values[0]) == KIT_OK);

  /*  Client will try to connect and generate packets which must be
   *  sent to the host.
   */
  peer_tick_result_t tick_result;
  peer_packets_ref_t packets_ref;

  tick_result = peer_tick(&client, 0);
  REQUIRE(tick_result.status == KIT_OK);

  packets_ref.size   = tick_result.packets.size;
  packets_ref.values = tick_result.packets.values;
  REQUIRE(peer_input(&host, packets_ref) == KIT_OK);
  DA_DESTROY(tick_result.packets);

  /*  Host will create a new session and generate packets which must
   *  be sent to the client.
   */
  tick_result = peer_tick(&host, 0);
  REQUIRE(tick_result.status == KIT_OK);

  packets_ref.size   = tick_result.packets.size;
  packets_ref.values = tick_result.packets.values;
  REQUIRE(peer_input(&client, packets_ref) == KIT_OK);
  DA_DESTROY(tick_result.packets);

  /*  Client will join the session and generate packets which must be
   *  sent to the host.
   */
  tick_result = peer_tick(&client, 0);
  REQUIRE(tick_result.status == KIT_OK);

  packets_ref.size   = tick_result.packets.size;
  packets_ref.values = tick_result.packets.values;
  REQUIRE(peer_input(&host, packets_ref) == KIT_OK);
  DA_DESTROY(tick_result.packets);

  /*  Host will accept the client and generate packets which must be
   *  sent to the client.
   */
  tick_result = peer_tick(&host, 0);
  REQUIRE(tick_result.status == KIT_OK);

  packets_ref.size   = tick_result.packets.size;
  packets_ref.values = tick_result.packets.values;
  REQUIRE(peer_input(&client, packets_ref) == KIT_OK);
  DA_DESTROY(tick_result.packets);

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

