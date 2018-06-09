/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "unix_hints.h"

#ifdef HOST_LONG_ALIGNED
#pragma pack(4)
#endif

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

#pragma pack()

/* This prevents redefinition of struct timeval */
#define _AROS_TYPES_TIMEVAL_S_H_

#define DEBUG 0
#define DASYNC(x)
#define DEXAM(x)
#define DMOUNT(x)
#define DOPEN(x)
#define DREAD(x)
#define DWRITE(x)
#define DSEEK(x)

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <dos/dosasl.h>
#include <hidd/unixio.h>
#include <utility/date.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/hostlib.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include "emul_intern.h"
#include "emul_unix.h"

#define NO_CASE_SENSITIVITY

struct dirent *ReadDir(struct emulbase *emulbase, struct filehandle *fh, IPTR *dirpos);

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

static LONG errno_u2a(int err)
{
    ULONG i;

    for (i = 0; i < sizeof(u2a)/sizeof(u2a[0]); i++)
    {
	if (u2a[i][0] == err)
	    return u2a[i][1];
    }

    return ERROR_UNKNOWN;
}

static inline LONG err_u2a(struct emulbase *emulbase)
{
    return errno_u2a(*emulbase->pdata.errnoPtr);
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

static void timestamp2datestamp(struct emulbase *emulbase, time_t *timestamp, struct DateStamp *datestamp)
{
    struct ClockData date;
    struct tm *tm;

    HostLib_Lock();

    tm = emulbase->pdata.SysIFace->localtime(timestamp);
    AROS_HOST_BARRIER
    
    HostLib_Unlock();

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

static time_t datestamp2timestamp(struct emulbase *emulbase, struct DateStamp *datestamp)
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
    
    ret = emulbase->pdata.SysIFace->mktime(&tm);
    AROS_HOST_BARRIER

    return ret;
}

/*********************************************************************************************/

#ifdef NO_CASE_SENSITIVITY

static void fixcase(struct emulbase *emulbase, char *pathname)
{
    struct LibCInterface *iface = emulbase->pdata.SysIFace;
    struct dirent 	*de;
    struct stat	st;
    DIR			*dir;
    char		*pathstart, *pathend;
    BOOL		dirfound;
    int			res;

    pathstart = pathname;

    res = emulbase->pdata.SysIFace->lstat((const char *)pathname, &st);
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
	    
	res = emulbase->pdata.SysIFace->lstat(pathname, &st);
	AROS_HOST_BARRIER
	if (res != 0)
	{
	    dirfound = FALSE;

            pathstart[-1] = '\0';
	    dir = emulbase->pdata.SysIFace->opendir(pathname);
	    AROS_HOST_BARRIER
	    pathstart[-1] = '/';

	    if (dir)
	    {
		while(1)
		{
		    de = emulbase->pdata.SysIFace->readdir(dir);
		    AROS_HOST_BARRIER
		    if (!de)
		    	break;
		    
        	    if (Stricmp(de->d_name, pathstart) == 0)
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

#define fixcase(emulbase, pathname)

#endif

/*-------------------------------------------------------------------------------------------*/

static int inline nocase_lstat(struct emulbase *emulbase, char *file_name, struct stat *st)
{
    int ret;

    fixcase(emulbase, file_name);
    ret = emulbase->pdata.SysIFace->lstat(file_name, st);
    AROS_HOST_BARRIER

    return ret;
}

/*-------------------------------------------------------------------------------------------*/

static inline int nocase_unlink(struct emulbase *emulbase, char *pathname)
{
    int ret;

    fixcase(emulbase, pathname);
    ret = emulbase->pdata.SysIFace->unlink((const char *)pathname);
    AROS_HOST_BARRIER
    
    return ret;
}

/*-------------------------------------------------------------------------------------------*/

static inline int nocase_mkdir(struct emulbase *emulbase, char *pathname, mode_t mode)
{
    int ret;

    fixcase(emulbase, pathname);
    ret = emulbase->pdata.SysIFace->mkdir(pathname, mode);
    AROS_HOST_BARRIER

    return ret;
}

/*-------------------------------------------------------------------------------------------*/

static inline int nocase_rmdir(struct emulbase *emulbase, char *pathname)
{
    int ret;

    fixcase(emulbase, pathname);
    ret = emulbase->pdata.SysIFace->rmdir(pathname);
    AROS_HOST_BARRIER
    
    return ret;
}

/*-------------------------------------------------------------------------------------------*/

static inline int nocase_link(struct emulbase *emulbase, char *oldpath, char *newpath)
{
    int ret;

    fixcase(emulbase, oldpath);
    fixcase(emulbase, newpath);

    ret = emulbase->pdata.SysIFace->link(oldpath, newpath);
    AROS_HOST_BARRIER

    return ret;
}

/*-------------------------------------------------------------------------------------------*/

static inline int nocase_symlink(struct emulbase *emulbase, char *oldpath, char *newpath)
{ 
    fixcase(emulbase, oldpath);
    fixcase(emulbase, newpath);

    return emulbase->pdata.SysIFace->symlink(oldpath, newpath);
}

/*-------------------------------------------------------------------------------------------*/

static inline int nocase_rename(struct emulbase *emulbase, char *oldpath, char *newpath)
{
    struct stat st;
    int ret;
    
    fixcase(emulbase, oldpath);
    fixcase(emulbase, newpath);

    /* AmigaDOS Rename does not allow overwriting */
    ret = emulbase->pdata.SysIFace->lstat(newpath, &st);
    AROS_HOST_BARRIER
    if (ret == 0)
    	return ERROR_OBJECT_EXISTS;

    ret = emulbase->pdata.SysIFace->rename(oldpath, newpath);
    AROS_HOST_BARRIER

    return ret;
}

/*-------------------------------------------------------------------------------------------*/

static inline int nocase_chmod(struct emulbase *emulbase, char *path, mode_t mode)
{
    int ret;

    fixcase(emulbase, path);

    ret = emulbase->pdata.SysIFace->chmod(path, mode);
    AROS_HOST_BARRIER
    
    return ret;
}

/*-------------------------------------------------------------------------------------------*/

static inline int nocase_readlink(struct emulbase *emulbase, char *path, char *buffer, SIPTR size)
{
    int ret;

    fixcase(emulbase, path);

    ret = emulbase->pdata.SysIFace->readlink(path, buffer, size);
    AROS_HOST_BARRIER
    
    return ret;
}

static inline int nocase_utime(struct emulbase *emulbase, char *path, const struct utimbuf *times)
{
    int ret;

    fixcase(emulbase, path);
    ret = emulbase->pdata.SysIFace->utime(path, times);
    AROS_HOST_BARRIER
    
    return ret;
}

/*-------------------------------------------------------------------------------------------*/

LONG DoOpen(struct emulbase *emulbase, struct filehandle *fh, LONG access, LONG mode, LONG protect, BOOL AllowDir)
{
    struct stat st;
    LONG ret = ERROR_OBJECT_WRONG_TYPE;
    int r;

    DOPEN(bug("[emul] Opening host name: %s\n", fh->hostname));

    HostLib_Lock();

    r = nocase_lstat(emulbase, fh->hostname, &st);
    /* File name case is already adjusted here, so after this we can call UNIX functions directly */

    if (r == -1)
    {
        /* Non-existing objects can be files opened for writing */
    	st.st_mode = S_IFREG;
    }

    DOPEN(bug("[emul] lstat() returned %d, st_mode is 0x%08X\n", r, st.st_mode));

    if (S_ISREG(st.st_mode))
    {
        /* Object is a plain file */
        int flags = O_RDONLY;
        if (access != ACCESS_READ)
            flags = O_RDWR;

        switch (mode)
        {
        case MODE_NEWFILE:
            flags |= O_TRUNC;
            /* Fallthrough */

        case MODE_READWRITE:
            flags |= O_CREAT;
        }

	r = emulbase->pdata.SysIFace->open(fh->hostname, flags, 0770);
	AROS_HOST_BARRIER

	if (r < 0 && err_u2a(emulbase) == ERROR_WRITE_PROTECTED)
	{
	    /* Try again with read-only access. This is needed because AROS
	     * FS handlers should only pay attention to R/W protection flags
	     * when the corresponding operation is attempted on the file */
	    r = emulbase->pdata.SysIFace->open(fh->hostname, O_RDONLY, 0770);
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
#ifndef HOST_OS_android
        if (fh->fd != NULL)
            fh->ph.dirpos_first = emulbase->pdata.SysIFace->telldir(fh->fd);
#endif
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

    HostLib_Unlock();

    return ret;
}

void DoClose(struct emulbase *emulbase, struct filehandle *current)
{
    HostLib_Lock();

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

    HostLib_Unlock();
}

LONG DoRead(struct emulbase *emulbase, struct filehandle *fh, APTR buff, ULONG len, SIPTR *err)
{
    SIPTR error;
    int res;

    DREAD(bug("[emul] Reading %u bytes from fd %ld\n", len, fh->fd));

    /* Wait until the fd is ready to read. We reuse UnixIO capabilities for this. */
    error = Hidd_UnixIO_Wait(emulbase->pdata.unixio, (long)fh->fd, vHidd_UnixIO_Read);
    if (error)
    {
    	*err = errno_u2a(error);
    	return -1;
    }

    HostLib_Lock();

    DREAD(bug("[emul] FD %ld ready for read\n", fh->fd));

    if (fh->type & FHD_STDIO)
    {
	int res2;
	struct pollfd pfd = {(long)fh->fd, POLLIN, 0};

	/*
	 * When reading from stdin, we have to read character-by-character until
	 * we read as many characters as we wanted, or there's nothing more to read.
	 * Without this read() can return an error. For example this happens on Darwin
	 * when the shell requests a single read of 208 bytes.
	 */
	res = 0;
	do
	{
	    res2 = emulbase->pdata.SysIFace->read((long)fh->fd, buff++, 1);
	    AROS_HOST_BARRIER

	    if (res2 == -1)
	        break;

	    if (res++ == len)
	        break;

    	    res2 = emulbase->pdata.SysIFace->poll(&pfd, 1, 0);
	    AROS_HOST_BARRIER

	} while (res2 > 0);

	if (res2 == -1)
	    res = -1;
    }
    else
    {
	/* It's not stdin. Read as much as we need to. */
	res = emulbase->pdata.SysIFace->read((long)fh->fd, buff, len);
	AROS_HOST_BARRIER
    }

    if (res == -1)
	error = err_u2a(emulbase);

    HostLib_Unlock();

    DREAD(bug("[emul] Result %d, error %ld\n", len, error));

    *err = error;
    return res;
}

LONG DoWrite(struct emulbase *emulbase, struct filehandle *fh, CONST_APTR buff, ULONG len, SIPTR *err)
{
    SIPTR error = 0;

    DWRITE(bug("[emul] Writing %u bytes to fd %ld\n", len, fh->fd));

    HostLib_Lock();

    len = emulbase->pdata.SysIFace->write((IPTR)fh->fd, buff, len);
    AROS_HOST_BARRIER
    if (len == -1)
	error = err_u2a(emulbase);

    HostLib_Unlock();

    *err = error;
    return len;
}

SIPTR DoSeek(struct emulbase *emulbase, struct filehandle *fh, SIPTR offset, ULONG mode, SIPTR *err)
{
    off_t res;
    LONG oldpos = 0, newpos;
    struct stat st;

    DSEEK(bug("[emul] DoSeek(%d, %d, %d)\n", (int)fh->fd, offset, mode));

    HostLib_Lock();

    res = oldpos = LSeek((IPTR)fh->fd, 0, SEEK_CUR);
    AROS_HOST_BARRIER
    if (res != -1)
	res = emulbase->pdata.SysIFace->fstat((int)(IPTR)fh->fd, &st);
    AROS_HOST_BARRIER

    DSEEK(bug("[emul] Original position: %lu\n", (unsigned long)oldpos));

    if (res != -1)
    {
	switch (mode) {
	case OFFSET_BEGINNING:
	    newpos = offset;
	    mode = SEEK_SET;
	    break;

	case OFFSET_CURRENT:
	    newpos = offset + res;
	    mode = SEEK_CUR;
	    break;

	default:
	    newpos = offset + st.st_size;
	    mode = SEEK_END;
	}

	if (newpos > st.st_size)
	    res = -1;
    }

    if (res != -1)
    {
        res = LSeek((IPTR)fh->fd, offset, mode);
        AROS_HOST_BARRIER

	DSEEK(bug("[emul] New position: %lu\n", (unsigned long)res));
    }

    if (res == -1)
    	oldpos = -1;

    HostLib_Unlock();

    *err = ERROR_SEEK_ERROR;
    return oldpos;
}

LONG DoMkDir(struct emulbase *emulbase, struct filehandle *fh, ULONG protect)
{
    LONG ret;

    protect = prot_a2u(protect);

    HostLib_Lock();

    ret = nocase_mkdir(emulbase, fh->hostname, protect);
    if (!ret)
    {
	fh->type = FHD_DIRECTORY;
	fh->fd   = emulbase->pdata.SysIFace->opendir(fh->hostname);
#ifndef HOST_OS_android
        fh->ph.dirpos_first = emulbase->pdata.SysIFace->telldir(fh->fd);
#endif
	AROS_HOST_BARRIER
    }

    if ((ret == -1) || (fh->fd == NULL))
        ret = err_u2a(emulbase);

    HostLib_Unlock();

    return ret;
}

LONG DoDelete(struct emulbase *emulbase, char *name)
{
    LONG ret;
    struct stat st;

    HostLib_Lock();

    ret = nocase_lstat(emulbase, name, &st);

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

    HostLib_Unlock();

    return ret;
}

LONG DoChMod(struct emulbase *emulbase, char *filename, ULONG prot)
{
    LONG ret;
    
    HostLib_Lock();

    ret = nocase_chmod(emulbase, filename, prot_a2u(prot));
    if (ret)
        ret = err_u2a(emulbase);

    HostLib_Unlock();
    
    return ret;
}

LONG DoHardLink(struct emulbase *emulbase, char *fn, char *oldfile)
{
    LONG error;

    HostLib_Lock();

    error = nocase_link(emulbase, oldfile, fn);
    if (error)
        error = err_u2a(emulbase);

    HostLib_Unlock();

    return error;
}

LONG DoSymLink(struct emulbase *emulbase, char *dest, char *src)
{
    LONG error;

    HostLib_Lock();

    error = nocase_symlink(emulbase, dest, src);
    if (error)
        error = err_u2a(emulbase);

    HostLib_Unlock();

    return error;
}

int DoReadLink(struct emulbase *emulbase, char *filename, char *buffer, ULONG size, LONG *err)
{
    int res;

    HostLib_Lock();

    res = nocase_readlink(emulbase, filename, buffer, size);
    if (res == -1)
        *err = err_u2a(emulbase);
    else if (res == size)
        /* Buffer was too small */
        res = -2;

    HostLib_Unlock();

    return res;
}

LONG DoRename(struct emulbase *emulbase, char *filename, char *newfilename)
{
    LONG error;

    HostLib_Lock();

    error = nocase_rename(emulbase, filename, newfilename);
    if (error)
	error = err_u2a(emulbase);

    HostLib_Unlock();

    return error;
}

LONG DoSetDate(struct emulbase *emulbase, char *name, struct DateStamp *date)
{
    struct utimbuf times;
    LONG res;

    HostLib_Lock();

    times.actime = datestamp2timestamp(emulbase, date);
    times.modtime = times.actime;

    res = nocase_utime(emulbase, name, &times);
    if (res < 0)
        res = err_u2a(emulbase);

    HostLib_Unlock();

    return res;
}

SIPTR DoSetSize(struct emulbase *emulbase, struct filehandle *fh, SIPTR offset, ULONG mode, SIPTR *err)
{
    SIPTR absolute = 0;
    SIPTR error = 0;

    HostLib_Lock();

    switch (mode) {
    case OFFSET_BEGINNING:
        break;

    case OFFSET_CURRENT:
       	absolute = LSeek((IPTR)fh->fd, 0, SEEK_CUR);
       	AROS_HOST_BARRIER
        break;

    case OFFSET_END:
        absolute = LSeek((IPTR)fh->fd, 0, SEEK_END); 
        AROS_HOST_BARRIER
        break;

    default:
    	error = ERROR_UNKNOWN;
    }

    if (absolute == -1)
        error = err_u2a(emulbase);

    if (!error)
    {
        absolute += offset;
	error = FTruncate((IPTR)fh->fd, absolute);
	AROS_HOST_BARRIER
	if (error)
	    error = err_u2a(emulbase);
    }

    HostLib_Unlock();

    if (error)
        absolute = -1;

    *err = error;
    return absolute;
}

LONG DoStatFS(struct emulbase *emulbase, char *path, struct InfoData *id)
{
    struct statfs buf;
    LONG err;
    
    HostLib_Lock();

    err = emulbase->pdata.SysIFace->statfs(path, &buf);
    AROS_HOST_BARRIER
    if (err)
    	err = err_u2a(emulbase);

    HostLib_Unlock();

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
    HostLib_Lock();

    emulbase->pdata.SysIFace->rewinddir(fh->fd);
    AROS_HOST_BARRIER

    HostLib_Unlock();

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

    HostLib_Lock();

    err = emulbase->pdata.SysIFace->lstat(name, st);
    AROS_HOST_BARRIER
    if (err)
	err = err_u2a(emulbase);

    HostLib_Unlock();

    if (FoundName)
    {
	DEXAM(bug("[emul] Freeing full name\n"));
	FreeVecPooled(emulbase->mempool, name);
    }
    return err;
}	

LONG DoExamineEntry(struct emulbase *emulbase, struct filehandle *fh, char *EntryName,
		   struct ExAllData *ead, ULONG size, ULONG type)
{
    STRPTR next, end, last, name;
    struct stat st;
    LONG err;

    DEXAM(bug("[emul] DoExamineEntry(0x%p, %s, 0x%p, %u, %u)\n", fh, EntryName, ead, size, type));

    /* Return an error, if supplied type is not supported. */
    if(type>ED_OWNER)
	return ERROR_BAD_NUMBER;

    /* Check, if the supplied buffer is large enough. */
    next=(STRPTR)ead+sizes[type];
    end =(STRPTR)ead+size;
    DEXAM(bug("[emul] ead 0x%p, next 0x%p, end 0x%p\n", ead, next, end));

    if(next>end) /* > is correct. Not >= */
	return ERROR_BUFFER_OVERFLOW;

    err = stat_entry(emulbase, fh, EntryName, &st);
    if (err)
    	return err;

    DEXAM(KrnPrintf("[emul] File mode %o, size %u\n", st.st_mode, st.st_size));
    DEXAM(KrnPrintf("[emul] Filling in information\n"));
    DEXAM(KrnPrintf("[emul] ead 0x%p, next 0x%p, end 0x%p, size %u, type %u\n", ead, next, end, size, type));

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

LONG DoExamineNext(struct emulbase *emulbase, struct filehandle *fh,
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

    HostLib_Lock();

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

    HostLib_Unlock();

    if (!dir)
    	return ERROR_NO_MORE_ENTRIES;

    err = stat_entry(emulbase, fh, dir->d_name, &st);
    if (err)
    {
        DEXAM(bug("stat_entry() failed for %s\n", dir->d_name));
        return err;
    }

    DEXAM(KrnPrintf("[emul] File mode %o, size %u\n", st.st_mode, st.st_size));

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

    DEXAM(bug("[emul] DirentryType %d\n", FIB->fib_DirEntryType));

    /* fast copying of the filename */
    src  = dir->d_name;
    dest = &FIB->fib_FileName[1];

    for (i =0; i<MAXFILENAMELENGTH-1;i++)
    {
	if(! (*dest++=*src++) )
	{
	    break;
	}
    }
    FIB->fib_FileName[0] = i;

    return 0;
}

/*********************************************************************************************/

LONG DoExamineAll(struct emulbase *emulbase, struct filehandle *fh, struct ExAllData *ead,
                  struct ExAllControl *eac, ULONG size, ULONG type, struct DosLibrary *DOSBase)
{
    struct ExAllData *last=NULL;
    STRPTR end=(STRPTR)ead+size;
    struct dirent *dir;
    LONG error = 0;
#ifndef HOST_OS_android
    SIPTR oldpos;
#endif

    eac->eac_Entries = 0;
    if(fh->type!=FHD_DIRECTORY)
	return ERROR_OBJECT_WRONG_TYPE;

    DEXAM(bug("[emul] examine_all()\n"));


#ifndef HOST_OS_android
    HostLib_Lock();

    if (eac->eac_LastKey == 0)
    {
        /* Theoretically this doesn't work if opendir/telldir "handle"
           can be 0 for a dir entry which is not the first one! */
           
        eac->eac_LastKey = fh->ph.dirpos_first;
    }
    
    emulbase->pdata.SysIFace->seekdir(fh->fd, eac->eac_LastKey);
    AROS_HOST_BARRIER

    HostLib_Unlock();
#endif

    for(;;)
    {
	HostLib_Lock();

#ifndef HOST_OS_android
        oldpos = eac->eac_LastKey;
                
	//oldpos = emulbase->pdata.SysIFace->telldir(fh->fd);
	//AROS_HOST_BARRIER
#endif

        *emulbase->pdata.errnoPtr = 0;
	dir = ReadDir(emulbase, fh, &eac->eac_LastKey);

	if (!dir)
	    error = err_u2a(emulbase);

	HostLib_Unlock();

	if (!dir)
	    break;

	DEXAM(bug("[emul] Found entry %s\n", dir->d_name));

	if (eac->eac_MatchString && !MatchPatternNoCase(eac->eac_MatchString, dir->d_name)) {
	    DEXAM(bug("[emul] Entry does not match, skipping\n"));
	    continue;
	}

	error = DoExamineEntry(emulbase, fh, dir->d_name, ead, end-(STRPTR)ead, type);
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
        eac->eac_LastKey = oldpos;

	//HostLib_Lock();
	//emulbase->pdata.SysIFace->seekdir(fh->fd, oldpos);
	//AROS_HOST_BARRIER
	//HostLib_Unlock();
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

    HostLib_Lock();

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

	newunixpath = AllocVecPooled(emulbase->mempool, hlen + splen + 1);
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

    HostLib_Unlock();

    return newunixpath;
}

ULONG GetCurrentDir(struct emulbase *emulbase, char *path, ULONG len)
{
    char *res;

    DMOUNT(bug("[emul] GetCurrentDir(0x%p, %u)\n", path, len)); 

    HostLib_Lock();

    res = emulbase->pdata.SysIFace->getcwd(path, len);
    AROS_HOST_BARRIER

    HostLib_Unlock();

    DMOUNT(bug("[emul] getcwd() returned %s\n", res));
    return res ? TRUE : FALSE;
}

BOOL CheckDir(struct emulbase *emulbase, char *path)
{
    int res;
    struct stat st;

    DMOUNT(bug("[emul] CheckDir(%s)\n", path));

    HostLib_Lock();

    res = emulbase->pdata.SysIFace->stat(path, &st);
    AROS_HOST_BARRIER

    HostLib_Unlock();

    DMOUNT(bug("[emul] Result: %d, mode: %o\n", res, st.st_mode));
    if ((!res) && S_ISDIR(st.st_mode))
	return FALSE;

    return TRUE;
}
