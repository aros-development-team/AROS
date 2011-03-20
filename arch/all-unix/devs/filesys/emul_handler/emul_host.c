/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifdef HOST_OS_ios

#ifdef __arm__
/*
 * Under ARM iOS quadwords are long-aligned, however in AROS (according to AAPCS)
 * they are quad-aligned. This macro turns on some tricks which bypass this problem
 */
#define HOST_LONG_ALIGNED
#endif
#ifdef __i386__
/*
 * Under i386 we pick up MacOS' libSystem.dylib instead of Simulator's libSystem.dylib,
 * so we have to use special versions of certain functions. We can't simply #define _DARWIN_NO_64_BIT_INODE
 * because iOS SDK forbids this (in iOS inode_t is always 64-bit wide)
 */
#define INODE64_SUFFIX "$INODE64"
#endif

#else

/* 
 * Use 32-bit inode_t on Darwin. Otherwise we are expected to use "stat$INODE64"
 * instead of "stat" function which is available only on MacOS 10.6.
 */
#define _DARWIN_NO_64_BIT_INODE
#endif

#ifndef INODE64_SUFFIX
#define INODE64_SUFFIX
#endif

#ifdef HOST_LONG_ALIGNED
#pragma pack(4)
#endif

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#pragma pack()

/* This prevents redefinition of struct timeval */
#define _AROS_TYPES_TIMEVAL_S_H_

#define DEBUG 0
#define DASYNC(x)
#define DEXAM(x)
#define DMOUNT(x)
#define DOPEN(x)
#define DREAD(x)
#define DSEEK(x)

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <dos/dosasl.h>
#include <utility/date.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/hostlib.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include "emul_intern.h"

#define NO_CASE_SENSITIVITY

#ifdef DEBUG_INTERFACE
#define DUMP_INTERFACE					\
{							\
    int i;						\
    APTR *iface = (APTR *)emulbase->pdata.SysIFace;	\
							\
    for (i = 0; libcSymbols[i]; i++)			\
	bug("%s\t\t0x%P\n", libcSymbols[i], iface[i]);	\
}
#else
#define DUMP_INTERFACE
#endif

/*********************************************************************************************/

static int TryRead(struct LibCInterface *iface, int fd, void *buf, size_t len)
{
    fd_set rfds;
    struct timeval tv = {0, 0};
    int res;

    FD_ZERO (&rfds);
    FD_SET(fd, &rfds);

    res = iface->select(fd+1, &rfds, NULL, NULL, &tv);
    AROS_HOST_BARRIER

    if (res == -1)
    {
	DASYNC(bug("[emul] Select error\n"));
	return -1;
    }

    if (res == 0)
	return -2;
    
    res = iface->read(fd, buf, len);
    AROS_HOST_BARRIER
    
    return res;
}

static void SigIOHandler(struct emulbase *emulbase, void *unused)
{
    struct IOFileSys *req, *req2;

    ForeachNodeSafe(&emulbase->pdata.readList, req, req2)
    {
	int len;
	struct filehandle *fh = (struct filehandle *)req->IOFS.io_Unit;

	/* Try to process the request */
        len = TryRead(emulbase->pdata.SysIFace, (IPTR)fh->fd, req->io_Union.io_READ.io_Buffer, req->io_Union.io_READ.io_Length);
	if (len != -2)
	{
	    /* Reply the requst if it is done */
	    Remove((struct Node *)req);	
	    if (len == -1)
		req->io_DosError = *emulbase->pdata.errnoPtr;
	    else {
		req->io_Union.io_READ.io_Length = len;
		req->io_DosError = 0;
	    }
	    DASYNC(bug("[emul] Replying request 0x%P, result %d, error %d\n", req, len, req->io_DosError));
	    ReplyMsg(&req->IOFS.io_Message);
	}
	DASYNC(bug("-"));
    };
}

/*********************************************************************************************/

static inline struct filehandle *CreateStdHandle(int fd)
{
    struct filehandle *fh;

    fh = AllocMem(sizeof(struct filehandle), MEMF_PUBLIC|MEMF_CLEAR);
    if (fh)
    {
	fh->type = FHD_FILE|FHD_STDIO;
	fh->fd   = (void *)(IPTR)fd;
    }

    return fh;
}

static const char *libcSymbols[] = {
    "open",
    "close",
    "closedir",
    "opendir",
    "readdir" INODE64_SUFFIX,
    "rewinddir",
    "read",
    "write",
    "lseek",
    "ftruncate",
    "mkdir",
    "rmdir",
    "unlink",
    "link",
    "symlink",
    "readlink",
    "rename",
    "chmod",
    "isatty",
    "statfs",
    "utime",
    "localtime",
    "mktime",
    "getcwd",
    "getenv",
    "fcntl",
    "select",
    "kill",
    "getpid",
#ifndef HOST_OS_android
    "seekdir",
    "telldir",
    "getpwent",
    "endpwent",
#endif
#ifdef HOST_OS_linux
    "__errno_location",
    "__xstat",
    "__lxstat",
#else
#ifdef HOST_OS_android
    "__errno",
#else
    "__error",
#endif
    "stat" INODE64_SUFFIX,
    "lstat" INODE64_SUFFIX,
#endif
    NULL
};

static int host_startup(struct emulbase *emulbase)
{
    ULONG r = 0;

    HostLibBase = OpenResource("hostlib.resource");

    D(bug("[EmulHandler] got hostlib.resource %p\n", HostLibBase));
    if (!HostLibBase)
	return FALSE;

    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase)
	return FALSE;

    emulbase->pdata.libcHandle = HostLib_Open(LIBC_NAME, NULL);
    if (!emulbase->pdata.libcHandle)
    	return FALSE;

    emulbase->pdata.SysIFace = (struct LibCInterface *)HostLib_GetInterface(emulbase->pdata.libcHandle, libcSymbols, &r);
    if (!emulbase->pdata.SysIFace)
    {
        D(bug("[EmulHandler] Unable go get host-side library interface!\n"));
    	return FALSE;
    }

    D(bug("[EmulHandler] %lu unresolved symbols!\n", r));
    DUMP_INTERFACE
    if (r)
    	return FALSE;

    emulbase->ReadIRQ = KrnAddIRQHandler(SIGIO, SigIOHandler, emulbase, NULL);
    if (!emulbase->ReadIRQ)
	return FALSE;

    emulbase->eb_stdin  = CreateStdHandle(STDIN_FILENO);
    emulbase->eb_stdout = CreateStdHandle(STDOUT_FILENO);
    emulbase->eb_stderr = CreateStdHandle(STDERR_FILENO);

    InitSemaphore(&emulbase->pdata.sem);
    NEWLIST(&emulbase->pdata.readList);
    emulbase->pdata.my_pid   = emulbase->pdata.SysIFace->getpid();
    AROS_HOST_BARRIER
    emulbase->pdata.errnoPtr = emulbase->pdata.SysIFace->__error();
    AROS_HOST_BARRIER

    return TRUE;
}

ADD2INITLIB(host_startup, 0);

static int host_cleanup(struct emulbase *emulbase)
{
    D(bug("[EmulHandler] Expunge\n"));

    if (!HostLibBase)
    	return TRUE;

    if (emulbase->pdata.SysIFace)
    	HostLib_DropInterface((APTR *)emulbase->pdata.SysIFace);

    if (emulbase->pdata.libcHandle)
    	HostLib_Close(emulbase->pdata.libcHandle, NULL);

    if (!KernelBase)
	return TRUE;

    KrnRemIRQHandler(emulbase->ReadIRQ);

    return TRUE;
}

ADD2EXPUNGELIB(host_cleanup, 0);
 
/*********************************************************************************************/

/* Make an AROS error-code (<dos/dos.h>) out of a unix error-code. */
static LONG u2a[][2]=
{
    { ENOMEM   , ERROR_NO_FREE_STORE         },
    { ENOENT   , ERROR_OBJECT_NOT_FOUND      },
    { EEXIST   , ERROR_OBJECT_EXISTS         },
    { EACCES   , ERROR_WRITE_PROTECTED       }, /* AROS distinguishes between different
                                        kinds of privelege violation. Therefore
                                        a routine using err_u2a(emulbase) should check
                                        for ERROR_WRITE_PROTECTED and replace
                                        it by a different constant, if
                                        necessary. */
    { ENOTDIR     , ERROR_DIR_NOT_FOUND      },
    { ENOSPC      , ERROR_DISK_FULL          },
    { ENOTEMPTY   , ERROR_DIRECTORY_NOT_EMPTY},
    { EISDIR      , ERROR_OBJECT_WRONG_TYPE  },
    { ETXTBSY     , ERROR_OBJECT_IN_USE      },
    { ENAMETOOLONG, ERROR_OBJECT_TOO_LARGE   },
    { EROFS       , ERROR_WRITE_PROTECTED    },
    { 0           , 0            	     }
};

static LONG err_u2a(struct emulbase *emulbase)
{
    ULONG i;
    int err = *emulbase->pdata.errnoPtr;

    for (i = 0; i < sizeof(u2a)/sizeof(u2a[0]); i++)
    {
	if (u2a[i][0] == err)
	    return u2a[i][1];
    }

#ifdef PassThroughErrnos
    return err + PassThroughErrnos;
#else
    return ERROR_UNKNOWN;
#endif
}

/*********************************************************************************************/

/* Make unix protection bits out of AROS protection bits. */
static mode_t prot_a2u(ULONG protect)
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
static ULONG prot_u2a(mode_t protect)
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

/*********************************************************************************************/

/* Converts AROS file access mode to *nix open() flags */
static int mode2flags(LONG mode)
{
    int flags;

    flags=(mode&FMF_CREATE?O_CREAT:0)|
          (mode&FMF_CLEAR?O_TRUNC:0);
    if(mode&FMF_WRITE)
        flags|=mode&FMF_READ?O_RDWR:O_WRONLY;
    else
        flags|=O_RDONLY;
    
    return flags;
}

/*********************************************************************************************/

static void timestamp2datestamp(struct emulbase *emulbase, time_t *timestamp, struct DateStamp *datestamp)
{
    struct ClockData date;
    struct tm *tm;

    ObtainSemaphore(&emulbase->pdata.sem);

    tm = emulbase->pdata.SysIFace->localtime(timestamp);
    AROS_HOST_BARRIER
    
    ReleaseSemaphore(&emulbase->pdata.sem);

    date.year  = tm->tm_year + 1900;
    date.month = tm->tm_mon + 1;
    date.mday  = tm->tm_mday;
    date.hour  = tm->tm_hour;
    date.min   = tm->tm_min;
    date.sec   = tm->tm_sec;

    ULONG secs = Date2Amiga(&date);

    datestamp->ds_Days = secs / (60 * 60 * 24);
    secs %= (60 * 60 * 24);
    datestamp->ds_Minute = secs / 60;
    secs %= 60;
    datestamp->ds_Tick = secs * TICKS_PER_SECOND;
}

/*********************************************************************************************/

static time_t datestamp2timestamp(struct LibCInterface *iface, struct DateStamp *datestamp)
{
    ULONG secs = datestamp->ds_Days * (60 * 60 * 24) + 
                 datestamp->ds_Minute * 60 +
                 datestamp->ds_Tick / TICKS_PER_SECOND;
    
    struct ClockData date;
    struct tm tm;
    time_t ret;

    Amiga2Date(secs, &date);

    tm.tm_year = date.year - 1900;
    tm.tm_mon = date.month - 1;
    tm.tm_mday = date.mday;
    tm.tm_hour = date.hour;
    tm.tm_min = date.min;
    tm.tm_sec = date.sec;
    
    ret = iface->mktime(&tm);
    AROS_HOST_BARRIER

    return ret;
}

/*********************************************************************************************/

#ifdef NO_CASE_SENSITIVITY

static void fixcase(struct LibCInterface *iface, char *pathname)
{
    struct dirent 	*de;
    struct stat	st;
    DIR			*dir;
    char		*pathstart, *pathend;
    BOOL		dirfound;
    int			res;

    pathstart = pathname;

    res = iface->lstat((const char *)pathname, &st);
    AROS_HOST_BARRIER

    if (res == 0)
        /* Pathname exists, no need to fix anything */
	return;

    while((pathstart = strchr(pathstart, '/')))
    {
	pathstart++;
	    
	pathend = strchr(pathstart, '/');
	if (pathend) *pathend = '\0';

	dirfound = TRUE;
	    
	res = iface->lstat(pathname, &st);
	AROS_HOST_BARRIER
	if (res != 0)
	{
	    dirfound = FALSE;

            pathstart[-1] = '\0';
	    dir = iface->opendir(pathname);
	    AROS_HOST_BARRIER
	    pathstart[-1] = '/';

	    if (dir)
	    {
		while(1)
		{
		    de = iface->readdir(dir);
		    AROS_HOST_BARRIER
		    if (!de)
		    	break;
		    
        	    if (strcasecmp(de->d_name, pathstart) == 0)
		    {
			dirfound = TRUE;
			strcpy(pathstart, de->d_name);
			break;
		    }
		}	    
		iface->closedir(dir);
		AROS_HOST_BARRIER

	    }
	} /* if (stat((const char *)pathname, &st) != 0) */
	    
	if (pathend) *pathend = '/';			    

	if (!dirfound) break;

    } /* while((pathpos = strchr(pathpos, '/))) */
}

#else

#define fixcase(iface, pathname)

#endif

/*-------------------------------------------------------------------------------------------*/

static int inline nocase_lstat(struct LibCInterface *iface, char *file_name, struct stat *st)
{
    int ret;

    fixcase(iface, file_name);
    ret = iface->lstat(file_name, st);
    AROS_HOST_BARRIER

    return ret;
}

/*-------------------------------------------------------------------------------------------*/

static inline int nocase_unlink(struct LibCInterface *iface, char *pathname)
{
    int ret;

    fixcase(iface, pathname);
    ret = iface->unlink((const char *)pathname);
    AROS_HOST_BARRIER
    
    return ret;
}

/*-------------------------------------------------------------------------------------------*/

static inline int nocase_mkdir(struct LibCInterface *iface, char *pathname, mode_t mode)
{
    int ret;

    fixcase(iface, pathname);
    ret = iface->mkdir(pathname, mode);
    AROS_HOST_BARRIER

    return ret;
}

/*-------------------------------------------------------------------------------------------*/

static inline int nocase_rmdir(struct LibCInterface *iface, char *pathname)
{
    int ret;

    fixcase(iface, pathname);
    ret = iface->rmdir(pathname);
    AROS_HOST_BARRIER
    
    return ret;
}

/*-------------------------------------------------------------------------------------------*/

static inline int nocase_link(struct LibCInterface *iface, char *oldpath, char *newpath)
{
    int ret;

    fixcase(iface, oldpath);
    fixcase(iface, newpath);

    ret = iface->link(oldpath, newpath);
    AROS_HOST_BARRIER

    return ret;
}

/*-------------------------------------------------------------------------------------------*/

static inline int nocase_symlink(struct LibCInterface *iface, char *oldpath, char *newpath)
{ 
    fixcase(iface, oldpath);
    fixcase(iface, newpath);

    return iface->symlink(oldpath, newpath);
}

/*-------------------------------------------------------------------------------------------*/

static inline int nocase_rename(struct LibCInterface *iface, char *oldpath, char *newpath)
{
    struct stat st;
    int ret;
    
    fixcase(iface, oldpath);
    fixcase(iface, newpath);

    /* AmigaDOS Rename does not allow overwriting */
    ret = iface->lstat(newpath, &st);
    AROS_HOST_BARRIER
    if (ret == 0)
    	return ERROR_OBJECT_EXISTS;

    ret = iface->rename(oldpath, newpath);
    AROS_HOST_BARRIER

    return ret;
}

/*-------------------------------------------------------------------------------------------*/

static inline int nocase_chmod(struct LibCInterface *iface, char *path, mode_t mode)
{
    int ret;

    fixcase(iface, path);

    ret = iface->chmod(path, mode);
    AROS_HOST_BARRIER
    
    return ret;
}

/*-------------------------------------------------------------------------------------------*/

static inline int nocase_readlink(struct LibCInterface *iface, char *path, char *buffer, size_t size)
{
    int ret;

    fixcase(iface, path);

    ret = iface->readlink(path, buffer, size);
    AROS_HOST_BARRIER
    
    return ret;
}

static inline int nocase_utime(struct LibCInterface *iface, char *path, const struct utimbuf *times)
{
    int ret;

    fixcase(iface, path);
    ret = iface->utime(path, times);
    AROS_HOST_BARRIER
    
    return ret;
}

/*-------------------------------------------------------------------------------------------*/

LONG DoOpen(struct emulbase *emulbase, struct filehandle *fh, LONG mode, LONG protect, BOOL AllowDir)
{
    struct stat st;
    LONG ret = ERROR_OBJECT_WRONG_TYPE;
    int r;
    long flags;

    DOPEN(bug("[emul] Opening host name: %s\n", fh->hostname));

    ObtainSemaphore(&emulbase->pdata.sem);

    r = nocase_lstat(emulbase->pdata.SysIFace, fh->hostname, &st);
    /* File name case is already adjusted here, so after this we can call UNIX functions directly */

    if (r == -1)
        /* Non-existing objects can be files opened for writing */
    	st.st_mode = S_IFREG;
    DOPEN(bug("[emul] lstat() returned %d, st_mode is 0x%08X\n", r, st.st_mode));

    if (S_ISREG(st.st_mode))
    {
	/* Object is a plain file */
	flags = mode2flags(mode);
	r = emulbase->pdata.SysIFace->open(fh->hostname, flags, 0770);
	AROS_HOST_BARRIER
	if (r < 0 && err_u2a(emulbase) == ERROR_WRITE_PROTECTED)
	{
	    /* Try again with read-only access. This is needed because AROS
	     * FS handlers should only pay attention to R/W protection flags
	     * when the corresponding operation is attempted on the file */
	    flags &= ~O_ACCMODE;
	    flags |= O_RDONLY;
	    r = emulbase->pdata.SysIFace->open(fh->hostname, flags, 0770);
	    AROS_HOST_BARRIER
	}
	if (r >= 0)
	{
	    fh->type = FHD_FILE;
	    fh->fd   = (void *)(IPTR)r;
	    ret = 0;
	}
	else
	    ret = err_u2a(emulbase);
    }

    if (AllowDir && S_ISDIR(st.st_mode))
    {
	/* Object is a directory */
	fh->fd = emulbase->pdata.SysIFace->opendir(fh->hostname);
	AROS_HOST_BARRIER

	if (fh->fd)
	{
	    fh->type = FHD_DIRECTORY;
	    ret = 0;
	}
	else
	    ret = err_u2a(emulbase);
    }

    if (S_ISLNK(st.st_mode))
    	/* Object is a softlink */
	ret = ERROR_IS_SOFT_LINK;

    ReleaseSemaphore(&emulbase->pdata.sem);

    return ret;
}

void DoClose(struct emulbase *emulbase, struct filehandle *current)
{
    ObtainSemaphore(&emulbase->pdata.sem);

    switch(current->type)
    {
    case FHD_FILE:
	/* Nothing will happen if type has FHD_STDIO set, this is intentional */
	emulbase->pdata.SysIFace->close((IPTR)current->fd);
	AROS_HOST_BARRIER
	break;

    case FHD_DIRECTORY:
    	emulbase->pdata.SysIFace->closedir(current->fd);
    	AROS_HOST_BARRIER
	break;
    }

    ReleaseSemaphore(&emulbase->pdata.sem);
}

LONG DoRead(struct emulbase *emulbase, struct IOFileSys *iofs, BOOL *async)
{
    struct filehandle *fh = (struct filehandle *)iofs->IOFS.io_Unit;
    int len;
    LONG error = 0;

    DREAD(bug("[emul] Reading %u bytes from fd %d\n", iofs->io_Union.io_READ.io_Length, (int)fh->fd));

    ObtainSemaphore(&emulbase->pdata.sem);

    len = TryRead(emulbase->pdata.SysIFace, (IPTR)fh->fd, iofs->io_Union.io_READ.io_Buffer, iofs->io_Union.io_READ.io_Length);
    DREAD(bug("[emul] Result: %d\n", len));
    if (len == -1)
	error = err_u2a(emulbase);

    ReleaseSemaphore(&emulbase->pdata.sem);

    if (len == -2)
    {
	/* There was no data available, perform an asynchronous read */
	DREAD(bug("[emul] Putting request 0x%P to asynchronous queue\n", iofs));

	Disable();
	AddTail((struct List *)&emulbase->pdata.readList, (struct Node *)iofs);
	Enable();

	ObtainSemaphore(&emulbase->pdata.sem);

	/* Own the filedescriptor and enable SIGIO on it */
	emulbase->pdata.SysIFace->fcntl((IPTR)fh->fd, F_SETOWN, emulbase->pdata.my_pid);
	AROS_HOST_BARRIER
	len = emulbase->pdata.SysIFace->fcntl((IPTR)fh->fd, F_GETFL);
	AROS_HOST_BARRIER
	len |= O_ASYNC;
	emulbase->pdata.SysIFace->fcntl((IPTR)fh->fd, F_SETFL, len);
	AROS_HOST_BARRIER

	/* Kick processing loop once because SIGIO could arrive after read attempt
	   but before we added the request to the queue. */
	emulbase->pdata.SysIFace->kill(emulbase->pdata.my_pid, SIGIO);
	AROS_HOST_BARRIER

	ReleaseSemaphore(&emulbase->pdata.sem);
	*async = TRUE;
	return 0;
    }
	
    if (!error)
	iofs->io_Union.io_READ.io_Length = len;
    return error;
}

LONG DoWrite(struct emulbase *emulbase, struct IOFileSys *iofs, BOOL *async)
{
    /* Our write routine is always synchronous */
    struct filehandle *fh = (struct filehandle *)iofs->IOFS.io_Unit;
    int len;
    LONG error = 0;
    
    ObtainSemaphore(&emulbase->pdata.sem);

    len = emulbase->pdata.SysIFace->write((IPTR)fh->fd, iofs->io_Union.io_READ.io_Buffer, iofs->io_Union.io_READ.io_Length);
    AROS_HOST_BARRIER
    if (len == -1)
	error = err_u2a(emulbase);

    ReleaseSemaphore(&emulbase->pdata.sem);
 
    if (!error)
    	iofs->io_Union.io_READ.io_Length = len;
    return error;
}

LONG DoSeek(struct emulbase *emulbase, void *file, UQUAD *Offset, ULONG mode)
{
    off_t res;
    off_t oldpos = 0;
    LONG error = 0;

    /* kprintf() does not understand UQUAD values */
    DSEEK(bug("[emul] DoSeek(%d, 0x%08X%08X, %d)\n", (int)file, (ULONG)(*Offset >> 32), (ULONG)*Offset, mode));

    switch (mode) {
    case OFFSET_BEGINNING:
	mode = SEEK_SET;
	break;

    case OFFSET_CURRENT:
	mode = SEEK_CUR;
	break;

    default:
	mode = SEEK_END;
    }

    ObtainSemaphore(&emulbase->pdata.sem);

    res = LSeek((IPTR)file, 0, SEEK_CUR);
    AROS_HOST_BARRIER

    DSEEK(bug("[emul] Original position: 0x%08X%08X\n", (ULONG)(res >> 32), (ULONG)res));
    if (res != -1)
    {
        UQUAD offs = *Offset;

        oldpos = res;
        res = LSeek((IPTR)file, offs, mode);
        AROS_HOST_BARRIER

	DSEEK(bug("[emul] New position: 0x%08X%08X\n", (ULONG)(res >> 32), (ULONG)res));
    }

    if (res == -1)
	error = err_u2a(emulbase);

    ReleaseSemaphore(&emulbase->pdata.sem);

    if (!error)
	*Offset = oldpos;
    return error;
}

LONG DoMkDir(struct emulbase *emulbase, struct filehandle *fh, ULONG protect)
{
    LONG ret;

    protect = prot_a2u(protect);

    ObtainSemaphore(&emulbase->pdata.sem);

    ret = nocase_mkdir(emulbase->pdata.SysIFace, fh->hostname, protect);
    if (!ret)
    {
	fh->type = FHD_DIRECTORY;
	fh->fd   = emulbase->pdata.SysIFace->opendir(fh->hostname);
	AROS_HOST_BARRIER
    }

    if ((ret == -1) || (fh->fd == NULL))
        ret = err_u2a(emulbase);

    ReleaseSemaphore(&emulbase->pdata.sem);

    return ret;
}

LONG DoDelete(struct emulbase *emulbase, char *name)
{
    LONG ret;
    struct stat st;

    ObtainSemaphore(&emulbase->pdata.sem);

    ret = nocase_lstat(emulbase->pdata.SysIFace, name, &st);

    if (!ret)
    {
        if (S_ISDIR(st.st_mode))
        {
	    ret = emulbase->pdata.SysIFace->rmdir(name);
	    AROS_HOST_BARRIER
	}
    	else
    	{
	    ret = emulbase->pdata.SysIFace->unlink(name);
	    AROS_HOST_BARRIER
	}
    }

    if (ret)
	ret = err_u2a(emulbase);

    ReleaseSemaphore(&emulbase->pdata.sem);

    return ret;
}

LONG DoChMod(struct emulbase *emulbase, char *filename, ULONG prot)
{
    LONG ret;
    
    ObtainSemaphore(&emulbase->pdata.sem);

    ret = nocase_chmod(emulbase->pdata.SysIFace, filename, prot_a2u(prot));
    if (ret)
        ret = err_u2a(emulbase);

    ReleaseSemaphore(&emulbase->pdata.sem);
    
    return ret;
}

LONG DoHardLink(struct emulbase *emulbase, char *fn, char *oldfile)
{
    LONG error;

    ObtainSemaphore(&emulbase->pdata.sem);

    error = nocase_link(emulbase->pdata.SysIFace, oldfile, fn);
    if (error)
        error = err_u2a(emulbase);

    ReleaseSemaphore(&emulbase->pdata.sem);

    return error;
}

LONG DoSymLink(struct emulbase *emulbase, char *dest, char *src)
{
    LONG error;

    ObtainSemaphore(&emulbase->pdata.sem);

    error = nocase_symlink(emulbase->pdata.SysIFace, dest, src);
    if (error)
        error = err_u2a(emulbase);

    ReleaseSemaphore(&emulbase->pdata.sem);

    return error;
}

int DoReadLink(struct emulbase *emulbase, char *filename, char *buffer, ULONG size, LONG *err)
{
    int res;

    ObtainSemaphore(&emulbase->pdata.sem);

    res = nocase_readlink(emulbase->pdata.SysIFace, filename, buffer, size);
    if (res == -1)
        *err = err_u2a(emulbase);
    else if (res == size)
        /* Buffer was too small */
        res = -2;

    ReleaseSemaphore(&emulbase->pdata.sem);

    return res;
}

LONG DoRename(struct emulbase *emulbase, char *filename, char *newfilename)
{
    LONG error;

    ObtainSemaphore(&emulbase->pdata.sem);

    error = nocase_rename(emulbase->pdata.SysIFace, filename, newfilename);
    if (error)
	error = err_u2a(emulbase);

    ReleaseSemaphore(&emulbase->pdata.sem);

    return error;
}

LONG DoSetDate(struct emulbase *emulbase, char *name, struct DateStamp *date)
{
    struct utimbuf times;
    LONG res;

    ObtainSemaphore(&emulbase->pdata.sem);

    times.actime = datestamp2timestamp(emulbase->pdata.SysIFace, date);
    times.modtime = times.actime;

    res = nocase_utime(emulbase->pdata.SysIFace, name, &times);
    if (res < 0)
        res = err_u2a(emulbase);

    ReleaseSemaphore(&emulbase->pdata.sem);

    return res;
}

LONG DoSetSize(struct emulbase *emulbase, struct filehandle *fh, struct IFS_SEEK *io_SEEK)
{
    off_t absolute = 0;
    LONG err = 0;

    ObtainSemaphore(&emulbase->pdata.sem);

    switch (io_SEEK->io_SeekMode) {
    case OFFSET_BEGINNING:
        absolute = io_SEEK->io_Offset;
        break;

    case OFFSET_CURRENT:
       	absolute = LSeek((IPTR)fh->fd, 0, SEEK_CUR);
       	AROS_HOST_BARRIER

        if (absolute == -1)
            err = err_u2a(emulbase);
	else
            absolute += io_SEEK->io_Offset;
        break;

    case OFFSET_END:
        absolute = LSeek((IPTR)fh->fd, 0, SEEK_END); 
        AROS_HOST_BARRIER

        if (absolute == -1)
            err = err_u2a(emulbase);
	else
            absolute -= io_SEEK->io_Offset;
        break;

    default:
    	err = ERROR_UNKNOWN;
    }

    if (!err)
    {
	err = FTruncate((IPTR)fh->fd, absolute);
	AROS_HOST_BARRIER
	if (err)
	    err = err_u2a(emulbase);
    }
       
    ReleaseSemaphore(&emulbase->pdata.sem);

    if (!err)
        io_SEEK->io_Offset = absolute;
    return err;
}

BOOL DoGetType(struct emulbase *emulbase, void *fd)
{
    int ret;

    ObtainSemaphore(&emulbase->pdata.sem);
    
    ret = emulbase->pdata.SysIFace->isatty((IPTR)fd);
    AROS_HOST_BARRIER

    ReleaseSemaphore(&emulbase->pdata.sem);

    return ret;
}

LONG DoStatFS(struct emulbase *emulbase, char *path, struct InfoData *id)
{
    struct statfs buf;
    LONG err;
    
    ObtainSemaphore(&emulbase->pdata.sem);

    err = emulbase->pdata.SysIFace->statfs(path, &buf);
    AROS_HOST_BARRIER
    if (err)
    	err = err_u2a(emulbase);

    ReleaseSemaphore(&emulbase->pdata.sem);

    if (!err)
    {
    	id->id_NumSoftErrors = 0;
    	id->id_DiskState = ID_VALIDATED;
    	id->id_NumBlocks = buf.f_blocks;
    	id->id_NumBlocksUsed = buf.f_blocks - buf.f_bavail;
    	id->id_BytesPerBlock = buf.f_bsize;
    }

    return err;
}

LONG DoRewindDir(struct emulbase *emulbase, struct filehandle *fh)
{
    ObtainSemaphore(&emulbase->pdata.sem);

    emulbase->pdata.SysIFace->rewinddir(fh->fd);
    AROS_HOST_BARRIER

    ReleaseSemaphore(&emulbase->pdata.sem);

    /* Directory search position has been reset */
    fh->ph.dirpos = 0;

    /* rewinddir() never fails */
    return 0;
}

static LONG stat_entry(struct emulbase *emulbase, struct filehandle *fh, STRPTR FoundName, struct stat *st)
{
    STRPTR filename, name;
    ULONG plen, flen;
    LONG err = 0;

    DEXAM(bug("[emul] stat_entry(): filehandle's path: %s\n", fh->hostname));
    if (FoundName)
    {
	DEXAM(bug("[emul] ...containing object: %s\n", FoundName));
	plen = strlen(fh->hostname);
	flen = strlen(FoundName);
	name = AllocVecPooled(emulbase->mempool, plen + flen + 2);
	if (NULL == name)
	    return ERROR_NO_FREE_STORE;

	strcpy(name, fh->hostname);
	filename = name + plen;
	*filename++ = '/';
	strcpy(filename, FoundName);
    } else
	name = fh->hostname;
  
    DEXAM(bug("[emul] Full name: %s\n", name));

    ObtainSemaphore(&emulbase->pdata.sem);

    err = emulbase->pdata.SysIFace->lstat(name, st);
    AROS_HOST_BARRIER
    if (err)
	err = err_u2a(emulbase);

    ReleaseSemaphore(&emulbase->pdata.sem);

    if (FoundName)
    {
	DEXAM(bug("[emul] Freeing full name\n"));
	FreeVecPooled(emulbase->mempool, name);
    }
    return err;
}	

LONG examine_entry(struct emulbase *emulbase, struct filehandle *fh, char *EntryName,
		   struct ExAllData *ead, ULONG size, ULONG type)
{
    STRPTR next, end, last, name;
    struct stat st;
    LONG err;

    DEXAM(bug("[emul] examine_entry(0x%P, %s, 0x%P, %u, %u)\n", fh, EntryName, ead, size, type));

    /* Return an error, if supplied type is not supported. */
    if(type>ED_OWNER)
	return ERROR_BAD_NUMBER;

    /* Check, if the supplied buffer is large enough. */
    next=(STRPTR)ead+sizes[type];
    end =(STRPTR)ead+size;
    DEXAM(bug("[emul] ead 0x%P, next 0x%P, end 0x%P\n", ead, next, end));

    if(next>end) /* > is correct. Not >= */
	return ERROR_BUFFER_OVERFLOW;

    err = stat_entry(emulbase, fh, EntryName, &st);
    if (err)
    	return err;

    DEXAM(bug("[emul] File mode: %o\n", st.st_mode));
    DEXAM(bug("[emul] File size: %u\n", st.st_size));
    DEXAM(bug("[emul] Filling in information\n"));
    DEXAM(bug("[emul] ead 0x%P, next 0x%P, end 0x%P, size %u, type %u\n", ead, next, end, size, type));

    switch(type)
    {
	default:
	case ED_OWNER:
	    ead->ed_OwnerUID	= st.st_uid;
	    ead->ed_OwnerGID	= st.st_gid;
	case ED_COMMENT:
	    ead->ed_Comment=NULL;
	case ED_DATE:
	{
	    struct DateStamp stamp;

	    timestamp2datestamp(emulbase, &st.st_mtime, &stamp);
	    ead->ed_Days	= stamp.ds_Days;
	    ead->ed_Mins	= stamp.ds_Minute;
	    ead->ed_Ticks	= stamp.ds_Tick;
	}
	case ED_PROTECTION:
	    ead->ed_Prot 	= prot_u2a(st.st_mode);
	case ED_SIZE:
	    ead->ed_Size	= st.st_size;
	case ED_TYPE:
            if (S_ISDIR(st.st_mode)) {
		if (EntryName || fh->name[0])
		    ead->ed_Type = ST_USERDIR;
		else
		    ead->ed_Type = ST_ROOT;
	    } else if (S_ISLNK(st.st_mode))
		ead->ed_Type = ST_SOFTLINK;
	    else
	        ead->ed_Type = ST_FILE;
	    
	case ED_NAME:
	    if (EntryName)
		last = EntryName;
	    else if (*fh->name) {
	        name = fh->name;
	        last = name;
	        while(*name) {
		    if(*name++ == '/')
			last = name;
		}
	    } else
	        last = fh->volumename;

	    ead->ed_Name=next;
	    for(;;)
	    {
		if(next>=end)
		    return ERROR_BUFFER_OVERFLOW;
		if(!(*next++=*last++))
		    break;
	    }
	case 0:
	    ead->ed_Next=(struct ExAllData *)(((IPTR)next+AROS_PTRALIGN-1)&~(AROS_PTRALIGN-1));
	    return 0;
    }
}

/*********************************************************************************************/

LONG examine_next(struct emulbase *emulbase, struct filehandle *fh,
                  struct FileInfoBlock *FIB)
{
    int	i;
    struct stat st;
    struct dirent *dir;
    char *src, *dest;
    LONG err;

    /* This operation does not make any sense on a file */
    if (fh->type != FHD_DIRECTORY)
    	return ERROR_OBJECT_WRONG_TYPE;

    ObtainSemaphore(&emulbase->pdata.sem);

    /*
     * First of all we have to go to the position where Examine() or
     * ExNext() stopped the previous time so we can read the next entry!
     * On Android this is handled by ReadDir() artificially tracking
     * current search position in the filehandle.
     */
#ifndef HOST_OS_android
    emulbase->pdata.SysIFace->seekdir(fh->fd, FIB->fib_DiskKey);
    AROS_HOST_BARRIER
#endif

    /* hm, let's read the data now! */
    dir = ReadDir(emulbase, fh, &FIB->fib_DiskKey);

    ReleaseSemaphore(&emulbase->pdata.sem);

    if (!dir)
    	return ERROR_NO_MORE_ENTRIES;

    err = stat_entry(emulbase, fh, dir->d_name, &st);
    if (err) {
        DEXAM(bug("stat_entry() failed for %s\n", dir->d_name));
        return err;
    }

    FIB->fib_OwnerUID	= st.st_uid;
    FIB->fib_OwnerGID	= st.st_gid;
    FIB->fib_Comment[0]	= '\0'; /* no comments available yet! */
    timestamp2datestamp(emulbase, &st.st_mtime, &FIB->fib_Date);
    FIB->fib_Protection	= prot_u2a(st.st_mode);
    FIB->fib_Size       = st.st_size;

    if (S_ISDIR(st.st_mode))
    {
	FIB->fib_DirEntryType = ST_USERDIR; /* S_ISDIR(st.st_mode)?(*fh->name?ST_USERDIR:ST_ROOT):0*/
    }
    else if(S_ISLNK(st.st_mode))
    {
	FIB->fib_DirEntryType = ST_SOFTLINK;
    }
    else
    {
	FIB->fib_DirEntryType = ST_FILE;
    }
    
    /* fast copying of the filename */
    src  = dir->d_name;
    dest = FIB->fib_FileName;

    for (i =0; i<MAXFILENAMELENGTH-1;i++)
    {
	if(! (*dest++=*src++) )
	{
	    break;
	}
    }

    return 0;
}

/*********************************************************************************************/

LONG examine_all(struct emulbase *emulbase,
			struct filehandle *fh,
                        struct ExAllData *ead,
                        struct ExAllControl *eac,
                        ULONG  size,
                        ULONG  type)
{
    struct ExAllData *last=NULL;
    STRPTR end=(STRPTR)ead+size;
    struct dirent *dir;
    LONG error;
#ifndef HOST_OS_android
    off_t oldpos;
#endif

    eac->eac_Entries = 0;
    if(fh->type!=FHD_DIRECTORY)
	return ERROR_OBJECT_WRONG_TYPE;

    DEXAM(bug("[emul] examine_all()\n"));

    for(;;)
    {
	ObtainSemaphore(&emulbase->pdata.sem);

#ifndef HOST_OS_android
	oldpos = emulbase->pdata.SysIFace->telldir(fh->fd);
	AROS_HOST_BARRIER
#endif

        *emulbase->pdata.errnoPtr = 0;
	dir = ReadDir(emulbase, fh, &eac->eac_LastKey);

	if (!dir)
	    error = err_u2a(emulbase);

	ReleaseSemaphore(&emulbase->pdata.sem);

	if (!dir)
	    break;

	DEXAM(bug("[emul] Found entry %s\n", dir->d_name));

	if (eac->eac_MatchString && !MatchPatternNoCase(eac->eac_MatchString, dir->d_name)) {
	    DEXAM(bug("[emul] Entry does not match, skipping\n"));
	    continue;
	}

	error = examine_entry(emulbase, fh, dir->d_name, ead, end-(STRPTR)ead, type);
	if(error)
	    break;

	if ((eac->eac_MatchFunc) && !CALLHOOKPKT(eac->eac_MatchFunc, ead, &type))
	  continue;

	eac->eac_Entries++;
	last=ead;
	ead=ead->ed_Next;
    }
    if (last!=NULL)
	last->ed_Next=NULL;

    if ((error==ERROR_BUFFER_OVERFLOW) && last)
    {
#ifdef HOST_OS_android
	eac->eac_LastKey--;
#else
	ObtainSemaphore(&emulbase->pdata.sem);

	emulbase->pdata.SysIFace->seekdir(fh->fd, oldpos);
	AROS_HOST_BARRIER

	ReleaseSemaphore(&emulbase->pdata.sem);
#endif
	/* Examination will continue from the current position */
	return 0;
    }

    if(!error)
	error = ERROR_NO_MORE_ENTRIES;
    /* Reading the whole directory has been completed, so reset position */
    DoRewindDir(emulbase, fh);

    return error;
}

char *GetHomeDir(struct emulbase *emulbase, char *sp)
{
    char *home = NULL;
    char *newunixpath = NULL;
    char *sp_end;
#ifndef HOST_OS_android
    BOOL do_endpwent = FALSE;
#endif

    ObtainSemaphore(&emulbase->pdata.sem);

    /* "~<name>" means home of user <name> */
    if ((sp[1] == '\0') || (sp[1] == '/'))
    {
    	sp_end = sp + 1;
	home = emulbase->pdata.SysIFace->getenv("HOME");
	AROS_HOST_BARRIER
    }
#ifndef HOST_OS_android
    else
    {
    	struct passwd *pwd;
	WORD  	       cmplen;
		
	for(sp_end = sp + 1; sp_end[0] != '\0' && sp_end[0] != '/'; sp_end++);
	cmplen = sp_end - sp - 1;

	while(1)
	{
	    pwd = emulbase->pdata.SysIFace->getpwent();
	    AROS_HOST_BARRIER

	    if (!pwd)
	    	break;

	    if(memcmp(pwd->pw_name, sp + 1, cmplen) == 0)
	    {
	    	if (pwd->pw_name[cmplen] == '\0')
		{
	    	    home = pwd->pw_dir;
		    break;
		}
	    }
	}
	do_endpwent = TRUE;
    }
#endif

    if (home)
    {
	int hlen = strlen(home);
	int splen = strlen(sp_end);

	newunixpath = AllocVec(hlen + splen + 1, MEMF_PUBLIC);
	if (newunixpath)
	{
	    char *s = newunixpath;

	    CopyMem(home, s, hlen);
	    s += hlen;
	    CopyMem(sp_end, s, splen);
	    s += splen;
	    *s = 0;
	}
    }

#ifndef HOST_OS_android
    if (do_endpwent)
    {
	emulbase->pdata.SysIFace->endpwent();
	AROS_HOST_BARRIER
    }
#endif

    ReleaseSemaphore(&emulbase->pdata.sem);

    return newunixpath;
}

ULONG GetCurrentDir(struct emulbase *emulbase, char *path, ULONG len)
{
    char *res;

    DMOUNT(bug("[emul] GetCurrentDir(0x%P, %u)\n", path, len)); 

    ObtainSemaphore(&emulbase->pdata.sem);

    res = emulbase->pdata.SysIFace->getcwd(path, len);
    AROS_HOST_BARRIER

    ReleaseSemaphore(&emulbase->pdata.sem);

    DMOUNT(bug("[emul] getcwd() returned %s\n", res));
    return res ? TRUE : FALSE;
}

int CheckDir(struct emulbase *emulbase, char *path)
{
    int res;
    struct stat st;

    DMOUNT(bug("[emul] CheckDir(%s)\n", path));

    ObtainSemaphore(&emulbase->pdata.sem);

    res = emulbase->pdata.SysIFace->stat(path, &st);
    AROS_HOST_BARRIER

    ReleaseSemaphore(&emulbase->pdata.sem);

    DMOUNT(bug("[emul] Result: %d, mode: %o\n", res, st.st_mode));
    if ((!res) && S_ISDIR(st.st_mode))
	return 0;

    return -1;
}
