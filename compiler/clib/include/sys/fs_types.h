#ifndef _SYS_FS_TYPES_H_
#define _SYS_FS_TYPES_H_

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

static char *mnt_names[] = { "none", "ufs", "nfs", "mfs", "pc", "ofs",
                             "ffs", "iofs", "iffs" };

#endif /* !_SYS_FS_TYPES_H */
