#ifndef _OSW_H
#define _OSW_H

#include <stddef.h>

#ifdef __cplusplus
#define _OSW_C_BEGIN  extern "C" {
#define _OSW_C_END    }
#else
#define _OSW_C_BEGIN
#define _OSW_C_END
#endif

_OSW_C_BEGIN

#ifdef _WIN32
#define OSW_OSAPI __stdcall
#else
#define OSW_OSAPI
#endif

/* DYNAMIC LINKING */

typedef struct _OSW_Library *OSW_Library;

typedef void OSW_OSAPI OSW_LibraryProc(void);

OSW_Library OSW_LoadLibrary(const char *path);
void OSW_FreeLibrary(OSW_Library lib);

OSW_LibraryProc *OSW_GetProcAddress(OSW_Library lib, const char *name);

/* FILE API */

typedef struct _OSW_File *OSW_File;

#define OSW_OPEN_READ       1
#define OSW_OPEN_WRITE      2
#define OSW_OPEN_READ_WRITE 3
#define OSW_OPEN_CREATE     4
#define OSW_OPEN_TRUNCATE   8

#define OSW_SEEK_SET  0
#define OSW_SEEK_CUR  1
#define OSW_SEEK_END  2

OSW_File OSW_OpenFile(const char *path, int flags);
void OSW_CloseFile(OSW_File file);

long long OSW_SeekFile(OSW_File file, long long offset, int whence);
long long OSW_GetFileSize(OSW_File file);

size_t OSW_ReadFile(void *buffer, size_t size, OSW_File file);
size_t OSW_WriteFile(const void *buffer, size_t size, OSW_File file);

int OSW_RenameFile(const char *path, const char *new_path);
int OSW_DeleteFile(const char *path);

int OSW_IsFile(const char *path);
int OSW_IsDirectory(const char *path);

int OSW_CreateDirectory(const char *path);
int OSW_DeleteDirectory(const char *path);

/* THREAD API */

typedef struct _OSW_Thread *OSW_Thread;
typedef int OSW_OSAPI OSW_ThreadProc(void*);

OSW_Thread OSW_CreateThread(OSW_ThreadProc *proc, void *param);
int OSW_JoinThread(OSW_Thread thread, int *code);

void OSW_ExitProcess(int code);
void OSW_ExitThread(int code);

int OSW_SwitchThread(void);

typedef struct _OSW_Mutex *OSW_Mutex;

OSW_Mutex OSW_CreateMutex(void);
int OSW_DeleteMutex(OSW_Mutex mutex);

int OSW_LockMutex(OSW_Mutex mutex);
int OSW_UnlockMutex(OSW_Mutex mutex);
int OSW_TryLockMutex(OSW_Mutex mutex);

typedef struct _OSW_TLS *OSW_TLS;

OSW_TLS OSW_CreateTLS(void);
int OSW_DeleteTLS(OSW_TLS tls);

void *OSW_GetTLS(OSW_TLS tls);
int OSW_SetTLS(OSW_TLS tls, void *value);

/* NETWORKING API */

#ifdef OSW_NET

#define OSW_NET_NONE  0
#define OSW_NET_IPv4  1
#define OSW_NET_IPv6  2

typedef struct _OSW_NetAddressIPv4
{
  unsigned short type;
  unsigned short port;
  unsigned char addr[4];
} OSW_NetAddressIPv4;

typedef struct _OSW_NetAddressIPv6
{
  unsigned short type;
  unsigned short port;
  unsigned int flow_info;
  unsigned char addr[16];
  unsigned int scope_id;
} OSW_NetAddressIPv6;

typedef union _OSW_NetAddress
{
  unsigned short type;
  OSW_NetAddressIPv4 ipv4;
  OSW_NetAddressIPv6 ipv6;
} OSW_NetAddress;

OSW_NetAddress OSW_NetResolve(const char *host, const char *service, int index);

#define OSW_NET_INVALID   0
#define OSW_NET_DATAGRAM  1
#define OSW_NET_SERVER    2
#define OSW_NET_STREAM    3

typedef struct _OSW_NetSocket *OSW_NetSocket;

OSW_NetSocket OSW_NetOpenDatagram(const OSW_NetAddress *addr);
OSW_NetSocket OSW_NetOpenServer(const OSW_NetAddress *addr, int backlog);

OSW_NetSocket OSW_NetConnect(const OSW_NetAddress *addr);
OSW_NetSocket OSW_NetAccept(OSW_NetSocket server);

void OSW_NetClose(OSW_NetSocket socket);

int OSW_NetGetSocketType(OSW_NetSocket socket);
OSW_NetAddress OSW_NetGetAddress(OSW_NetSocket socket);

int OSW_NetSend(const void *buffer, int size, OSW_NetSocket socket);
int OSW_NetSendDatagram(const void *buffer,
    int size, OSW_NetSocket socket, const OSW_NetAddress *addr);

int OSW_NetReceive(void *buffer, int size, OSW_NetSocket socket);
int OSW_NetReceiveDatagram(void *buffer,
    int size, OSW_NetSocket socket, OSW_NetAddress *addr);

int OSW_NetSetTimeout(OSW_NetSocket socket, unsigned long millis);
int OSW_NetSetBlocking(OSW_NetSocket socket, int blocking);

#endif

_OSW_C_END

#endif

