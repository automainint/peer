#include "../../peer/socket_pool.h"

#define KIT_TEST_FILE socket_pool
#include <kit_test/test.h>

#ifndef PEER_DISABLE_SYSTEM_SOCKETS
TEST("socket pool UDP IPv4") {
  peer_socket_pool_t pool;
  REQUIRE_EQ(peer_pool_init(&pool, kit_alloc_default()), KIT_OK);

  peer_t host, client;
  REQUIRE_EQ(peer_init(&host, PEER_HOST, kit_alloc_default()),
             KIT_OK);
  REQUIRE_EQ(peer_init(&client, PEER_CLIENT, kit_alloc_default()),
             KIT_OK);

  REQUIRE_EQ(
      peer_pool_open(&pool, &host, PEER_UDP_IPv4, PEER_ANY_PORT, 2),
      KIT_OK);

  REQUIRE(pool.nodes.size == 2 &&
          peer_pool_connect(
              &pool, &client, PEER_UDP_IPv4, SZ("127.0.0.1"),
              pool.nodes.values[0].local_port) == KIT_OK);

  REQUIRE_EQ(peer_pool_tick(&pool, &client, 0), KIT_OK);
  REQUIRE_EQ(peer_pool_tick(&pool, &host, 0), KIT_OK);
  REQUIRE_EQ(peer_pool_tick(&pool, &client, 0), KIT_OK);
  REQUIRE_EQ(peer_pool_tick(&pool, &host, 0), KIT_OK);
  REQUIRE_EQ(peer_pool_tick(&pool, &client, 0), KIT_OK);
  REQUIRE_EQ(peer_pool_tick(&pool, &host, 0), KIT_OK);
  REQUIRE_EQ(peer_pool_tick(&pool, &client, 0), KIT_OK);

  uint8_t          data[]      = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  peer_chunk_ref_t data_ref[3] = { { .size = 2, .values = data },
                                   { .size = 4, .values = data + 2 },
                                   { .size   = 3,
                                     .values = data + 6 } };

  REQUIRE_EQ(peer_queue(&host, data_ref[0]), KIT_OK);
  REQUIRE_EQ(peer_queue(&host, data_ref[1]), KIT_OK);
  REQUIRE_EQ(peer_queue(&host, data_ref[2]), KIT_OK);

  REQUIRE_EQ(peer_pool_tick(&pool, &host, 12), KIT_OK);
  REQUIRE_EQ(peer_pool_tick(&pool, &client, 12), KIT_OK);
  REQUIRE_EQ(peer_pool_tick(&pool, &host, 0), KIT_OK);
  REQUIRE_EQ(peer_pool_tick(&pool, &client, 0), KIT_OK);

  REQUIRE_EQ(client.queue.size, 3);
  REQUIRE(client.queue.size == 3 &&
          client.queue.values[0].time == 12);
  REQUIRE(client.queue.size == 3 &&
          client.queue.values[1].time == 12);
  REQUIRE(client.queue.size == 3 &&
          client.queue.values[2].time == 12);

  REQUIRE_EQ(client.time, 12);

  REQUIRE_EQ(peer_destroy(&host), KIT_OK);
  REQUIRE_EQ(peer_destroy(&client), KIT_OK);
  REQUIRE_EQ(peer_pool_destroy(&pool), KIT_OK);
}
#endif
