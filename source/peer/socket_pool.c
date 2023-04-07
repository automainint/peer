#include "socket_pool.h"

#include <assert.h>

#include <stdio.h>

#ifndef PEER_DISABLE_SYSTEM_SOCKETS
static kit_status_t find_pool_node(
    peer_socket_pool_t *const pool, int const protocol,
    uint16_t const remote_port, ptrdiff_t const remote_address_size,
    char const *const remote_address, ptrdiff_t *const out_id) {
  assert(pool != NULL);
  assert(protocol == PEER_UDP_IPv4);
  assert(remote_address_size > 0);

  if (protocol != PEER_UDP_IPv4)
    return PEER_ERROR_UNKNOWN_PROTOCOL;

  *out_id = -1;

  for (ptrdiff_t i = 0; i < pool->nodes.size; i++) {
    peer_node_t const *const node = pool->nodes.values + i;

    if (node->socket == INVALID_SOCKET &&
        node->protocol == protocol &&
        node->local_port == PEER_ANY_PORT &&
        node->remote_port == remote_port &&
        AR_EQUAL(kit_str(node->remote_address_size,
                         (char const *) node->remote_address),
                 kit_str(remote_address_size, remote_address))) {
      *out_id = i;
      return KIT_OK;
    }
  }

  ptrdiff_t const n = pool->nodes.size;

  DA_RESIZE(pool->nodes, n + 1);
  assert(pool->nodes.size == n + 1);
  if (pool->nodes.size != n + 1)
    return PEER_ERROR_BAD_ALLOC;

  peer_node_t *const node = pool->nodes.values + n;
  memset(node, 0, sizeof *node);

  node->socket              = INVALID_SOCKET;
  node->protocol            = protocol;
  node->local_port          = PEER_ANY_PORT;
  node->remote_port         = remote_port;
  node->remote_address_size = remote_address_size;

  memcpy(node->remote_address, remote_address, remote_address_size);

  *out_id = n;
  return KIT_OK;
}

static kit_status_t resolve_address_and_id(
    peer_socket_pool_t *const pool, peer_t *const peer) {
  printf("\n");
  printf("  pool size: %lld  \n", (long long) pool->nodes.size);
  for (ptrdiff_t i = 0; i < peer->slots.size; i++)
    printf("  slot %2lld: %2lld - %2lld\n", (long long) i,
           (long long) peer->slots.values[i].local.id,
           (long long) peer->slots.values[i].remote.id);
  fflush(stdout);

  kit_status_t status = KIT_OK;

  for (ptrdiff_t i = 0; i < peer->slots.size; i++) {
    peer_slot_t *const slot = peer->slots.values + i;

    if (!slot->local.is_address_resolved &&
        slot->local.id != PEER_UNDEFINED) {
      /*  Get local port by id.
       */

      assert(slot->local.id >= 0);
      assert(slot->local.id < pool->nodes.size);

      if (slot->local.id < 0 || slot->local.id >= pool->nodes.size) {
        status |= PEER_ERROR_INVALID_ID;
      } else {
        peer_node_t const *const node = pool->nodes.values +
                                        slot->local.id;

        slot->local.address_size    = 3;
        slot->local.address_data[0] = (uint8_t) node->protocol;
        slot->local.address_data[1] = (uint8_t) (node->local_port &
                                                 0xff);
        slot->local.address_data[2] =
            (uint8_t) ((node->local_port >> 8) & 0xff);

        slot->local.is_address_resolved = 1;
      }
    }

    if (!slot->remote.is_address_resolved &&
        slot->remote.id != PEER_UNDEFINED) {
      /*  Get remote address and port by id.
       */

      assert(slot->remote.id >= 0);
      assert(slot->remote.id < pool->nodes.size);

      if (slot->remote.id < 0 ||
          slot->remote.id >= pool->nodes.size) {
        status |= PEER_ERROR_INVALID_ID;
      } else {
        peer_node_t const *const node = pool->nodes.values +
                                        slot->remote.id;

        slot->remote.address_size    = 3 + node->remote_address_size;
        slot->remote.address_data[0] = (uint8_t) node->protocol;
        slot->remote.address_data[1] = (uint8_t) (node->remote_port &
                                                  0xff);
        slot->remote.address_data[2] =
            (uint8_t) ((node->remote_port >> 8) & 0xff);

        if (node->remote_address_size > 0)
          memcpy(slot->remote.address_data + 3, node->remote_address,
                 node->remote_address_size);

        slot->remote.is_address_resolved = 1;
      }
    }

    if (!slot->remote.is_id_resolved &&
        slot->remote.address_size == 3 &&
        slot->remote.id != PEER_UNDEFINED) {
      /*  Get remote id by port and previous id.
       */

      assert(slot->remote.id >= 0);
      assert(slot->remote.id < pool->nodes.size);

      if (slot->remote.id < 0 ||
          slot->remote.id >= pool->nodes.size) {
        status |= PEER_ERROR_INVALID_ID;
      } else {
        peer_node_t const *const node = pool->nodes.values +
                                        slot->remote.id;

        int const      protocol = slot->remote.address_data[0];
        uint16_t const port =
            (uint16_t) (slot->remote.address_data[1] |
                        (slot->remote.address_data[2] << 8));

        ptrdiff_t          id;
        kit_status_t const s = find_pool_node(
            pool, protocol, port, node->remote_address_size,
            (char const *) node->remote_address, &id);

        if (s != KIT_OK)
          status |= s;
        else {
          slot->remote.id             = id;
          slot->remote.is_id_resolved = 1;
        }
      }
    }

    if (!slot->remote.is_id_resolved &&
        slot->remote.address_size > 3) {
      /*  Get remote id by address and port.
       */

      int const      protocol = slot->remote.address_data[0];
      uint16_t const port = (uint16_t) (slot->remote.address_data[1] |
                                        (slot->remote.address_data[2]
                                         << 8));

      ptrdiff_t          id;
      kit_status_t const s = find_pool_node(
          pool, protocol, port, slot->remote.address_size - 3,
          (char const *) slot->remote.address_data + 3, &id);

      if (s != KIT_OK)
        status |= s;
      else {
        slot->remote.id             = id;
        slot->remote.is_id_resolved = 1;
      }
    }
  }

  return status;
}

kit_status_t peer_pool_init(peer_socket_pool_t *const pool,
                            kit_allocator_t const     alloc) {
  assert(pool != NULL);

  if (pool == NULL)
    return PEER_ERROR_INVALID_POOL;

  pool->alloc = alloc;
  DA_INIT(pool->nodes, 0, alloc);

  return KIT_OK;
}

kit_status_t peer_pool_destroy(peer_socket_pool_t *const pool) {
  assert(pool != NULL);

  if (pool == NULL)
    return PEER_ERROR_INVALID_POOL;

  for (ptrdiff_t i = 0; i < pool->nodes.size; i++)
    closesocket(pool->nodes.values[i].socket);

  DA_DESTROY(pool->nodes);

  return KIT_OK;
}

kit_status_t peer_pool_open(peer_socket_pool_t *const pool,
                            peer_t *const peer, int const protocol,
                            uint16_t const  port,
                            ptrdiff_t const count) {
  assert(pool != NULL);
  assert(peer != NULL);
  assert(count > 0);

  printf("\n  init pool %lld  \n", (long long) count);
  fflush(stdout);

  if (pool == NULL)
    return PEER_ERROR_INVALID_POOL;
  if (peer == NULL)
    return PEER_ERROR_INVALID_PEER;
  if (count <= 0)
    return PEER_ERROR_INVALID_COUNT;

  kit_status_t    status = KIT_OK;
  ptrdiff_t const n      = pool->nodes.size;

  switch (protocol) {
    case PEER_UDP_IPv4:
      DA_RESIZE(pool->nodes, n + count);
      assert(pool->nodes.size == n + count);

      if (pool->nodes.size != n + count) {
        DA_RESIZE(pool->nodes, n);
        status |= PEER_ERROR_BAD_ALLOC;
        break;
      }

      memset(pool->nodes.values + n, 0,
             count * sizeof *pool->nodes.values);

      for (ptrdiff_t i = 0; i < count; i++) {
        peer_node_t *const node = pool->nodes.values + (n + i);

        node->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (node->socket == INVALID_SOCKET) {
          status |= PEER_ERROR_CREATE_SOCKET_FAILED;
          break;
        }

        if (peer_socket_set_nonblocking(node->socket) != 0) {
          status |= PEER_ERROR_MAKE_SOCKET_NONBLOCKING_FAILED;
          break;
        }

        struct sockaddr_in name;
        memset(&name, 0, sizeof name);

        name.sin_family      = AF_INET;
        name.sin_port        = htons(port);
        name.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(node->socket, (struct sockaddr const *) &name,
                 sizeof name) == -1) {
          assert(errno != EADDRINUSE);
          status |= PEER_ERROR_BIND_SOCKET_FAILED;
          break;
        }

        socklen_t len = sizeof name;

        if (getsockname(node->socket, (struct sockaddr *) &name,
                        &len) == -1) {
          status |= PEER_ERROR_GET_SOCKET_NAME_FAILED;
          break;
        }

        node->protocol    = PEER_UDP_IPv4;
        node->local_port  = ntohs(name.sin_port);
        node->remote_port = PEER_ANY_PORT;

        printf("  socket %d done  \n", (int) i);
        fflush(stdout);
      }

      break;

    case PEER_UDP_IPv6:
    case PEER_TCP_IPv4:
    case PEER_TCP_IPv6: status |= PEER_ERROR_NOT_IMPLEMENTED; break;
    default: status |= PEER_ERROR_UNKNOWN_PROTOCOL;
  }

  DA(ptrdiff_t) ids;
  DA_INIT(ids, count, pool->alloc);
  assert(ids.size == count);

  if (ids.size == count) {
    for (ptrdiff_t i = 0; i < count; i++) ids.values[i] = n + i;
    peer_ids_ref_t const ref = { .size   = ids.size,
                                 .values = ids.values };
    printf("  open  \n");
    fflush(stdout);

    status |= peer_open(peer, ref);
    DA_DESTROY(ids);
  } else {
    status |= PEER_ERROR_BAD_ALLOC;
  }

  if (status != KIT_OK) {
    printf("  revert  \n");
    fflush(stdout);

    for (ptrdiff_t i = n; i < pool->nodes.size; i++)
      if (pool->nodes.values[i].socket != INVALID_SOCKET)
        closesocket(pool->nodes.values[i].socket);
    DA_RESIZE(pool->nodes, n);
  }

  return status;
}

kit_status_t peer_pool_connect(peer_socket_pool_t *const pool,
                               peer_t *const peer, int const protocol,
                               kit_str_t const address,
                               uint16_t const  port) {
  assert(pool != NULL);
  assert(peer != NULL);
  assert(address.size > 0);
  assert(address.size <= PEER_ADDRESS_SIZE - 2);
  assert(port != PEER_ANY_PORT);
  assert(protocol == PEER_UDP_IPv4);

  if (pool == NULL)
    return PEER_ERROR_INVALID_POOL;
  if (peer == NULL)
    return PEER_ERROR_INVALID_PEER;
  if (address.size <= 0 || address.size > PEER_ADDRESS_SIZE - 2)
    return PEER_ERROR_INVALID_ADDRESS;
  if (port == PEER_ANY_PORT)
    return PEER_ERROR_INVALID_PORT;

  ptrdiff_t    remote_id;
  kit_status_t s = find_pool_node(pool, protocol, port, address.size,
                                  address.values, &remote_id);
  if (s != KIT_OK)
    return s;

  s = peer_connect(peer, remote_id);
  if (s != PEER_ERROR_NO_FREE_SLOTS)
    return s;

  s = peer_pool_open(pool, peer, protocol, PEER_ANY_PORT, 1);
  if (s != KIT_OK)
    return s;

  return peer_connect(peer, remote_id);
}

kit_status_t peer_pool_tick(peer_socket_pool_t *const pool,
                            peer_t *const             peer,
                            peer_time_t const         time_elapsed) {
  assert(pool != NULL);
  assert(peer != NULL);
  assert(time_elapsed >= 0);

  if (pool == NULL)
    return PEER_ERROR_INVALID_POOL;
  if (peer == NULL)
    return PEER_ERROR_INVALID_PEER;
  if (time_elapsed < 0)
    return PEER_ERROR_INVALID_TIME_ELAPSED;

  kit_status_t status = KIT_OK;

  status |= resolve_address_and_id(pool, peer);

  peer_packets_t packets;
  DA_INIT(packets, 0, pool->alloc);

  for (ptrdiff_t i = 0; i < pool->nodes.size; i++) {
    peer_node_t *const node = pool->nodes.values + i;

    if (node->socket == INVALID_SOCKET)
      continue;
    if (node->protocol != PEER_UDP_IPv4)
      continue;

    struct sockaddr_in remote;
    memset(&remote, 0, sizeof remote);

    socklen_t len = sizeof remote;

    uint8_t buf[PEER_PACKET_SIZE];

    ptrdiff_t const size = recvfrom(
        node->socket, buf, PEER_PACKET_SIZE, 0,
        (struct sockaddr *) &remote, &len);

    if (size == -1 && errno != EAGAIN) {
      assert(errno != EMSGSIZE);
      assert(errno != ECONNRESET);
      status |= PEER_ERROR_SOCKET_RECEIVE_FAILED;
    }

    if (size <= 0)
      continue;

    char sbuf[PEER_ADDRESS_SIZE - 1];
    inet_ntop(AF_INET,
              &((struct sockaddr_in const *) &remote)->sin_addr, sbuf,
              sizeof sbuf);

    uint16_t const remote_port = ntohs(remote.sin_port);

    ptrdiff_t          id;
    kit_status_t const s = find_pool_node(
        pool, node->protocol, remote_port, strlen(sbuf), sbuf, &id);

    if (s != KIT_OK) {
      status |= s;
      continue;
    }

    ptrdiff_t const n = packets.size;

    DA_RESIZE(packets, n + 1);
    assert(packets.size == n + 1);
    if (packets.size != n + 1) {
      status |= PEER_ERROR_BAD_ALLOC;
      continue;
    }

    memset(packets.values + n, 0, sizeof *packets.values);

    packets.values[n].source_id      = id;
    packets.values[n].destination_id = i;
    packets.values[n].size           = size;

    memcpy(packets.values[n].data, buf, size);
  }

  peer_packets_ref_t const ref = { .size   = packets.size,
                                   .values = packets.values };

  status |= peer_input(peer, ref);

  DA_DESTROY(packets);

  peer_tick_result_t const tick = peer_tick(peer, time_elapsed);

  for (ptrdiff_t i = 0; i < tick.packets.size; i++) {
    peer_packet_t const *packet = tick.packets.values + i;

    assert(packet->source_id >= 0);
    assert(packet->source_id < pool->nodes.size);
    assert(packet->destination_id >= 0);
    assert(packet->destination_id < pool->nodes.size);

    if (packet->source_id < 0 ||
        packet->source_id >= pool->nodes.size ||
        packet->destination_id < 0 ||
        packet->destination_id >= pool->nodes.size) {
      status |= PEER_ERROR_INVALID_ID;
      continue;
    }

    peer_node_t const *const src = pool->nodes.values +
                                   packet->source_id;
    peer_node_t const *const dst = pool->nodes.values +
                                   packet->destination_id;

    assert(src->socket != INVALID_SOCKET);
    if (src->socket == INVALID_SOCKET) {
      status |= PEER_ERROR_INVALID_SOCKET;
      continue;
    }

    assert(src->protocol == dst->protocol);
    assert(src->protocol == PEER_UDP_IPv4);

    if (src->protocol != dst->protocol ||
        src->protocol != PEER_UDP_IPv4) {
      status |= PEER_ERROR_UNKNOWN_PROTOCOL;
      continue;
    }

    struct sockaddr_in name;
    memset(&name, 0, sizeof name);

    name.sin_family = AF_INET;
    name.sin_port   = htons(dst->remote_port);

    if (inet_pton(AF_INET, (char const *) dst->remote_address,
                  &name.sin_addr.s_addr) == -1) {
      status |= PEER_ERROR_GET_SOCKET_NAME_FAILED;
      continue;
    }

    ptrdiff_t const n = sendto(
        src->socket, packet->data, packet->size, 0,
        (struct sockaddr const *) &name, sizeof name);

    if (n == -1) {
      assert(errno != EMSGSIZE);
      assert(errno != ECONNRESET);
      assert(errno != EWOULDBLOCK);
      status |= PEER_ERROR_SOCKET_SEND_FAILED;
    }
  }

  DA_DESTROY(tick.packets);

  status |= tick.status;

  return status;
}
#endif
