/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <proto/dos.h>

#include <string.h>
#include <errno.h>

#include "__time.h"
#include "__errno.h"
#include "__stat.h"

#include <sys/stat.h>


static mode_t __prot_a2u(ULONG protect);
static uid_t  __id_a2u(UWORD id);


int __stat(BPTR lock, struct stat *sb)
{
    struct FileInfoBlock *fib;

    fib = AllocDosObject(DOS_FIB, NULL);

    if (!fib)
    {
        errno = IoErr2errno(IoErr());

        return -1;
    }

    if (!Examine(lock, fib))
    {
        errno = IoErr2errno(IoErr());
        FreeDosObject(DOS_FIB, fib);

        return -1;
    }

    sb->st_dev     = (dev_t)((struct FileHandle *)lock)->fh_Device;
    sb->st_ino     = (ino_t)fib->fib_DiskKey;
    sb->st_size    = (off_t)fib->fib_Size;
    sb->st_blocks  = (long)fib->fib_NumBlocks;
    sb->st_atime   =
    sb->st_ctime   =
    sb->st_mtime   = (fib->fib_Date.ds_Days * 24*60 + fib->fib_Date.ds_Minute + __gmtoffset) * 60 +
	              fib->fib_Date.ds_Tick / TICKS_PER_SECOND + OFFSET_FROM_1970;
    sb->st_uid     = __id_a2u(fib->fib_OwnerUID);
    sb->st_gid     = __id_a2u(fib->fib_OwnerGID);
    sb->st_mode    = __prot_a2u(fib->fib_Protection);

    {
        struct InfoData info;

        if (Info(lock, &info))
        {
            sb->st_blksize = info.id_BytesPerBlock;
        }
        else
        {
        /* The st_blksize is just a guideline anyway, so we set it
           to 1024 in case Info() didn't succeed */
        sb->st_blksize = 1024;
        }
    }

    /* With SAS/C++ 6.55 on the Amiga, `stat' sets the `st_nlink'
       field to -1 for a file, or to 1 for a directory.  */
    switch (fib->fib_DirEntryType)
    {
        case ST_PIPEFILE:
            /* don't use S_IFIFO, we don't have a mkfifo() call ! */
            sb->st_mode |= S_IFCHR;
            break;

        case ST_ROOT:
        case ST_USERDIR:
        case ST_LINKDIR:
            sb->st_nlink = 1;
            sb->st_mode |= S_IFDIR;
            break;

        case ST_SOFTLINK:
            sb->st_nlink = 1;
            sb->st_mode |= S_IFLNK;
            break;

        case ST_FILE:
        case ST_LINKFILE:
        default:
            sb->st_nlink = -1;
            sb->st_mode |= S_IFREG;
    }

    FreeDosObject(DOS_FIB, fib);

    return 0;
}


static mode_t __prot_a2u(ULONG protect)
{
    mode_t uprot = 0000;

    if ((protect & FIBF_SCRIPT))
        uprot |= 0111;
    /* The following three flags are low-active! */
    if (!(protect & FIBF_EXECUTE))
        uprot |= 0100;
    if (!(protect & FIBF_WRITE))
        uprot |= 0200;
    if (!(protect & FIBF_READ))
        uprot |= 0400;
    if ((protect & FIBF_GRP_EXECUTE))
        uprot |= 0010;
    if ((protect & FIBF_GRP_WRITE))
        uprot |= 0020;
    if ((protect & FIBF_GRP_READ))
        uprot |= 0040;
    if ((protect & FIBF_OTR_EXECUTE))
        uprot |= 0001;
    if ((protect & FIBF_OTR_WRITE))
        uprot |= 0002;
    if ((protect & FIBF_OTR_READ))
        uprot |= 0004;

    return uprot;
}


static uid_t __id_a2u(UWORD id)
{
    switch(id)
    {
        case (UWORD)-1:
            return 0;

        case (UWORD)-2:
            return (UWORD)-1;

        case 0:
            return (UWORD)-2;

        default:
            return id;
    }
}
