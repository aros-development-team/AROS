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

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <aros/hostthread.h>

#include "dos_native.h"
#include "emul_handler_intern.h"

#define ST_ROOT 1
#define ST_USERDIR 2
#define ST_FILE -3

#define DERROR(x) x   /* Error code translation debug  */
#define DSTAT(x)     /* Stat() debug                  */
#define DSTATFS(x)   /* StatFS() debug		      */
#define DWINAPI(x)   /* WinAPI calls debug            */
#define DASYNC(x)    /* Asynchronous I/O thread debug */

HMODULE kernel_lib;
void (*CauseIRQ)(unsigned char irq, void *data);

/* Make an AROS error-code (<dos/dos.h>) out of an Windows error-code. */
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

/* Make unix protection bits out of AROS protection bits. */
mode_t prot_a2u(ULONG protect)
{
  mode_t uprot = 0;
  
  /* The following three flags are low-active! */
  if (!(protect & FIBF_EXECUTE))
	uprot |= S_IXUSR;
  if (!(protect & FIBF_WRITE))
	uprot |= S_IWUSR;
  if (!(protect & FIBF_READ))
	uprot |= S_IRUSR;
  
/*if ((protect & FIBF_GRP_EXECUTE))
	uprot |= S_IXGRP;
  if ((protect & FIBF_GRP_WRITE))
	uprot |= S_IWGRP;
  if ((protect & FIBF_GRP_READ))
	uprot |= S_IRGRP;
  
  if ((protect & FIBF_OTR_EXECUTE))
	uprot |= S_IXOTH;
  if ((protect & FIBF_OTR_WRITE))
	uprot |= S_IWOTH;
  if ((protect & FIBF_OTR_READ))
	uprot |= S_IROTH;
  
  if ((protect & FIBF_SCRIPT))
	uprot |= S_ISVTX;*/
  
  return uprot;
}

/*********************************************************************************************/

/* Make AROS protection bits out of unix protection bits. */
ULONG prot_u2a(mode_t protect)
{
  ULONG aprot = 0;
  
  /* The following three (AROS) flags are low-active! */
  if (!(protect & S_IRUSR))
	aprot |= FIBF_READ;
  if (!(protect & S_IWUSR))
	aprot |= FIBF_WRITE;
  if (!(protect & S_IXUSR))
	aprot |= FIBF_EXECUTE;
  
  /* The following flags are high-active again. */
/*if ((protect & S_IRGRP))
	aprot |= FIBF_GRP_READ;
  if ((protect & S_IWGRP))
	aprot |= FIBF_GRP_WRITE;
  if ((protect & S_IXGRP))
	aprot |= FIBF_GRP_EXECUTE;
  
  if ((protect & S_IROTH))
	aprot |= FIBF_OTR_READ;
  if ((protect & S_IWOTH))
	aprot |= FIBF_OTR_WRITE;
  if ((protect & S_IXOTH))
	aprot |= FIBF_OTR_EXECUTE;
  
  if ((protect & S_ISVTX))
	aprot |= FIBF_SCRIPT;*/
  
  return aprot;
}

int __declspec(dllexport) EmulChmod(const char *path, int protect)
{
  protect = prot_a2u(protect);
  DWINAPI(printf("[EmulHandler] chnmod(\"%s\")\n", path));
  return chmod(path,protect);
}

int __declspec(dllexport) EmulMKDir(const char *path, int protect)
{
  int r;

  protect = prot_a2u(protect);
  DWINAPI(printf("[EmulHandler] mkdir(\"%s\")\n", path));
  r = mkdir(path);
  if (!r) {
      DWINAPI(printf("[EmulHandler] chnmod(\"%s\")\n", path));
      r = chmod(path, protect);
  }
  return r;

}

int __declspec(dllexport) EmulStatFS(const char *path, struct InfoData *id)
{
  char buf[256];
  DWORD buflen;
  LPTSTR c;
  DWORD SectorsPerCluster, BytesPerSector, FreeBlocks;
  int retval = 0;
  LPTSTR newbuf = NULL;

  DSTATFS(printf("[EmulHandler] StatFS(\"%s\")\n", path));
  buflen = GetFullPathName(path, sizeof(buf), buf, &c);
  if (buflen) {
      if (buflen > sizeof(buf)) {
          DWINAPI(printf("[EmulHandler] LocalAlloc()\n", path));
          newbuf = LocalAlloc(LMEM_FIXED, buflen);
          if (newbuf) {
              DWINAPI(printf("[EmulHandler] GetFullPathName(\"%s\")\n", path));
              GetFullPathName(path, buflen, newbuf, &c);
          } else
              return 0;
      } else
          newbuf = buf;
      DSTATFS(printf("[EmulHandler] Full path: %s\n", newbuf));
/* TODO: this works only with drive letters, no mounts/UNC paths support yet */
      for (c = newbuf; *c; c++) {
	  if (*c == '\\') {
      	      c[1] = 0;
      	      break;
      	  }
      }
      if (*c) {
          DSTATFS(printf("[EmulHandler] Root path: %s\n", newbuf));
      	  retval = GetDiskFreeSpace(newbuf, &SectorsPerCluster, &BytesPerSector, &FreeBlocks, &id->id_NumBlocks);
  	  id->id_NumSoftErrors = 0;
	  id->id_UnitNumber = 0;
	  id->id_DiskState = ID_VALIDATED;
	  id->id_NumBlocksUsed = id->id_NumBlocks - FreeBlocks;
	  id->id_BytesPerBlock = SectorsPerCluster*BytesPerSector;
	  id->id_DiskType = ID_DOS_DISK;
	  id->id_InUse = TRUE;
      } else {
          DWINAPI(printf("[EmulHandler] SetLastError()\n"));
          SetLastError(ERROR_BAD_PATHNAME);
      }
      if (newbuf != buf) {
          DWINAPI(printf("[EmulHandler] LocalFree()\n"));
          LocalFree(newbuf);
      }
  }
  return retval;
}

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

void stat2FIB(struct stat * s, struct FileInfoBlock * FIB)
{
  FIB->fib_OwnerUID	  = s->st_uid;
  FIB->fib_OwnerGID	  = s->st_gid;
  FIB->fib_Comment[0]	  = '\0'; /* no comments available yet! */
  FIB->fib_Date.ds_Days   = s->st_ctime/(60*60*24) - (6*365 + 2*366);
  FIB->fib_Date.ds_Minute = (s->st_ctime/60)%(60*24);
  FIB->fib_Date.ds_Tick   = (s->st_ctime%60)*TICKS_PER_SECOND;
  FIB->fib_Protection	  = prot_u2a(s->st_mode);
  FIB->fib_Size           = s->st_size;

  if (S_ISDIR(s->st_mode))
  {
	FIB->fib_DirEntryType = ST_USERDIR;
  }
  else
  {
	FIB->fib_DirEntryType = ST_FILE;
  }
}

int __declspec(dllexport) EmulStat(const char *path, struct FileInfoBlock *FIB)
{
  struct stat st;
  int retval;
  int m;

  DWINAPI(printf("[EmulHandler] EmulStat(\"%s\")\n", path));
  retval = stat(path,&st);
  m = st.st_mode;
  DSTAT(printf("[EmulHandler] stat(\"%s\") returned %i kind: ",path,retval));
  if (retval == 0)
  {
    if (S_ISREG(m)) {
        retval = 1;
        DSTAT(printf("regular\n"));
    } else if (S_ISDIR(m)) {
        retval = 2;
        DSTAT(printf("directory\n"));
    }
        DSTAT(else printf("other\n");)
  } else {
    retval = -1;
    DSTAT(printf("unknown or not existing\n");)
  }
  if (FIB)
  {
	  stat2FIB(&st,FIB);
  }
  return retval;
}

int __declspec(dllexport) EmulErrno(void)
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
