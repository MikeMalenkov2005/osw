#include "osw.h"

#include <Windows.h>

/* WINDOWS UNICODE */

static LPWSTR _OSW_DecodeUTF8(const char *utf8)
{
  LPWSTR result = NULL;
  UINT len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
  if (len && (result = OSW_malloc(sizeof(*result) * len)))
  {
    (void)MultiByteToWideChar(CP_UTF8, 0, utf8, -1, result, len);
  }
  return result;
}

/* DYNAMIC LINKING */

OSW_Library OSW_LoadLibrary(const char *path)
{
  OSW_Library lib = NULL;
  LPWSTR wpath = _OSW_DecodeUTF8(path);
  if (wpath)
  {
    lib = (OSW_Library)LoadLibraryW(wpath);
    OSW_free(wpath);
  }
  return lib;
}

void OSW_FreeLibrary(OSW_Library lib)
{
  (void)FreeLibrary((HMODULE)lib);
}

OSW_LibraryProc *OSW_GetProcAddress(OSW_Library lib, const char *name)
{
  return (OSW_LibraryProc*)GetProcAddress((HMODULE)lib, name);
}

/* FILE API */

OSW_File osw_file_open(const char *path, int flags)
{
  HANDLE handle = INVALID_HANDLE_VALUE;
  LPWSTR wpath = _OSW_DecodeUTF8(path);
  DWORD f1 = 0, f2 = OPEN_EXISTING;
  if (wpath)
  {
    if (flags & OSW_OPEN_READ) f1 |= GENERIC_READ;
    if (flags & OSW_OPEN_WRITE) f1 |= GENERIC_WRITE;
    if (flags & OSW_OPEN_CREATE)
    {
      f2 = OPEN_ALWAYS;
      if ((flags & OSW_OPEN_TRUNCATE) && (flags & OSW_OPEN_WRITE))
      {
        f2 = CREATE_ALWAYS;
      }
    }
    else if ((flags & OSW_OPEN_TRUNCATE) && (flags & OSW_OPEN_WRITE))
    {
      f2 = TRUNCATE_EXISTING;
    }
    handle = CreateFileW(wpath, f1, 0, NULL, f2, FILE_ATTRIBUTE_NORMAL, NULL);
    OSW_free(wpath);
  }
  return (OSW_File)((ptrdiff_t)handle + 1);
}

void OSW_CloseFile(OSW_File file)
{
  (void)CloseHandle((HANDLE)((ptrdiff_t)file - 1));
}

long long OSW_SeekFile(OSW_File file, long long offset, int whence)
{
  LARGE_INTEGER value = { 0 };
  HANDLE handle = (HANDLE)((ptrdiff_t)file - 1);
  value.QuadPart = offset;
  if (!file || !SetFilePointerEx(handle, value, &value, whence)) return -1;
  return value.QuadPart;
}

long long OSW_GetFileSize(OSW_File file)
{
  LARGE_INTEGER value;
  HANDLE handle = (HANDLE)((ptrdiff_t)file - 1);
  if (!file || !GetFileSizeEx(handle, &value)) return -1;
  return value.QuadPart;
}

size_t OSW_ReadFile(void *buffer, size_t size, OSW_File file)
{
  size_t remain, bytes = 0;
  DWORD toread = 0, count = 0;
  HANDLE handle = (HANDLE)((ptrdiff_t)file - 1);
  if (file && buffer) while (bytes < size && count >= toread)
  {
    count = 0;
    remain = size - bytes;
    toread = remain > ~(DWORD)0 ? ~(DWORD)0 : remain;
    if (!ReadFile(handle, (char*)buffer + bytes, toread, &count, NULL)) break;
    bytes += count;
  }
  return bytes;
}

size_t OSW_WriteFile(const void *buffer, size_t size, OSW_File file)
{
  size_t remain, bytes = 0;
  DWORD towrite = 0, count = 0;
  HANDLE handle = (HANDLE)((ptrdiff_t)file - 1);
  if (file && buffer) while (bytes < size && count >= towrite)
  {
    count = 0;
    remain = size - bytes;
    towrite = remain > ~(DWORD)0 ? ~(DWORD)0 : remain;
    if (!WriteFile(handle, (char*)buffer + bytes, towrite, &count, NULL)) break;
    bytes += count;
  }
  return bytes;
}

int OSW_RenameFile(const char *path, const char *new_path)
{
  LPWSTR wpath = _OSW_DecodeUTF8(path);
  LPWSTR new_wpath = _OSW_DecodeUTF8(new_path);
  int result = wpath && new_wpath && MoveFileW(wpath, new_wpath);
  if (new_wpath) OSW_free(new_wpath);
  if (wpath) OSW_free(wpath);
  return result;
}

int OSW_DeleteFile(const char *path)
{
  LPWSTR wpath = _OSW_DecodeUTF8(path);
  int result = wpath && DeleteFileW(wpath);
  if (wpath) OSW_free(wpath);
  return result;
}

int OSW_IsFile(const char *path)
{
  LPWSTR wpath = _OSW_DecodeUTF8(path);
  DWORD attr = wpath ? GetFileAttributesW(wpath) : INVALID_FILE_ATTRIBUTES;
  if (wpath) OSW_free(wpath);
  return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

int OSW_IsDirectory(const char *path)
{
  LPWSTR wpath = _OSW_DecodeUTF8(path);
  DWORD attr = wpath ? GetFileAttributesW(wpath) : INVALID_FILE_ATTRIBUTES;
  if (wpath) OSW_free(wpath);
  return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

int OSW_CreateDirectory(const char *path)
{
  LPWSTR wpath = _OSW_DecodeUTF8(path);
  int result = wpath && CreateDirectoryW(wpath, NULL);
  if (wpath) OSW_free(wpath);
  return result;
}

int OSW_DeleteDirectory(const char *path)
{
  LPWSTR wpath = _OSW_DecodeUTF8(path);
  int result = wpath && RemoveDirectoryW(wpath);
  if (wpath) OSW_free(wpath);
  return result;
}

/* THREAD API */

OSW_Thread OSW_CreateThread(OSW_ThreadProc *proc, void *param)
{
  return (OSW_Thread)CreateThread(
      NULL, 0, (LPTHREAD_START_ROUTINE)proc, param, 0, NULL);
}

int OSW_JoinThread(OSW_Thread thread, int *code)
{
  DWORD value;
  int result = WaitForSingleObject((HANDLE)thread, INFINITE) == WAIT_OBJECT_0;
  if (result)
  {
    if (code) *code = GetExitCodeThread((HANDLE)thread, &value) ? value : -1;
    CloseHandle((HANDLE)thread);
  }
  return result;
}

void OSW_ExitProcess(int code)
{
  ExitProcess(code);
}

void OSW_ExitThread(int code)
{
  ExitThread(code);
}

int OSW_SwitchThread(void)
{
  return SwitchToThread();
}

