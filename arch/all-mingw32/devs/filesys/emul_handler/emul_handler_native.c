/*
 *  emul_handler_native.c
 *  AROS
 *
 *  Created by Daniel Oberhoff on 06.04.08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#define DEBUG

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

#include "dos_native.h"

#define ST_ROOT 1
#define ST_USERDIR 2
#define ST_FILE -3

#define D(x) x

/* Make an AROS error-code (<dos/dos.h>) out of an Windows error-code. */
static DWORD u2a[][2]=
{
  { ERROR_NOT_ENOUGH_MEMORY, ERROR_NO_FREE_STORE },
  { ERROR_FILE_NOT_FOUND, AROS_ERROR_OBJECT_NOT_FOUND },
  { ERROR_FILE_EXISTS, ERROR_OBJECT_EXISTS },
  { ERROR_WRITE_PROTECT, ERROR_WRITE_PROTECTED },
  { ERROR_DISK_FULL, AROS_ERROR_DISK_FULL },
  { ERROR_DIR_NOT_EMPTY, ERROR_DIRECTORY_NOT_EMPTY },
  { ERROR_SHARING_VIOLATION, ERROR_OBJECT_IN_USE },
  { ERROR_LOCK_VIOLATION, ERROR_OBJECT_IN_USE },
  { ERROR_BUFFER_OVERFLOW, ERROR_OBJECT_TOO_LARGE },
  { 0, ERROR_UNKNOWN }
};

static void fixpath(char *pathname)
{
  D(printf("supposed to fix path : %s\n",pathname));
#if 0  
  struct dirent 	*de;
  struct stat		st;
  DIR			*dir;
  char		*pathstart, *pathend;
  BOOL		dirfound;
  
  pathstart = pathname;

  if (stat((const char *)pathname, &st) != 0)
  {
	/* file/dir called pathname does not exist */
	
	while((pathstart = strchr(pathstart, '/')))
	{
	  pathstart++;
	  
	  pathend = strchr(pathstart, '/');
	  if (pathend) *pathend = '\0';
	  
	  dirfound = TRUE;
	  
	  if (stat((const char *)pathname, &st) != 0)
	  {
		dirfound = FALSE;
		
		pathstart[-1] = '\0';
		dir = opendir(pathname);
		pathstart[-1] = '/';		
		
		if (dir)
		{
		  while((de = readdir(dir)))
		  {
			if (strcasecmp(de->d_name, pathstart) == 0)
			{
			  dirfound = TRUE;
			  strcpy(pathstart, de->d_name);
			  break;
			}
		  }	    
		  closedir(dir);
		  
		} /* if ((dir = opendir(pathname))) */
		
	  } /* if (stat((const char *)pathname, &st) != 0) */
	  
	  if (pathend) *pathend = '/';			    
	  
	  if (!dirfound) break;
	  
	} /* while((pathpos = strchr(pathpos, '/))) */
	
  } /* if (stat((const char *)pathname, &st) != 0) */
  
#endif
}

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

void * __declspec(dllexport) EmulOpenDir(const char *path)
{
  return opendir(path);
}

int __declspec(dllexport) EmulCloseDir(void *dir)
{
  return closedir((DIR *)dir);
}

int __declspec(dllexport) EmulClose(int fd)
{
  return close(fd);
}

char * __declspec(dllexport) EmulGetCWD(char *buf, long len)
{
  char * retval = 0;
  if (buf != 0) //do not alloc mem here!
    retval = getcwd(buf,len);
  printf("[EmulHandler] cwd: \"%s\"\n",retval);
  return (struct TagItem *)retval;
}

int __declspec(dllexport) EmulOpen(const char *path, int mode, int protect)
{
  int flags=(mode&FMF_CREATE?O_CREAT:0)|(mode&FMF_CLEAR?O_TRUNC:0);

  if(mode&FMF_WRITE)
	flags|=mode&FMF_READ?O_RDWR:O_WRONLY;
  else
	flags|=O_RDONLY;
  protect = prot_a2u(protect);
  return open(path, flags, protect);
}  

long __declspec(dllexport) EmulLSeek(int fd, long offset, long base)
{
  return lseek(fd,offset,base);
}

int __declspec(dllexport) EmulRead(int fd, char *buf, long len)
{
  return read(fd,buf,len);
}

int __declspec(dllexport) EmulWrite(int fd, char *buf, long len)
{
  return write(fd,buf,len);
}

int __declspec(dllexport) EmulChmod(const char *path, int protect)
{
  protect = prot_a2u(protect);
  return chmod(path,protect);
}

int __declspec(dllexport) EmulMKDir(const char *path, int protect)
{
  int r;

  protect = prot_a2u(protect);
  r = mkdir(path);
  if (!r)
      r = chmod(path, protect);
  return r;

}

int __declspec(dllexport) EmulIsatty(int fd)
{
  return isatty(fd);
}

int __declspec(dllexport) EmulChDir(const char *path)
{
  return chdir(path);
}


int __declspec(dllexport) EmulStatFS(const char *path, struct InfoData *id)
{
  char buf[256];
  DWORD buflen;
  LPTSTR c;
  DWORD SectorsPerCluster, BytesPerSector, FreeBlocks;
  int retval = 0;
  LPTSTR newbuf = NULL;

  D(printf("[EmulHandler] StatFS(\"%s\")\n", path));
  buflen = GetFullPathName(path, sizeof(buf), buf, &c);
  if (buflen) {
      if (buflen > sizeof(buf)) {
          newbuf = LocalAlloc(LMEM_FIXED, buflen);
          if (newbuf) {
              GetFullPathName(path, buflen, newbuf, &c);
          } else
              return 0;
      } else
          newbuf = buf;
      D(printf("[EmulHandler] Full path: %s\n", newbuf));
/* TODO: this works only with drive letters, no mounts/UNC paths support yet */
      for (c = newbuf; *c; c++) {
	  if (*c == '\\') {
      	      c[1] = 0;
      	      break;
      	  }
      }
      if (*c) {
          D(printf("[EmulHandler] Root path: %s\n", newbuf));
      	  retval = GetDiskFreeSpace(newbuf, &SectorsPerCluster, &BytesPerSector, &FreeBlocks, &id->id_NumBlocks);
  	  id->id_NumSoftErrors = 0;
	  id->id_UnitNumber = 0;
	  id->id_DiskState = ID_VALIDATED;
	  id->id_NumBlocksUsed = id->id_NumBlocks - FreeBlocks;
	  id->id_BytesPerBlock = SectorsPerCluster*BytesPerSector;
	  id->id_DiskType = ID_DOS_DISK;
	  id->id_InUse = TRUE;
      } else
          SetLastError(ERROR_BAD_PATHNAME);
      if (newbuf != buf)
          LocalFree(newbuf);
  }
  return retval;
}

unsigned long __declspec(dllexport) EmulGetHome(const char *name, char *home)
{
  HRESULT res;

  /* TODO: currently username is ignored, however we should acquire an access token for it */
  res = SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_DEFAULT, home);
  if (res)
      return AROS_ERROR_OBJECT_NOT_FOUND;
  else
      return 0;
}

void __declspec(dllexport) EmulDelete(const char *filename)
{
  struct stat st;
  if (!stat(filename,&st))
  {
	if (S_ISDIR(st.st_mode))
	  rmdir(filename);
	else
	  unlink(filename);
  }
}

const char * __declspec(dllexport) EmulDirName(void *dir)
{
  struct dirent * entry = readdir((DIR *)dir);
  const char * name = entry ? entry->d_name : NULL;
  return name;
}

int __declspec(dllexport) EmulTellDir(void *dir)
{
  return telldir((DIR *)dir);
}

void __declspec(dllexport) EmulRewindDir(void *dir)
{
  rewinddir((DIR *)dir);
}

void __declspec(dllexport) EmulSeekDir(void *dir, long loc)
{
  seekdir((DIR *)dir,loc);
}

void stat2FIB(struct stat * s, struct FileInfoBlock * FIB)
{
  FIB->fib_OwnerUID	    = s->st_uid;
  FIB->fib_OwnerGID	    = s->st_gid;
  FIB->fib_Comment[0]	    = '\0'; /* no comments available yet! */
  FIB->fib_Date.ds_Days   = s->st_ctime/(60*60*24) - (6*365 + 2*366);
  FIB->fib_Date.ds_Minute = (s->st_ctime/60)%(60*24);
  FIB->fib_Date.ds_Tick   = (s->st_ctime%60)*TICKS_PER_SECOND;
  FIB->fib_Protection	    = s->st_mode;
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
  int retval = stat(path,&st);
  int m = st.st_mode;

  D(printf("[EmulHandler] stat(%s) returned %i kind: ",path,retval));
  if (retval == 0)
  {
    if (S_ISREG(m)) {
        retval = 1;
        D(printf("regular\n"));
    } else if (S_ISDIR(m)) {
        retval = 2;
        D(printf("directory\n"));
    } else {
        retval = -1;
        D(printf("other\n"));
    }
  }
  D(else printf("unknown or not existing\n");)
  if (FIB)
  {
	  stat2FIB(&st,FIB);
  }
  return retval;
}

int __declspec(dllexport) EmulRename(const char *spath, const char *dpath)
{
  return rename(spath,dpath);
}

int __declspec(dllexport) EmulErrno(void)
{
  ULONG i;
  DWORD e = GetLastError();

  D(printf("[EmulHandler] Win32 error code: %lu\n", e));
  for(i=0;i<sizeof(u2a)/sizeof(u2a[0]);i++)
	if(u2a[i][0]==e) {
	  D(printf("EmulHandler] Translated to AROS error code: 0x%08lX\n", u2a[i][1]));
	  return u2a[i][1];
	}
  D(printf("[EmulHandler] Unknown error code\n"));
#if PassThroughErrnos
  return e+PassThroughErrnos;
#else
  return ERROR_UNKNOWN;
#endif
}

