/*
    Copyright (C) 2025-2026, The AROS Development Team.
    All rights reserved.

    POSIX.1-2008 function fstatvfs
*/

#include <aros/debug.h>
#include <proto/dos.h>

#include "__dos64.h"
#include <errno.h>
#include <string.h>
#include <sys/statvfs.h>
#include <exec/types.h>
#include <dos/dos.h>

/****************************************************************************
    NAME
        int statvfs(

    SYNOPSIS
        const char *restrict path, struct statvfs *restrict buf)

    FUNCTION
        Read block usage for the volume containing a file or
        directory.

    INPUTS
        path - Null-terminated AROS pathname of an existing file
               or directory. Passed directly to dos.library/Lock();
               POSIX pathname translation is not performed.
        buf - Writable, caller-owned struct statvfs storage. The
              pathname must not overlap this storage.

    RESULT
        0 on success; -1 on failure with errno set as follows:

        EINVAL - path or buf is NULL; the buffer is unchanged.
        ENOENT - The pathname could not be locked.
        EIO    - The volume-information query failed.

        For ENOENT or EIO, the entire result buffer is zeroed.

    NOTES
        Include <sys/statvfs.h>. On success, f_bsize and f_frsize
        contain bytes per block; f_blocks contains total blocks;
        f_bfree contains total blocks minus used blocks; and
        f_bavail equals f_bfree.

        f_files, f_ffree, f_favail, f_fsid and f_flag are zero.
        f_namemax is always 31. These fields are placeholders,
        not queried filesystem limits, identifiers or mount flags.

        The call may wait for DOS pathname resolution and volume
        queries. Its temporary lock is released before returning.

    BUGS
        All Lock() failures become ENOENT, including failures
        unrelated to a missing file. Volume-query errors become
        EIO without preserving their specific cause in errno.

        Block-count precision depends on the filesystem query and
        the compiled result types. Narrowing and inconsistent
        used-block counts are not checked; no EOVERFLOW error is
        reported. Zero f_flag does not establish writability, and
        f_bavail does not account for per-user restrictions.

    SEE ALSO
        fstatvfs(), dos.library/Lock(), dos.library/Info()

*****************************************************************************/
int statvfs(const char *restrict path, struct statvfs *restrict buf)
{
    if (!path || !buf) {
        errno = EINVAL;
        return -1;
    }

    memset(buf, 0, sizeof(*buf));

    BPTR lock = Lock((STRPTR)path, ACCESS_READ);
    if (lock == 0) {
        errno = ENOENT;
        return -1;
    }

    struct InfoData64 info;
    if (!__dos64_info(lock, &info)) {
        UnLock(lock);
        errno = EIO;
        return -1;
    }

    buf->f_bsize = info.id_BytesPerBlock;
    buf->f_frsize = info.id_BytesPerBlock;
    buf->f_blocks = info.id_NumBlocks;
    buf->f_bfree = info.id_NumBlocks - info.id_NumBlocksUsed;
    buf->f_bavail = buf->f_bfree;
    buf->f_files = 0;       // AmigaFS does not support inodes like Unix
    buf->f_ffree = 0;
    buf->f_favail = 0;
    buf->f_fsid = 0;
    buf->f_flag = 0;       // No mount flags
    buf->f_namemax = 31;   // Classic Amiga filename max length

    UnLock(lock);

    return 0;
}
