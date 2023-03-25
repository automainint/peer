#ifndef PEER_SOCKETS_H
#define PEER_SOCKETS_H

#ifndef PEER_DISABLE_SYSTEM_SOCKETS
#  ifdef __GNUC__
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wunused-function"
#    pragma GCC diagnostic ignored "-Wunknown-pragmas"
#  endif

#  if defined(_WIN32) && !defined(__CYGWIN__)

#    define WIN32_LEAN_AND_MEAN
#    include <winsock2.h>
#    include <ws2tcpip.h>

#    define socket_t SOCKET
#    define socklen_t int

//#    define EINPROGRESS WSAEINPROGRESS
//#    define EWOULDBLOCK WSAEWOULDBLOCK
//#    define EMSGSIZE WSAEMSGSIZE
//#    define EISCONN WSAEISCONN
//#    define ECONNRESET WSAECONNRESET
//#    define EADDRINUSE WSAEADDRINUSE

//#    define errno ((int) WSAGetLastError())

#    ifdef __cplusplus
extern "C" {
#    endif

static int peer_sockets_init() {
  WSADATA data;
  memset(&data, 0, sizeof data);
  WORD version = MAKEWORD(2, 2);
  if (WSAStartup(version, &data) != ERROR_SUCCESS)
    return -1;
  return 0;
}

static int peer_sockets_cleanup() {
  WSACleanup();
  return 0;
}

static int peer_socket_set_blocking(socket_t s) {
  u_long flag = 0;
  if (ioctlsocket(s, FIONBIO, &flag) == -1)
    return -1;
  return 0;
}

static int peer_socket_set_nonblocking(socket_t s) {
  u_long flag = 1;
  if (ioctlsocket(s, FIONBIO, &flag) == -1)
    return -1;
  return 0;
}

#    ifdef __cplusplus
}
#    endif

#  else

#    include <arpa/inet.h>
#    include <errno.h>
#    include <fcntl.h>
#    include <netinet/in.h>
#    include <signal.h>
#    include <sys/ioctl.h>
#    include <sys/select.h>
#    include <sys/socket.h>
#    include <sys/types.h>
#    include <unistd.h>

#    define socket_t int

#    define closesocket close

#    ifdef __cplusplus
extern "C" {
#    endif

static int peer_sockets_init() {
  signal(SIGPIPE, SIG_IGN);
  return 0;
}

static int peer_sockets_cleanup() {
  return 0;
}

static int peer_socket_set_blocking(socket_t s) {
  int const flags = fcntl(s, F_GETFL, 0);
  return fcntl(s, F_SETFL, flags & ~O_NONBLOCK);
}

static int peer_socket_set_nonblocking(socket_t s) {
  int const flags = fcntl(s, F_GETFL, 0);
  return fcntl(s, F_SETFL, flags | O_NONBLOCK);
}

#    ifdef __cplusplus
}
#    endif

#  endif

#  ifdef __GNUC__
#    pragma GCC diagnostic pop
#  endif
#endif

#endif
