/*
    Copyright © 2025, The AROS Development Team.
    All rights reserved.

    POSIX.1-2008 function fstatvfs
*/

#include <aros/debug.h>
#include <proto/dos.h>
#include <errno.h>
#include <string.h>
#include <sys/statvfs.h>
#include <exec/types.h>
#include <dos/dos.h>

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

    struct InfoData info;
    if (!Info(lock, &info)) {
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
