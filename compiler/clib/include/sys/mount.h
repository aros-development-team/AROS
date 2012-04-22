/*
  Desc: BSD4.4 header file sys/mount.h
        This file is not part of POSIX.1-2008 standard
*/

/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)mount.h     7.22 (Berkeley) 6/3/91
 */


#ifndef _SYS_MOUNT_H_
#define _SYS_MOUNT_H_

#ifndef _POSIX_SOURCE

#include <stdint.h>

typedef struct { int32_t val[2]; } fsid_t;      /* file system id type */

#define MNAMELEN 90     /* length of buffer for returned name */

struct statfs {
        short   f_type;                 /* type of filesystem (see below) */
        short   f_flags;                /* copy of mount flags */
        long    f_fsize;                /* fundamental file system block size */
        long    f_bsize;                /* optimal transfer block size */
        long    f_blocks;               /* total data blocks in file system */
        long    f_bfree;                /* free blocks in fs */
        long    f_bavail;               /* free blocks avail to non-superuser */
        long    f_files;                /* total file nodes in file system */
        long    f_ffree;                /* free file nodes in fs */
        fsid_t  f_fsid;                 /* file system id */
        long    f_spare[9];             /* spare for later */
        char    f_mntonname[MNAMELEN];  /* directory on which mounted */
        char    f_mntfromname[MNAMELEN];/* mounted filesystem */
};

/*
 * Flags for various system call interfaces.
 *
 * forcibly flags for vfs_umount().
 * waitfor flags to vfs_sync() and getfsstat()
 */

#define MNT_FORCE       1
#define MNT_NOFORCE     2
#define MNT_WAIT        1
#define MNT_NOWAIT      2


/*
 * filesystem control flags.
 *
 * MNT_MLOCK lock the mount entry so that name lookup cannot proceed
 * past the mount point.  This keeps the subtree stable during mounts
 * and unmounts.
 */
#define MNT_UPDATE      0x00010000      /* not a real mount, just an update */
#define MNT_MLOCK       0x00100000      /* lock so that subtree is stable */
#define MNT_MWAIT       0x00200000      /* someone is waiting for lock */
#define MNT_MPBUSY      0x00400000      /* scan of mount point in progress */
#define MNT_MPWANT      0x00800000      /* waiting for mount point */
#define MNT_UNMOUNT     0x01000000      /* unmount in progress */

/*
 * File system types.
 */
#define MOUNT_NONE      0
#define MOUNT_UFS       1
#define MOUNT_NFS       2
#define MOUNT_MFS       3
#define MOUNT_PC        4
#define MOUNT_ADOS_OFS  5       /* for AmigaOS standard and international */
#define MOUNT_ADOS_FFS  6       /* old fs and fast fs */
#define MOUNT_ADOS_IOFS 7
#define MOUNT_ADOS_IFFS 8
#define MOUNT_MAXTYPE   8

static char __attribute__((unused)) *mnt_names[] = {
    "none", "ufs", "nfs", "mfs", "pc", "ofs", "ffs", "iofs", "iffs"
};

__BEGIN_DECLS

int getfsstat (struct statfs *, long, int);
int statfs(const char *path, struct statfs *buf);

__END_DECLS

#endif /* ! _POSIX_SOURCE */

#endif /* !_SYS_MOUNT_H_ */
