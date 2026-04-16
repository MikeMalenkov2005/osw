#include "osw.h"

#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#endif

#ifdef _WIN32

typedef u_long in_addr_t;
typedef u_short in_port_t;

static void _SOCKET_OnExit(void)
{
  (void)WSACleanup();
}

static int _SOCKET_Init(void)
{
  WSADATA wsa;
  if (WSAStartup(MAKEWORD(2,2), &wsa))
  {
    atexit(_SOCKET_OnExit);
    return TRUE;
  }
  return FALSE;
}

#define close closesocket

static int _SOCKET_SetTimeout(SOCKET s, unsigned long millis) {
  return setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&millis, sizeof(DWORD));
}

static int _SOCKET_SetBlocking(SOCKET s, int blocking) {
  u_long mode = !blocking;
  return -(int)(ioctlsocket(s, FIONBIO, &mode) != 0);
}

#else

typedef int SOCKET;

#define INVALID_SOCKET  (-1)

#define _SOCKET_Init() 1

static int _SOCKET_SetTimeout(SOCKET s, unsigned long millis) {
  struct timeval tv;
  tv.tv_sec = millis / 1000;
  tv.tv_usec = millis % 1000;
  return setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

static int _SOCKET_SetBlocking(SOCKET s, int blocking) {
  int flags = fcntl(s, F_GETFL, 0);
  if (flags == -1) return -1;
  if (blocking) flags &= ~O_NONBLOCK;
  else flags |= O_NONBLOCK;
  return fcntl(s, F_SETFL, flags);
}

#endif

struct _OSW_NetSocket
{
  SOCKET sock;
  OSW_NetAddress addr;
  int type;
};

static OSW_NetAddress _OSW_NetDecode(const struct sockaddr *addr)
{
  OSW_NetAddress result = { OSW_NET_NONE };
  if (addr) switch (addr->sa_family)
  {
  case AF_INET:
    result.type = OSW_NET_IPv4;
    result.ipv4.port = ntohs(((struct sockaddr_in*)addr)->sin_port);
    memcpy(result.ipv4.addr,
        &((struct sockaddr_in*)addr)->sin_addr, sizeof(result.ipv4.addr));
    break;
  case AF_INET6:
    result.type = OSW_NET_IPv6;
    result.ipv6.port = ntohs(((struct sockaddr_in6*)addr)->sin6_port);
    result.ipv6.flow_info = ((struct sockaddr_in6*)addr)->sin6_flowinfo;
    result.ipv6.scope_id = ((struct sockaddr_in6*)addr)->sin6_scope_id;
    memcpy(result.ipv6.addr,
        &((struct sockaddr_in6*)addr)->sin6_addr, sizeof(result.ipv6.addr));
    break;
  }
  return result;
}

static socklen_t _OSW_NetEncode(
    struct sockaddr *out, const OSW_NetAddress *addr)
{
  if (out && addr) switch (addr->type)
  {
  case OSW_NET_IPv4:
    ((struct sockaddr_in*)out)->sin_family = AF_INET;
    ((struct sockaddr_in*)out)->sin_port = htons(addr->ipv4.port);
    memcpy(&((struct sockaddr_in*)out)->sin_addr,
        addr->ipv4.addr, sizeof(addr->ipv4.addr));
    return sizeof(struct sockaddr_in);
  case OSW_NET_IPv6:
    ((struct sockaddr_in6*)out)->sin6_family = AF_INET;
    ((struct sockaddr_in6*)out)->sin6_port = htons(addr->ipv6.port);
    ((struct sockaddr_in6*)out)->sin6_flowinfo = addr->ipv6.flow_info;
    ((struct sockaddr_in6*)out)->sin6_scope_id = addr->ipv6.scope_id;
    memcpy(&((struct sockaddr_in6*)out)->sin6_addr,
        addr->ipv6.addr, sizeof(addr->ipv6.addr));
    return sizeof(struct sockaddr_in6);
  }
  return 0;
}

OSW_NetAddress OSW_NetResolve(const char *host, const char *service, int index)
{
  struct addrinfo *root, *info;
  OSW_NetAddress result = { OSW_NET_NONE };
  if (_SOCKET_Init() && !getaddrinfo(host, service, NULL, &root))
  {
    for (info = root; info && index > 0; --index) info = info->ai_next;
    if (info) result = _OSW_NetDecode(info->ai_addr);
    freeaddrinfo(root);
  }
  return result;
}

OSW_NetSocket OSW_NetOpenDatagram(const OSW_NetAddress *addr)
{
  unsigned char buffer[128];
  OSW_NetSocket result = NULL;
  struct sockaddr *sa = (void*)buffer;
  socklen_t len = _OSW_NetEncode(sa, addr);
  if (len > 0 && _SOCKET_Init() && (result = OSW_malloc(sizeof(*result))))
  {
    result->sock = socket(sa->sa_family, SOCK_DGRAM, 0);
    if (result->sock != INVALID_SOCKET)
    {
      if (bind(result->sock, sa, len))
      {
        close(result->sock);
        result->sock = INVALID_SOCKET;
      }
      else
      {
        result->addr = *addr;
        result->type = OSW_NET_DATAGRAM;
      }
    }
    if (result->sock == INVALID_SOCKET)
    {
      OSW_free(result);
      result = NULL;
    }
  }
  return result;
}

OSW_NetSocket OSW_NetOpenServer(const OSW_NetAddress *addr, int backlog)
{
  unsigned char buffer[128];
  OSW_NetSocket result = NULL;
  struct sockaddr *sa = (void*)buffer;
  socklen_t len = _OSW_NetEncode(sa, addr);
  if (len > 0 && _SOCKET_Init() && (result = OSW_malloc(sizeof(*result))))
  {
    result->sock = socket(sa->sa_family, SOCK_STREAM, 0);
    if (result->sock != INVALID_SOCKET)
    {
      if (bind(result->sock, sa, len) || listen(result->sock, backlog))
      {
        close(result->sock);
        result->sock = INVALID_SOCKET;
      }
      else
      {
        result->addr = *addr;
        result->type = OSW_NET_SERVER;
      }
    }
    if (result->sock == INVALID_SOCKET)
    {
      OSW_free(result);
      result = NULL;
    }
  }
  return result;
}

OSW_NetSocket OSW_NetConnect(const OSW_NetAddress *addr)
{
  unsigned char buffer[128];
  OSW_NetSocket result = NULL;
  struct sockaddr *sa = (void*)buffer;
  socklen_t len = _OSW_NetEncode(sa, addr);
  if (len > 0 && _SOCKET_Init() && (result = OSW_malloc(sizeof(*result))))
  {
    result->sock = socket(sa->sa_family, SOCK_STREAM, 0);
    if (result->sock != INVALID_SOCKET)
    {
      if (connect(result->sock, sa, len))
      {
        close(result->sock);
        result->sock = INVALID_SOCKET;
      }
      else
      {
        result->addr = *addr;
        result->type = OSW_NET_STREAM;
      }
    }
    if (result->sock == INVALID_SOCKET)
    {
      OSW_free(result);
      result = NULL;
    }
  }
  return result;
}

OSW_NetSocket OSW_NetAccept(OSW_NetSocket server)
{
  unsigned char buffer[128];
  OSW_NetSocket result = NULL;
  struct sockaddr *sa = (void*)buffer;
  socklen_t len = sizeof(buffer);
  if (server && server->sock != INVALID_SOCKET
      && server->type == OSW_NET_SERVER && _SOCKET_Init()
      && (result = OSW_malloc(sizeof(*result))))
  {
    result->sock = accept(server->sock, sa, &len);
    if (result->sock != INVALID_SOCKET)
    {
      result->addr = _OSW_NetDecode(sa);
      if (result->addr.type == OSW_NET_NONE)
      {
        close(result->sock);
        result->sock = INVALID_SOCKET;
      }
      else result->type = OSW_NET_SERVER;
    }
    if (result->sock == INVALID_SOCKET)
    {
      OSW_free(result);
      result = NULL;
    }
  }
  return result;
}

void OSW_NetClose(OSW_NetSocket socket)
{
  if (socket && socket->sock != INVALID_SOCKET && _SOCKET_Init())
  {
    close(socket->sock);
    socket->sock = INVALID_SOCKET;
    OSW_free(socket);
  }
}

int OSW_NetGetSocketType(OSW_NetSocket socket)
{
  return socket && socket->sock != INVALID_SOCKET && _SOCKET_Init()
    ? socket->type : OSW_NET_INVALID;
}

OSW_NetAddress OSW_NetGetAddress(OSW_NetSocket socket)
{
  OSW_NetAddress none = { OSW_NET_NONE };
  return socket && socket->sock != INVALID_SOCKET && _SOCKET_Init()
    ? socket->addr : none;
}

int OSW_NetSend(OSW_NetSocket socket, const void *buffer, int size)
{
  return buffer && size > 0 && socket && socket->sock != INVALID_SOCKET
    && socket->type == OSW_NET_STREAM && _SOCKET_Init()
    ? send(socket->sock, buffer, size, 0) : -1;
}

int OSW_NetSendDatagram(OSW_NetSocket socket,
    const void *buffer, int size, const OSW_NetAddress *addr)
{
  unsigned char sa_buffer[128];
  struct sockaddr *sa = (void*)sa_buffer;
  socklen_t len = _OSW_NetEncode(sa, addr);
  return buffer && size > 0 && len > 0 && socket && _SOCKET_Init()
    && socket->sock != INVALID_SOCKET && socket->type == OSW_NET_DATAGRAM
? sendto(socket->sock, buffer, size, 0, sa, len) : -1;
}

int OSW_NetReceive(OSW_NetSocket socket, void *buffer, int size)
{
  return buffer && size > 0 && socket && socket->sock != INVALID_SOCKET
    && socket->type == OSW_NET_STREAM && _SOCKET_Init()
    ? recv(socket->sock, buffer, size, 0) : -1;
}

int OSW_NetReceiveDatagram(OSW_NetSocket socket,
    void *buffer, int size, OSW_NetAddress *addr)
{
  unsigned char sa_buffer[128];
  struct sockaddr *sa = (void*)sa_buffer;
  socklen_t len = sizeof(sa_buffer);
  int result = -1;
  if (buffer && size > 0 && socket && socket->sock != INVALID_SOCKET
      && socket->type == OSW_NET_DATAGRAM && _SOCKET_Init())
  {
    result = recvfrom(socket->sock, buffer, size, 0, sa, &len);
    if (addr && result > 0) *addr = _OSW_NetDecode(sa);
  }
  return result;
}

int OSW_NetSetTimeout(OSW_NetSocket socket, unsigned long millis)
{
  return socket && socket->sock != INVALID_SOCKET && _SOCKET_Init()
    ? _SOCKET_SetTimeout(socket->sock, millis) : -1;
}

int OSW_NetSetBlocking(OSW_NetSocket socket, int blocking)
{
  return socket && socket->sock != INVALID_SOCKET && _SOCKET_Init()
    ? _SOCKET_SetBlocking(socket->sock, blocking) : -1;
}

