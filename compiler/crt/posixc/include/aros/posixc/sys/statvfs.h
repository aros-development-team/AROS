#ifndef _POSIXC_SYS_STATVFS_H_
#define _POSIXC_SYS_STATVFS_H_
/*
    Copyright © 2025, The AROS Development Team.
    All rights reserved.
    $Id$

    POSIX.1-2008 header file: sys/statvfs.h
*/

#include <aros/system.h>
#include <aros/types/fs_t.h>

struct statvfs {
    unsigned long  f_bsize;    // File system block size
    unsigned long  f_frsize;   // Fragment size
    fsblkcnt_t     f_blocks;   // Total number of blocks
    fsblkcnt_t     f_bfree;    // Free blocks
    fsblkcnt_t     f_bavail;   // Free blocks available to unprivileged user
    fsfilcnt_t     f_files;    // Total file nodes
    fsfilcnt_t     f_ffree;    // Free file nodes
    fsfilcnt_t     f_favail;   // Free file nodes for unprivileged user
    unsigned long  f_fsid;     // Filesystem ID
    unsigned long  f_flag;     // Mount flags
    unsigned long  f_namemax;  // Maximum filename length
};

__BEGIN_DECLS

int statvfs(const char *path, struct statvfs *buf);
int fstatvfs(int fd, struct statvfs *buf);

__END_DECLS

#endif /* _POSIXC_SYS_STATVFS_H_ */
