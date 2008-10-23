/*
 *  emul_handler_native.c
 *  AROS
 *
 *  Created by Daniel Oberhoff on 06.04.08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#define _WIN32_IE 0x0500
#include <windows.h>
#include <shlobj.h>

#include <stdio.h>
#include <aros/hostthread.h>

#include "dos_native.h"
#include "emul_handler_intern.h"

#define DERROR(x)    /* Error code translation debug  */
#define DSTAT(x)     /* Stat() debug                  */
#define DSTATFS(x)   /* StatFS() debug		      */
#define DWINAPI(x)   /* WinAPI calls debug            */
#define DASYNC(x)    /* Asynchronous I/O thread debug */

HMODULE kernel_lib;
void (*CauseIRQ)(unsigned char irq, void *data);

/* Make an AROS error-code (<dos/dos.h>) out of a Windows error-code. */
static DWORD u2a[][2]=
{
  { ERROR_PATH_NOT_FOUND, AROS_ERROR_OBJECT_NOT_FOUND },
  { ERROR_ACCESS_DENIED, ERROR_OBJECT_WRONG_TYPE },
  { ERROR_NO_MORE_FILES, ERROR_NO_MORE_ENTRIES },
  { ERROR_NOT_ENOUGH_MEMORY, ERROR_NO_FREE_STORE },
  { ERROR_FILE_NOT_FOUND, AROS_ERROR_OBJECT_NOT_FOUND },
  { ERROR_FILE_EXISTS, ERROR_OBJECT_EXISTS },
  { ERROR_WRITE_PROTECT, ERROR_WRITE_PROTECTED },
  { ERROR_DISK_FULL, AROS_ERROR_DISK_FULL },
  { ERROR_DIR_NOT_EMPTY, ERROR_DIRECTORY_NOT_EMPTY },
  { ERROR_SHARING_VIOLATION, ERROR_OBJECT_IN_USE },
  { ERROR_LOCK_VIOLATION, ERROR_OBJECT_IN_USE },
  { ERROR_BUFFER_OVERFLOW, ERROR_OBJECT_TOO_LARGE },
  { ERROR_INVALID_NAME, AROS_ERROR_OBJECT_NOT_FOUND },
  { 0, 0 }
};

/*********************************************************************************************/

unsigned long __declspec(dllexport) EmulGetHome(const char *name, char *home)
{
  HRESULT res;

  /* TODO: currently username is ignored, however we should acquire an access token for it */
  DWINAPI(printf("[EmulHandler] SHGetFolderPath()\n"));
  res = SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_DEFAULT, home);
  if (res)
      return AROS_ERROR_OBJECT_NOT_FOUND;
  else
      return 0;
}

BOOL __declspec(dllexport) EmulDelete(const char *filename)
{
  DWORD attrs;
  unsigned long res = FALSE;
  
  attrs = GetFileAttributes(filename);
  if (attrs != INVALID_FILE_ATTRIBUTES) {
      if (attrs & FILE_ATTRIBUTE_DIRECTORY)
          res = RemoveDirectory(filename);
      else
          res = DeleteFile(filename);
  }
  return res;
}

LONG __declspec(dllexport) EmulStat(const char *path, WIN32_FILE_ATTRIBUTE_DATA *FIB)
{
  DWORD retval;
  DWORD m;

  DSTAT(printf("[EmulHandler] Stat(%s, 0x%p)\n", path, FIB));
  if (FIB) {
      retval = GetFileAttributesEx(path, GetFileExInfoStandard, FIB);
      m = FIB->dwFileAttributes;
  } else {
      m = GetFileAttributes(path);
      retval = (m != INVALID_FILE_ATTRIBUTES);
  }
  DSTAT(printf("[EmulHandler] Return value %ld, object attributes 0x%08lX\n", retval, m));
  if (retval) {
      /* TODO: Here we may also probably recognize hard and soft links */
      if (m & FILE_ATTRIBUTE_DIRECTORY) {
          DSTAT(printf("[EmulHandler] Object is a directory\n"));
          retval = ST_USERDIR;
      } else {
          DSTAT(printf("[EmulHandler] Object is a file\n"));
          retval = ST_FILE;
      }
  }
  return retval;
}

ULONG __declspec(dllexport) EmulErrno(void)
{
  ULONG i;
  DWORD e;
  
  DWINAPI(printf("[EmulHandler] GetLastError\n"));
  e = GetLastError();
  DERROR(printf("[EmulHandler] Windows error code: %lu\n", e));
  for(i=0;i<sizeof(u2a)/sizeof(u2a[0]);i++)
	if(u2a[i][0]==e) {
	  DERROR(printf("[EmulHandler] Translated to AROS error code: %lu\n", u2a[i][1]));
	  return u2a[i][1];
	}
  DERROR(printf("[EmulHandler] Unknown error code\n"));
  return ERROR_UNKNOWN;
}

int __declspec(dllexport) EmulStatFS(const char *path, struct InfoData *id)
{
  LPTSTR c;
  char s;
  DWORD SectorsPerCluster, BytesPerSector, FreeBlocks;
  BOOL res = 0;

  DSTATFS(printf("[EmulHandler] StatFS(\"%s\")\n", path));
  /* GetDiskFreeSpace() can be called only on root path. We always work with absolute pathnames, so let's get first part of the path */
  c = path;
  if ((c[0] == '\\') && (c[1] == '\\')) {
      /* If the path starts with "\\", it's a UNC path. Its root is "\\Server\Share\", so we skip "\\Server\" part */
      c += 2;
      while (*c != '\\') {
          if (*c == 0)
              return AROS_ERROR_OBJECT_NOT_FOUND;
          c++;
      }
      c++;
  }
  /* Skip everything up to the first '\'. */
  while (*c != '\\') {
      if (*c == 0)
          return AROS_ERROR_OBJECT_NOT_FOUND;
      c++;
  }
  /* Temporarily terminate the path */
  s = c[1];
  c[1] = 0;
  DSTATFS(printf("[EmulHandler] Root path: %s\n", path));
  res = GetDiskFreeSpace(path, &SectorsPerCluster, &BytesPerSector, &FreeBlocks, &id->id_NumBlocks);
  c[1] = s;
  if (res) {
      id->id_NumSoftErrors = 0;
      id->id_UnitNumber = 0;
      id->id_DiskState = ID_VALIDATED;
      id->id_NumBlocksUsed = id->id_NumBlocks - FreeBlocks;
      id->id_BytesPerBlock = SectorsPerCluster*BytesPerSector;
      id->id_DiskType = ID_DOS_DISK;
      id->id_InUse = TRUE;
      return 0;
  }
  return EmulErrno();
}

DWORD __declspec(dllexport) EmulThread(struct ThreadHandle *THandle)
{
    struct EmulThreadMessage *emsg;
    BOOL res;

    DASYNC(printf("[EmulHandler I/O] Thread started, handle 0x%08lX, host handle 0x%08lX, host ID %lu\n", THandle, THandle->handle, THandle->id));
    for (;;) {
        emsg = HT_GetMsg();
        DASYNC(printf("[EmulHandler I/O] Got message: 0x%p\n", emsg));
        if (emsg && (emsg != (struct EmulThreadMessage *)-1)) {
	    switch(emsg->op) {
	    case EMUL_CMD_READ:
	        DASYNC(printf("[EmulHandler I/O] READ %lu bytes at 0x%p, file 0x%p\n", emsg->len, emsg->addr, emsg->fh));
	        res = ReadFile(emsg->fh, emsg->addr, emsg->len, &emsg->actual, NULL);
	    	break;
	    case EMUL_CMD_WRITE:
	        DASYNC(printf("[EmulHandler I/O] WRITE %lu bytes at 0x%p, file 0x%p\n", emsg->len, emsg->addr, emsg->fh));
	        res = WriteFile(emsg->fh, emsg->addr, emsg->len, &emsg->actual, NULL);
	    	break;
	    }
	    emsg->error = res ? 0 : EmulErrno();
	    DASYNC(printf("[EmulHandler I/O] %lu bytes transferred, result %ld, error %lu\n", emsg->actual, res, emsg->error));
	    HT_CauseInterrupt(emsg);
	} else {
	    DASYNC(printf("[EmulHandler I/O] shutting down thread\n"));
	    return 0;
	}
    }
}
