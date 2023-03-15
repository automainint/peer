#include "../../peer/peer.h"

#include <string.h>

#define KIT_TEST_FILE peer
#include <kit_test/test.h>

/*  TODO
 *  - Heartbeat packets.
 *  - Ping.
 *  - Connection timeout.
 *  - Multiple clients.
 *  - Relay.
 *  - Lobby.
 *  - Session token.
 *  - Session version.
 *  - Reconnect.
 *  - Messages duplication.
 *  - Encryption.
 *  - Predefined public keys.
 */

static int has_id_(peer_t const *const peer, ptrdiff_t const id) {
  for (ptrdiff_t i = 0; i < peer->slots.size; i++)
    if (peer->slots.values[i].local.id == id)
      return 1;
  return 0;
}

static int resolve_address_id_(peer_t *const       client,
                               peer_t const *const host) {
  if (client->slots.size == 0)
    return 0;
  if (client->slots.values[0].remote.address_size != 1)
    return 0;
  client->slots.values[0].remote.id =
      client->slots.values[0].remote.address_data[0];
  return has_id_(host, client->slots.values[0].remote.id);
}

static int send_packets_to_(peer_tick_result_t const tick,
                            peer_t *const            peer) {
  if (tick.status != KIT_OK)
    return 0;

  peer_packets_t packets;
  DA_INIT(packets, tick.packets.size, kit_alloc_default());
  DA_RESIZE(packets, 0);

  for (ptrdiff_t i = 0; i < tick.packets.size; i++)
    if (has_id_(peer, tick.packets.values[i].destination_id)) {
      ptrdiff_t const n = packets.size;
      DA_RESIZE(packets, n + 1);
      if (packets.size != n + 1) {
        DA_DESTROY(packets);
        return 0;
      }
      memcpy(packets.values + n, tick.packets.values + i,
             sizeof *packets.values);
    }

  peer_packets_ref_t const ref = { .size   = packets.size,
                                   .values = packets.values };

  int ok = 1;

  if (peer_input(peer, ref) != KIT_OK)
    ok = 0;

  DA_DESTROY(packets);
  return ok;
}

static int send_packets_to_and_free_(peer_tick_result_t const tick,
                                     peer_t *const            peer) {
  int const ok = send_packets_to_(tick, peer);
  DA_DESTROY(tick.packets);
  return ok;
}

TEST("peer host to client initial state update") {
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

  if (host.slots.size == 2) {
    /*  Specify the address data for host.
     */
    host.slots.values[1].local.address_size    = 1;
    host.slots.values[1].local.address_data[0] = 2;
  }

  REQUIRE(peer_open(&client, client_sockets) == KIT_OK);
  REQUIRE(client.slots.size == 1 &&
          client.slots.values[0].local.id == 3);

  /*  Put data to the host.
   */
  uint8_t          data[]      = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  peer_chunk_ref_t data_ref[3] = { { .size = 2, .values = data },
                                   { .size = 4, .values = data + 2 },
                                   { .size   = 3,
                                     .values = data + 6 } };

  REQUIRE(peer_queue(&host, data_ref[0]) == KIT_OK);
  REQUIRE(peer_queue(&host, data_ref[1]) == KIT_OK);
  REQUIRE(peer_queue(&host, data_ref[2]) == KIT_OK);

  /*  Check if host's data was updated.
   */
  REQUIRE(host.queue.size == 3);
  REQUIRE(host.queue.size == 3 && host.queue.values[0].time == 0);
  REQUIRE(host.queue.size == 3 && host.queue.values[0].actor == 0);
  REQUIRE(host.queue.size == 3 &&
          host.queue.values[0].data.size == 2);
  REQUIRE(host.queue.size == 3 && host.queue.values[1].time == 0);
  REQUIRE(host.queue.size == 3 && host.queue.values[1].actor == 0);
  REQUIRE(host.queue.size == 3 &&
          host.queue.values[1].data.size == 4);
  REQUIRE(host.queue.size == 3 && host.queue.values[2].time == 0);
  REQUIRE(host.queue.size == 3 && host.queue.values[2].actor == 0);
  REQUIRE(host.queue.size == 3 &&
          host.queue.values[2].data.size == 3);
  REQUIRE(host.queue.size == 3 &&
          kit_ar_equal_bytes(1, 2, data, 1,
                             host.queue.values[0].data.size,
                             host.queue.values[0].data.values));
  REQUIRE(host.queue.size == 3 &&
          kit_ar_equal_bytes(1, 4, data + 2, 1,
                             host.queue.values[1].data.size,
                             host.queue.values[1].data.values));
  REQUIRE(host.queue.size == 3 &&
          kit_ar_equal_bytes(1, 3, data + 6, 1,
                             host.queue.values[2].data.size,
                             host.queue.values[2].data.values));

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

  /*  Check if client did receive the session address and resolve the
   *  address id.
   */
  REQUIRE(resolve_address_id_(&client, &host));

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
  REQUIRE(client.queue.size == 3);
  REQUIRE(client.queue.size == 3 && client.queue.values[0].time == 0);
  REQUIRE(client.queue.size == 3 &&
          client.queue.values[0].actor == 0);
  REQUIRE(client.queue.size == 3 && client.queue.values[1].time == 0);
  REQUIRE(client.queue.size == 3 &&
          client.queue.values[1].actor == 0);
  REQUIRE(client.queue.size == 3 && client.queue.values[2].time == 0);
  REQUIRE(client.queue.size == 3 &&
          client.queue.values[2].actor == 0);
  REQUIRE(client.queue.size == 3 &&
          kit_ar_equal_bytes(1, 2, data, 1,
                             client.queue.values[0].data.size,
                             client.queue.values[0].data.values));
  REQUIRE(client.queue.size == 3 &&
          kit_ar_equal_bytes(1, 4, data + 2, 1,
                             client.queue.values[1].data.size,
                             client.queue.values[1].data.values));
  REQUIRE(client.queue.size == 3 &&
          kit_ar_equal_bytes(1, 3, data + 6, 1,
                             client.queue.values[2].data.size,
                             client.queue.values[2].data.values));

  /*  Destroy the host and the client.
   */
  REQUIRE(peer_destroy(&host) == KIT_OK);
  REQUIRE(peer_destroy(&client) == KIT_OK);
}

TEST("peer host to client state update once") {
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

  if (host.slots.size == 2) {
    /*  Specify the address data for host.
     */
    host.slots.values[1].local.address_size    = 1;
    host.slots.values[1].local.address_data[0] = 2;
  }

  REQUIRE(peer_open(&client, client_sockets) == KIT_OK);
  REQUIRE(client.slots.size == 1 &&
          client.slots.values[0].local.id == 3);

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

  /*  Check if client did receive the session address and resolve the
   *  address id.
   */
  REQUIRE(resolve_address_id_(&client, &host));

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

  /*  Put data to the host.
   */
  uint8_t          data[]      = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  peer_chunk_ref_t data_ref[3] = { { .size = 2, .values = data },
                                   { .size = 4, .values = data + 2 },
                                   { .size   = 3,
                                     .values = data + 6 } };

  REQUIRE(peer_queue(&host, data_ref[0]) == KIT_OK);
  REQUIRE(peer_queue(&host, data_ref[1]) == KIT_OK);
  REQUIRE(peer_queue(&host, data_ref[2]) == KIT_OK);

  /*  Check if host's data was updated.
   */
  REQUIRE(host.queue.size == 3);
  REQUIRE(host.queue.size == 3 && host.queue.values[0].time == 0);
  REQUIRE(host.queue.size == 3 && host.queue.values[0].actor == 0);
  REQUIRE(host.queue.size == 3 && host.queue.values[1].time == 0);
  REQUIRE(host.queue.size == 3 && host.queue.values[1].actor == 0);
  REQUIRE(host.queue.size == 3 && host.queue.values[2].time == 0);
  REQUIRE(host.queue.size == 3 && host.queue.values[2].actor == 0);
  REQUIRE(host.queue.size == 3 &&
          kit_ar_equal_bytes(1, 2, data, 1,
                             host.queue.values[0].data.size,
                             host.queue.values[0].data.values));
  REQUIRE(host.queue.size == 3 &&
          kit_ar_equal_bytes(1, 4, data + 2, 1,
                             host.queue.values[1].data.size,
                             host.queue.values[1].data.values));
  REQUIRE(host.queue.size == 3 &&
          kit_ar_equal_bytes(1, 3, data + 6, 1,
                             host.queue.values[2].data.size,
                             host.queue.values[2].data.values));

  /*  Host will generate packets which must be sent to the client.
   */
  tick_result = peer_tick(&host, 0);
  REQUIRE(tick_result.status == KIT_OK);

  packets_ref.size   = tick_result.packets.size;
  packets_ref.values = tick_result.packets.values;
  REQUIRE(peer_input(&client, packets_ref) == KIT_OK);
  DA_DESTROY(tick_result.packets);

  /*  Check if client's data was updated.
   */
  REQUIRE(client.queue.size == 3);
  REQUIRE(client.queue.size == 3 && client.queue.values[0].time == 0);
  REQUIRE(client.queue.size == 3 &&
          client.queue.values[0].actor == 0);
  REQUIRE(client.queue.size == 3 && client.queue.values[1].time == 0);
  REQUIRE(client.queue.size == 3 &&
          client.queue.values[1].actor == 0);
  REQUIRE(client.queue.size == 3 && client.queue.values[2].time == 0);
  REQUIRE(client.queue.size == 3 &&
          client.queue.values[2].actor == 0);
  REQUIRE(client.queue.size == 3 &&
          kit_ar_equal_bytes(1, 2, data, 1,
                             client.queue.values[0].data.size,
                             client.queue.values[0].data.values));
  REQUIRE(client.queue.size == 3 &&
          kit_ar_equal_bytes(1, 4, data + 2, 1,
                             client.queue.values[1].data.size,
                             client.queue.values[1].data.values));
  REQUIRE(client.queue.size == 3 &&
          kit_ar_equal_bytes(1, 3, data + 6, 1,
                             client.queue.values[2].data.size,
                             client.queue.values[2].data.values));

  /*  Destroy the host and the client.
   */
  REQUIRE(peer_destroy(&host) == KIT_OK);
  REQUIRE(peer_destroy(&client) == KIT_OK);
}

TEST("peer host to client state update twice") {
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

  if (host.slots.size == 2) {
    /*  Specify the address data for host.
     */
    host.slots.values[1].local.address_size    = 1;
    host.slots.values[1].local.address_data[0] = 2;
  }

  REQUIRE(peer_open(&client, client_sockets) == KIT_OK);
  REQUIRE(client.slots.size == 1 &&
          client.slots.values[0].local.id == 3);

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

  /*  Check if client did receive the session address and resolve the
   *  address id.
   */
  REQUIRE(resolve_address_id_(&client, &host));

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

  /*  Put data to the host.
   */
  uint8_t          data[]      = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  peer_chunk_ref_t data_ref[3] = { { .size = 2, .values = data },
                                   { .size = 4, .values = data + 2 },
                                   { .size   = 3,
                                     .values = data + 6 } };

  REQUIRE(peer_queue(&host, data_ref[0]) == KIT_OK);

  /*  Check if host's data was updated.
   */
  REQUIRE(host.queue.size == 1);
  REQUIRE(host.queue.size == 1 && host.queue.values[0].time == 0);
  REQUIRE(host.queue.size == 1 && host.queue.values[0].actor == 0);
  REQUIRE(host.queue.size == 1 &&
          kit_ar_equal_bytes(1, 2, data, 1,
                             host.queue.values[0].data.size,
                             host.queue.values[0].data.values));

  /*  Host will generate packets which must be sent to the client.
   */
  tick_result = peer_tick(&host, 0);
  REQUIRE(tick_result.status == KIT_OK);

  packets_ref.size   = tick_result.packets.size;
  packets_ref.values = tick_result.packets.values;
  REQUIRE(peer_input(&client, packets_ref) == KIT_OK);
  DA_DESTROY(tick_result.packets);

  /*  Check if client's data was updated.
   */
  REQUIRE(client.queue.size == 1);
  REQUIRE(client.queue.size == 1 && client.queue.values[0].time == 0);
  REQUIRE(client.queue.size == 1 &&
          client.queue.values[0].actor == 0);
  REQUIRE(client.queue.size == 1 &&
          kit_ar_equal_bytes(1, 2, data, 1,
                             client.queue.values[0].data.size,
                             client.queue.values[0].data.values));

  /*  Put data to the host.
   */
  REQUIRE(peer_queue(&host, data_ref[1]) == KIT_OK);
  REQUIRE(peer_queue(&host, data_ref[2]) == KIT_OK);

  /*  Check if host's data was updated.
   */
  REQUIRE(host.queue.size == 3);
  REQUIRE(host.queue.size == 3 && host.queue.values[1].time == 0);
  REQUIRE(host.queue.size == 3 && host.queue.values[1].actor == 0);
  REQUIRE(host.queue.size == 3 && host.queue.values[2].time == 0);
  REQUIRE(host.queue.size == 3 && host.queue.values[2].actor == 0);
  REQUIRE(host.queue.size == 3 &&
          kit_ar_equal_bytes(1, 4, data + 2, 1,
                             host.queue.values[1].data.size,
                             host.queue.values[1].data.values));
  REQUIRE(host.queue.size == 3 &&
          kit_ar_equal_bytes(1, 3, data + 6, 1,
                             host.queue.values[2].data.size,
                             host.queue.values[2].data.values));

  /*  Host will generate packets which must be sent to the client.
   */
  tick_result = peer_tick(&host, 0);
  REQUIRE(tick_result.status == KIT_OK);

  packets_ref.size   = tick_result.packets.size;
  packets_ref.values = tick_result.packets.values;
  REQUIRE(peer_input(&client, packets_ref) == KIT_OK);
  DA_DESTROY(tick_result.packets);

  /*  Check if client's data was updated.
   */
  REQUIRE(client.queue.size == 3);
  REQUIRE(client.queue.size == 3 && client.queue.values[1].time == 0);
  REQUIRE(client.queue.size == 3 &&
          client.queue.values[1].actor == 0);
  REQUIRE(client.queue.size == 3 && client.queue.values[2].time == 0);
  REQUIRE(client.queue.size == 3 &&
          client.queue.values[2].actor == 0);
  REQUIRE(client.queue.size == 3 &&
          kit_ar_equal_bytes(1, 4, data + 2, 1,
                             client.queue.values[1].data.size,
                             client.queue.values[1].data.values));
  REQUIRE(client.queue.size == 3 &&
          kit_ar_equal_bytes(1, 3, data + 6, 1,
                             client.queue.values[2].data.size,
                             client.queue.values[2].data.values));

  /*  Destroy the host and the client.
   */
  REQUIRE(peer_destroy(&host) == KIT_OK);
  REQUIRE(peer_destroy(&client) == KIT_OK);
}

TEST("peer client to client state update") {
  /*  Initialize and connect three peers.
   */

  kit_allocator_t alloc = kit_alloc_default();
  peer_t          host, alice, bob;

  REQUIRE(peer_init(&host, PEER_HOST, alloc) == KIT_OK);
  REQUIRE(peer_init(&alice, PEER_CLIENT, alloc) == KIT_OK);
  REQUIRE(peer_init(&bob, PEER_CLIENT, alloc) == KIT_OK);

  ptrdiff_t const      sockets[]     = { 1, 2, 3, 4, 5, 6 };
  peer_ids_ref_t const host_sockets  = { .size   = 3,
                                         .values = sockets };
  peer_ids_ref_t const alice_sockets = { .size   = 1,
                                         .values = sockets + 3 };
  peer_ids_ref_t const bob_sockets   = { .size   = 1,
                                         .values = sockets + 4 };

  REQUIRE(peer_open(&host, host_sockets) == KIT_OK);
  REQUIRE(peer_open(&alice, alice_sockets) == KIT_OK);
  REQUIRE(peer_open(&bob, bob_sockets) == KIT_OK);

  if (host.slots.size == 3) {
    /*  Specify the address data for host.
     */
    host.slots.values[1].local.address_size    = 1;
    host.slots.values[1].local.address_data[0] = 2;
    host.slots.values[2].local.address_size    = 1;
    host.slots.values[2].local.address_data[0] = 3;
  }

  REQUIRE(peer_connect(&alice, host_sockets.values[0]) == KIT_OK);
  REQUIRE(peer_connect(&bob, host_sockets.values[0]) == KIT_OK);

  peer_tick_result_t tick_result;

  /*  Clients are trying to connect.
   */
  REQUIRE(send_packets_to_and_free_(peer_tick(&alice, 0), &host));
  REQUIRE(send_packets_to_and_free_(peer_tick(&bob, 0), &host));

  /*  Host will create a new session for clients.
   */
  tick_result = peer_tick(&host, 0);
  REQUIRE(send_packets_to_(tick_result, &alice));
  REQUIRE(send_packets_to_(tick_result, &bob));
  DA_DESTROY(tick_result.packets);

  /*  Resolve address ids.
   */
  REQUIRE(resolve_address_id_(&alice, &host));
  REQUIRE(resolve_address_id_(&bob, &host));

  /*  Clients are joining the session.
   */
  REQUIRE(send_packets_to_and_free_(peer_tick(&alice, 0), &host));
  REQUIRE(send_packets_to_and_free_(peer_tick(&bob, 0), &host));

  /*  Host is accepting clients.
   */
  tick_result = peer_tick(&host, 0);
  REQUIRE(send_packets_to_(tick_result, &alice));
  REQUIRE(send_packets_to_(tick_result, &bob));
  DA_DESTROY(tick_result.packets);

  /*  Check peers' actor ids.
   */
  REQUIRE(host.actor != alice.actor);
  REQUIRE(host.actor != bob.actor);
  REQUIRE(alice.actor != bob.actor);

  /*  Put data to Alice.
   */
  uint8_t          data[]   = { 1, 2, 3, 4, 5 };
  peer_chunk_ref_t data_ref = { .size = 5, .values = data };
  REQUIRE(peer_queue(&alice, data_ref) == KIT_OK);

  /*  Alice will send data to the host.
   */
  REQUIRE(send_packets_to_and_free_(peer_tick(&alice, 0), &host));
  REQUIRE(send_packets_to_and_free_(peer_tick(&bob, 0), &host));

  /*  Host will update the data and send updates to clients.
   */
  tick_result = peer_tick(&host, 0);
  REQUIRE(send_packets_to_(tick_result, &alice));
  REQUIRE(send_packets_to_(tick_result, &bob));
  DA_DESTROY(tick_result.packets);

  /*  Check if Alice' data was updated.
   */
  REQUIRE(alice.queue.size == 1);
  REQUIRE(alice.queue.size == 1 && alice.queue.values[0].time == 0);
  REQUIRE(alice.queue.size == 1 &&
          alice.queue.values[0].actor == alice.actor);
  REQUIRE(alice.queue.size == 1 &&
          kit_ar_equal_bytes(1, 5, data, 1,
                             alice.queue.values[0].data.size,
                             alice.queue.values[0].data.values));

  /*  Check if Bob's data was updated.
   */
  REQUIRE(bob.queue.size == 1);
  REQUIRE(bob.queue.size == 1 && bob.queue.values[0].time == 0);
  REQUIRE(bob.queue.size == 1 &&
          bob.queue.values[0].actor == alice.actor);
  REQUIRE(bob.queue.size == 1 &&
          kit_ar_equal_bytes(1, 5, data, 1,
                             bob.queue.values[0].data.size,
                             bob.queue.values[0].data.values));

  /*  Destroy peers.
   */
  REQUIRE(peer_destroy(&host) == KIT_OK);
  REQUIRE(peer_destroy(&alice) == KIT_OK);
  REQUIRE(peer_destroy(&bob) == KIT_OK);
}

TEST("peer two clients state update") {
  /*  Initialize and connect three peers.
   */

  kit_allocator_t alloc = kit_alloc_default();
  peer_t          host, alice, bob;

  REQUIRE(peer_init(&host, PEER_HOST, alloc) == KIT_OK);
  REQUIRE(peer_init(&alice, PEER_CLIENT, alloc) == KIT_OK);
  REQUIRE(peer_init(&bob, PEER_CLIENT, alloc) == KIT_OK);

  ptrdiff_t const      sockets[]     = { 1, 2, 3, 4, 5, 6 };
  peer_ids_ref_t const host_sockets  = { .size   = 3,
                                         .values = sockets };
  peer_ids_ref_t const alice_sockets = { .size   = 1,
                                         .values = sockets + 3 };
  peer_ids_ref_t const bob_sockets   = { .size   = 1,
                                         .values = sockets + 4 };

  REQUIRE(peer_open(&host, host_sockets) == KIT_OK);
  REQUIRE(peer_open(&alice, alice_sockets) == KIT_OK);
  REQUIRE(peer_open(&bob, bob_sockets) == KIT_OK);

  if (host.slots.size == 3) {
    /*  Specify the address data for host.
     */
    host.slots.values[1].local.address_size    = 1;
    host.slots.values[1].local.address_data[0] = 2;
    host.slots.values[2].local.address_size    = 1;
    host.slots.values[2].local.address_data[0] = 3;
  }

  REQUIRE(peer_connect(&alice, host_sockets.values[0]) == KIT_OK);
  REQUIRE(peer_connect(&bob, host_sockets.values[0]) == KIT_OK);

  peer_tick_result_t tick_result;

  /*  Clients are trying to connect.
   */
  REQUIRE(send_packets_to_and_free_(peer_tick(&alice, 0), &host));
  REQUIRE(send_packets_to_and_free_(peer_tick(&bob, 0), &host));

  /*  Host will create a new session for clients.
   */
  tick_result = peer_tick(&host, 0);
  REQUIRE(send_packets_to_(tick_result, &alice));
  REQUIRE(send_packets_to_(tick_result, &bob));
  DA_DESTROY(tick_result.packets);

  /*  Resolve address ids.
   */
  REQUIRE(resolve_address_id_(&alice, &host));
  REQUIRE(resolve_address_id_(&bob, &host));

  /*  Clients are joining the session.
   */
  REQUIRE(send_packets_to_and_free_(peer_tick(&alice, 0), &host));
  REQUIRE(send_packets_to_and_free_(peer_tick(&bob, 0), &host));

  /*  Host is accepting clients.
   */
  tick_result = peer_tick(&host, 0);
  REQUIRE(send_packets_to_(tick_result, &alice));
  REQUIRE(send_packets_to_(tick_result, &bob));
  DA_DESTROY(tick_result.packets);

  /*  Check peers' actor ids.
   */
  REQUIRE(host.actor != alice.actor);
  REQUIRE(host.actor != bob.actor);
  REQUIRE(alice.actor != bob.actor);

  /*  Put data to Alice and Bob.
   */
  uint8_t          data[]     = { 1, 2, 3, 4, 5 };
  peer_chunk_ref_t data_ref_0 = { .size = 3, .values = data };
  peer_chunk_ref_t data_ref_1 = { .size = 2, .values = data + 3 };
  REQUIRE(peer_queue(&alice, data_ref_0) == KIT_OK);
  REQUIRE(peer_queue(&bob, data_ref_1) == KIT_OK);

  /*  Alice and Bob will send data to the host.
   *  Alice' message should be first.
   */
  REQUIRE(send_packets_to_and_free_(peer_tick(&alice, 0), &host));
  REQUIRE(send_packets_to_and_free_(peer_tick(&bob, 0), &host));

  /*  Host will update the data and send updates to clients.
   */
  tick_result = peer_tick(&host, 0);
  REQUIRE(send_packets_to_(tick_result, &alice));
  REQUIRE(send_packets_to_(tick_result, &bob));
  DA_DESTROY(tick_result.packets);

  /*  Check if Alice' data was updated.
   */
  REQUIRE(alice.queue.size == 2);
  REQUIRE(alice.queue.size == 2 && alice.queue.values[0].time == 0);
  REQUIRE(alice.queue.size == 2 &&
          alice.queue.values[0].actor == alice.actor);
  REQUIRE(alice.queue.size == 2 && alice.queue.values[1].time == 0);
  REQUIRE(alice.queue.size == 2 &&
          alice.queue.values[1].actor == bob.actor);
  REQUIRE(alice.queue.size == 2 &&
          kit_ar_equal_bytes(1, 3, data, 1,
                             alice.queue.values[0].data.size,
                             alice.queue.values[0].data.values));
  REQUIRE(alice.queue.size == 2 &&
          kit_ar_equal_bytes(1, 2, data + 3, 1,
                             alice.queue.values[1].data.size,
                             alice.queue.values[1].data.values));

  /*  Check if Bob's data was updated.
   */
  REQUIRE(bob.queue.size == 2);
  REQUIRE(bob.queue.size == 2 && bob.queue.values[0].time == 0);
  REQUIRE(bob.queue.size == 2 &&
          bob.queue.values[0].actor == alice.actor);
  REQUIRE(bob.queue.size == 2 && bob.queue.values[1].time == 0);
  REQUIRE(bob.queue.size == 2 &&
          bob.queue.values[1].actor == bob.actor);
  REQUIRE(bob.queue.size == 2 &&
          kit_ar_equal_bytes(1, 3, data, 1,
                             bob.queue.values[0].data.size,
                             bob.queue.values[0].data.values));
  REQUIRE(bob.queue.size == 2 &&
          kit_ar_equal_bytes(1, 2, data + 3, 1,
                             bob.queue.values[1].data.size,
                             bob.queue.values[1].data.values));

  /*  Destroy peers.
   */
  REQUIRE(peer_destroy(&host) == KIT_OK);
  REQUIRE(peer_destroy(&alice) == KIT_OK);
  REQUIRE(peer_destroy(&bob) == KIT_OK);
}
