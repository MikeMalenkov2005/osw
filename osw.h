#ifndef _OSW_H
#define _OSW_H

#include <stddef.h>

#ifdef _WIN32
#define OSW_OSAPI __stdcall
#else
#define OSW_OSAPI
#endif

#define _OSW_DEFINE_HANDLE(name) \
  typedef struct OSW_ ## name { int _; } *OSW_ ## name

/* DYNAMIC LINKING */

_OSW_DEFINE_HANDLE(Library);

typedef void OSW_OSAPI OSW_LibraryProc(void);

OSW_Library OSW_LoadLibrary(const char *path);
void OSW_FreeLibrary(OSW_Library lib);

OSW_LibraryProc *OSW_GetProcAddress(OSW_Library lib, const char *name);

/* FILE API */

_OSW_DEFINE_HANDLE(File);

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

_OSW_DEFINE_HANDLE(Thread);
typedef int OSW_OSAPI OSW_ThreadProc(void*);

OSW_Thread OSW_CreateThread(OSW_ThreadProc *proc, void *param);
int OSW_JoinThread(OSW_Thread thread, int *code);

void OSW_ExitProcess(int code);
void OSW_ExitThread(int code);

int OSW_SwitchThread(void);

#endif

