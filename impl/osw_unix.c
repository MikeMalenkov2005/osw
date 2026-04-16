#include "osw.h"

#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <pthread.h>
#include <sched.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>

/* DYNAMIC LINKING */

OSW_Library OSW_LoadLibrary(const char *path)
{
  return dlopen(path, RTLD_LAZY);
}

void OSW_FreeLibrary(OSW_Library lib)
{
  (void)dlclose(lib);
}

OSW_LibraryProc *OSW_GetProcAddress(OSW_Library lib, const char *name)
{
  return (OSW_LibraryProc*)dlsym(lib, name);
}

/* FILE API */

OSW_File OSW_OpenFile(const char *path, int flags)
{
  int f = 0;
  switch (flags & OSW_OPEN_READ_WRITE)
  {
  case OSW_OPEN_READ:
    f = O_RDONLY;
    break;
  case OSW_OPEN_WRITE:
    f = O_WRONLY;
    break;
  case OSW_OPEN_READ_WRITE:
    f = O_RDWR;
    break;
  }
  if (flags & OSW_OPEN_CREATE) f |= O_CREAT;
  if ((flags & OSW_OPEN_TRUNCATE) && (flags & OSW_OPEN_WRITE)) f |= O_TRUNC;
  return (OSW_File)(size_t)(open(path, f, 0666) + 1);
}

void OSW_CloseFile(OSW_File file)
{
  (void)close((int)(size_t)file - 1);
}

long long OSW_SeekFile(OSW_File file, long long offset, int whence)
{
#if defined(__linux__) && defined(_FILE_OFFSET_BITS) && _FILE_OFFSET_BITS < 64
  return lseek64((int)(size_t)file - 1, (off64_t)offset, whence);
#else
  return lseek((int)(size_t)file - 1, (off_t)offset, whence);
#endif
}

long long OSW_GetFileSize(OSW_File file)
{
  struct stat st;
  if (fstat((int)(size_t)file - 1, &st) == -1) return -1;
  return st.st_size;
}

size_t OSW_ReadFile(OSW_File file, void *buffer, size_t size)
{
  ssize_t result = 0;
  if (buffer && size && size <= SSIZE_MAX)
  {
    result = read((int)(size_t)file, buffer, size);
    if (result < 0) result = 0;
  }
  return result;
}

size_t OSW_WriteFile(OSW_File file, const void *buffer, size_t size)
{
  ssize_t result = 0;
  if (buffer && size && size <= SSIZE_MAX)
  {
    result = write((int)(size_t)file, buffer, size);
    if (result < 0) result = 0;
  }
  return result;
}

int OSW_RenameFile(const char *path, const char *new_path)
{
  return !rename(path, new_path);
}

int OSW_DeleteFile(const char *path)
{
  return !remove(path);
}

int OSW_IsFile(const char *path)
{
  struct stat path_stat;
  if (stat(path, &path_stat)) return 0;
  return S_ISREG(path_stat.st_mode);
}

int OSW_IsDirectory(const char *path)
{
  struct stat path_stat;
  if (stat(path, &path_stat)) return 0;
  return S_ISDIR(path_stat.st_mode);
}

int OSW_CreateDirectory(const char *path)
{
  return !mkdir(path, 0777);
}

int OSW_DeleteDirectory(const char *path)
{
  return !rmdir(path);
}

/* THREAD API */

struct _OSW_Thread
{
  pthread_t id;
  OSW_ThreadProc *proc;
  void *param;
};

static void *_OSW_ThreadStart(void *param)
{
  OSW_Thread thread = param;
  return (void*)(size_t)thread->proc(thread->param);
}

OSW_Thread OSW_CreateThread(OSW_ThreadProc *proc, void *param)
{
  OSW_Thread thread = NULL;
  if (proc && (thread = OSW_malloc(sizeof(*thread))))
  {
    thread->proc = proc;
    thread->param = param;
    if (pthread_create(&thread->id, NULL, _OSW_ThreadStart, thread))
    {
      OSW_free(thread);
      thread = NULL;
    }
  }
  return thread;
}

int OSW_JoinThread(OSW_Thread thread, int *code)
{
  void *value;
  if (!thread || !thread->proc || pthread_join(thread->id, &value)) return 0;
  if (code) *code = (int)(size_t)value;
  memset(thread, 0, sizeof(*thread));
  OSW_free(thread);
  return 1;
}

void OSW_ExitProcess(int code)
{
  _exit(code);
}

void OSW_ExitThread(int code)
{
  pthread_exit((void*)(size_t)code);
}

int OSW_SwitchThread(void)
{
  return !sched_yield();
}

OSW_Mutex OSW_CreateMutex(void)
{
  pthread_mutex_t *mutex = OSW_malloc(sizeof(*mutex));
  if (mutex && pthread_mutex_init(mutex, NULL))
  {
    OSW_free(mutex);
    mutex = NULL;
  }
  return (void*)mutex;
}

int OSW_DeleteMutex(OSW_Mutex mutex)
{
  if (pthread_mutex_destroy((void*)mutex))
  {
    OSW_free(mutex);
    return 1;
  }
  return 0;
}

int OSW_LockMutex(OSW_Mutex mutex)
{
  return !pthread_mutex_lock((void*)mutex);
}

int OSW_UnlockMutex(OSW_Mutex mutex)
{
  return !pthread_mutex_unlock((void*)mutex);
}

int OSW_TryLockMutex(OSW_Mutex mutex)
{
  return !pthread_mutex_trylock((void*)mutex);
}

OSW_TLS OSW_CreateTLS(void)
{
  pthread_key_t key;
  if (pthread_key_create(&key, NULL)) return NULL;
  return (OSW_TLS)((size_t)key + 1);
}

int OSW_DeleteTLS(OSW_TLS tls)
{
  return !pthread_key_delete((pthread_key_t)((size_t)tls - 1));
}

void *OSW_GetTLS(OSW_TLS tls)
{
  return pthread_getspecific((pthread_key_t)((size_t)tls - 1));
}

int OSW_SetTLS(OSW_TLS tls, void *value)
{
  return !pthread_setspecific((pthread_key_t)((size_t)tls - 1), value);
}

