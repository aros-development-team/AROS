/*
 *  emul_handler_native.c
 *  AROS
 *
 *  Created by Daniel Oberhoff on 06.04.08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "emul_handler_native.h"

#include <dos/dos.h>
#include <dos/filesystem.h>
#include <dos/exall.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <pwd.h>

#define ST_ROOT 1
#define ST_USERDIR 2
#define ST_FILE -3

#define NO_CASE_SENSITIVITY 1


/* Make an AROS error-code (<dos/dos.h>) out of an unix error-code. */
static LONG u2a[][2]=
{
  { ENOMEM, ERROR_NO_FREE_STORE },
  { ENOENT, ERROR_OBJECT_NOT_FOUND },
  { EEXIST, ERROR_OBJECT_EXISTS },
  { EACCES, ERROR_WRITE_PROTECTED }, /* AROS distinguishes between different
   kinds of privelege violation. Therefore
   a routine using err_u2a() should check
   for ERROR_WRITE_PROTECTED and replace
   it by a different constant, if
   necessary. */
  { ENOTDIR, ERROR_DIR_NOT_FOUND },
  { ENOSPC, ERROR_DISK_FULL },
  { ENOTEMPTY, ERROR_DIRECTORY_NOT_EMPTY },
  { EISDIR, ERROR_OBJECT_WRONG_TYPE },
  { ETXTBSY, ERROR_OBJECT_IN_USE },
  { ENAMETOOLONG, ERROR_OBJECT_TOO_LARGE },
  { EROFS, ERROR_WRITE_PROTECTED },
  { 0, ERROR_UNKNOWN }
};

static void fixpath(char *pathname)
{
  printf("supposed to fix path : %s\n",pathname);
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
  
  if ((protect & FIBF_GRP_EXECUTE))
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
	uprot |= S_ISVTX;
  
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
  if ((protect & S_IRGRP))
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
	aprot |= FIBF_SCRIPT;
  
  return aprot;
}

struct TagItem * EmulOpenDir(struct Hook * hook, void * object, struct TagItem * msg)
{
  const char * path = (const char *)object;
  return (struct TagItem *)opendir(path);
}

struct TagItem * EmulCloseDir(struct Hook * hook, void * object, struct TagItem * msg)
{
  DIR * dir = (DIR *)object;
  return (struct TagItem *)closedir(dir);
}

struct TagItem * EmulClose(struct Hook * hook, void * object, struct TagItem * msg)
{
  int fd = (int)object;
  return (struct TagItem *)close(fd);
}

struct TagItem * EmulGetCWD(struct Hook * hook, void * object, struct TagItem * msg)
{
  char * buf = (const char *)object;
  long len = (long)msg;
  char * retval = 0;
  if (buf != 0) //do not alloc mem here!
    retval = getcwd(buf,len);
  printf("[EmulHandler] cwd: \"%s\"\n",retval);
  return (struct TagItem *)retval;
}

struct TagItem * EmulOpen(struct Hook * hook, void * object, struct TagItem * msg)
{
  const char * path = (const char *)object;
  int mode = (int)msg->ti_Data; ++msg;
  int protect = (int)msg->ti_Data; ++msg;  
  int flags=(mode&FMF_CREATE?O_CREAT:0)|
  (mode&FMF_CLEAR?O_TRUNC:0);
  if(mode&FMF_WRITE)
	flags|=mode&FMF_READ?O_RDWR:O_WRONLY;
  else
	flags|=O_RDONLY;
  protect = prot_a2u(protect);
  return (struct TagItem *) open(path,mode,protect);
}  

struct TagItem * EmulLSeek(struct Hook * hook, void * object, struct TagItem * msg)
{
  int fd = (int)object;
  long offset = (long)msg->ti_Data; ++msg;
  long base = (long)msg->ti_Data; ++msg;  
  return (struct TagItem *)lseek(fd,offset,base);
}

struct TagItem * EmulRead(struct Hook * hook, void * object, struct TagItem * msg)
{
  int fd = (int)object;
  char * buf = (char*)msg->ti_Data; ++msg;
  long len = (long)msg->ti_Data; ++msg;  
  return (struct TagItem *)read(fd,buf,len);
}

struct TagItem * EmulReadLink(struct Hook * hook, void * object, struct TagItem * msg)
{
  int fd = (int)object;
  char * buf = (char*)msg->ti_Data; ++msg;
  long len = (long)msg->ti_Data; ++msg;  
  return (struct TagItem *)readlink(fd,buf,len);
}

struct TagItem * EmulWrite(struct Hook * hook, void * object, struct TagItem * msg)
{
  int fd = (int)object;
  char * buf = (char*)msg->ti_Data; ++msg;
  long len = (long)msg->ti_Data; ++msg;  
  return (struct TagItem *)write(fd,buf,len);
}

struct TagItem * EmulChmod(struct Hook * hook, void * object, struct TagItem * msg)
{
  const char * path = (const char *)object;
  int protect = (int)msg;
  protect = prot_a2u(protect);
  return (struct TagItem *)chmod(path,protect);
}

struct TagItem * EmulMKDir(struct Hook * hook, void * object, struct TagItem * msg)
{
  const char * path = (const char *)object;
  int protect = (int)msg;
  protect = prot_a2u(protect);
  return (struct TagItem *)mkdir(path,protect);
}

struct TagItem * EmulIsatty(struct Hook * hook, void * object, struct TagItem * msg)
{
  const char * path = (const char *)object;
  return (struct TagItem *)isatty(path);
}

struct TagItem * EmulChDir(struct Hook * hook, void * object, struct TagItem * msg)
{
  const char * path = (const char *)object;
  return (struct TagItem *)chdir(path);
}

struct TagItem * EmulLink(struct Hook * hook, void * object, struct TagItem * msg)
{
  const char * path1 = (const char *)object;
  const char * path2 = (const char *)msg;
  return (struct TagItem *)link(path1,path2);
}

struct TagItem * EmulSymLink(struct Hook * hook, void * object, struct TagItem * msg)
{
  const char * path1 = (const char *)object;
  const char * path2 = (const char *)msg;
  return (struct TagItem *)symlink(path1,path2);
}

struct TagItem * EmulStatFS(struct Hook * hook, void * object, struct TagItem * msg)
{
  const char * path = (const char *)object;
  struct InfoData *id = (struct InfoData *)msg;
  struct statfs    buf;
  
  int retval = statfs(path, &buf);
  
  id->id_NumSoftErrors = 0;
  id->id_UnitNumber = 0;
  id->id_DiskState = ID_VALIDATED;
  id->id_NumBlocks = buf.f_blocks;
  id->id_NumBlocksUsed = buf.f_blocks - buf.f_bavail;
  id->id_BytesPerBlock = buf.f_bsize;
  id->id_DiskType = ID_DOS_DISK; /* Well, not really true... */
  id->id_VolumeNode = NULL;
  id->id_InUse = TRUE;
  
  return retval;
}

struct TagItem * EmulClosePW(struct Hook * hook, void * object, struct TagItem * msg)
{
  endpwent();
  return 0;
}

struct TagItem * EmulGetHome(struct Hook * hook, void * object, struct TagItem * msg)
{
  const char * name = (const char *)object;
  struct passwd * pwd = getpwnam(name);
  const char * home = pwd ? pwd->pw_dir : 0;
  return (struct TagItem *)home;
}


struct TagItem * EmulDelete(struct Hook * hook, void * object, struct TagItem * msg)
{
  const char * filename = (const char *)object;
  struct stat st;
  if (!lstat(filename,&st))
  {
	if (S_ISDIR(st.st_mode))
	  rmdir(filename);
	else
	  unlink(filename);
  }
  return 0;
}

struct TagItem * EmulDirName(struct Hook * hook, void * object, struct TagItem * msg)
{
  DIR * dir = (DIR *)object;
  struct dirent * entry = readdir(dir);
  const char * name = entry ? entry->d_name : 0;
  return (struct TagItem *)name;
}

struct TagItem * EmulTellDir(struct Hook * hook, void * object, struct TagItem * msg)
{
  DIR * dir = (DIR *)object;
  return (struct TagItem *)telldir(dir);
}

struct TagItem * EmulRewindDir(struct Hook * hook, void * object, struct TagItem * msg)
{
  DIR * dir = (DIR *)object;
  rewinddir(dir);
  return 0;
}

struct TagItem * EmulSeekDir(struct Hook * hook, void * object, struct TagItem * msg)
{
  DIR * dir = (DIR *)object;
  long loc = (long)msg;
  seekdir(dir,loc);
  return 0;
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

struct TagItem * EmulStat(struct Hook * hook, void * object, struct TagItem * msg)
{
  const char * path = (const char *)object;
  struct FileInfoBlock * FIB = (struct FileInfoBlock *)msg;
  struct stat st;
  int retval = stat(path,&st);
  printf("[EmulHandler] stat(%s) returned %i kind: ",path,retval);
  int m = st.st_mode;
  if (retval == 0)
  {
    if (S_ISREG(m)) { retval = 1; printf("regular\n"); }
	  else if (S_ISDIR(m)) { retval = 2; printf("directory\n"); }
	  else { retval = -1; printf("other\n"); }
    } else {printf("unknown or not existing\n");}
  if (FIB)
  {
	  stat2FIB(&st,FIB);
  }
  return (struct TagItem *) retval;
}
struct TagItem * EmulGetEnv(struct Hook * hook, void * object, struct TagItem * msg)
{
  const char * name = (const char *)object;
  return (struct TagItem *)getenv(name);
}

struct TagItem * EmulRename(struct Hook * hook, void * object, struct TagItem * msg)
{
  const char * spath = (const char *)object;
  const char * dpath = (const char *)msg;
  return (struct TagItem*)rename(spath,dpath);
}

struct TagItem * EmulLStat(struct Hook * hook, void * object, struct TagItem * msg)
{
  const char * path = (const char *)object;
  struct FileInfoBlock * FIB = (struct FileInfoBlock *)msg;
  struct stat st;
  int retval = lstat(path,&st);
  int m = st.st_mode;
  if (retval)
  {
	if (S_ISREG(m)) retval = 1;
	else if (S_ISDIR(m)) retval = 2;
	else retval = -1;
  }
  if (FIB)
  {
	stat2FIB(&st,FIB);
  }
  return (struct TagItem *) retval;
}

struct TagItem * EmulErrno(struct Hook * hook, void * object, struct TagItem * msg)
{
  ULONG i;
  int e = errno; errno = 0;
  for(i=0;i<sizeof(u2a)/sizeof(u2a[0]);i++)
	if(u2a[i][0]==e)
	  return (struct TagItem*)u2a[i][1];
#if PassThroughErrnos
  return (struct TagItem *)(e+PassThroughErrnos);
#else
  return (struct TagItem *)ERROR_UNKNOWN;
#endif
}

