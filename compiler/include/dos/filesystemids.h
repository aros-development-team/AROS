#ifndef DOS_FILESYSTEMIDS_H
#define DOS_FILESYSTEMIDS_H

/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id: dos.h 49655 2014-09-21 23:01:25Z neil $

    Desc: Additional Filesystem Id's
    Lang: english
*/

#ifndef AROS_MACROS_H
#   include <aros/macros.h>
#endif

#define ID_AFS0_DISK            (0x41465300L) /* AFS\0 */
#define ID_muFS_DISK            (0x6d754653L) /* muFS - Multiuserfsys */

#define ID_PFS_DISK             (0x50465301L) /* PFS */
#define ID_PFS2_DISK            (0x50465302L)
#define ID_PFS2_SCSI_DISK       (0x50445300L)
#define ID_PFS2_muFS_DISK       (0x6d755046L)
#define ID_FLOPPY_PFS_DISK      (0x50465300L)

#define ID_ACD0_DISK            (0x41434400L) /* ACD\0 - AmiCDFS disk */
#define ID_CDFS_DISK            (0x43444653L) /* CDFS  - AmiCDFS disk */
#define ID_CACHECDFS_DISK       (0x43443031L)
#define ID_ASIMCDFS_DISK        (0x662dabacL)

/* SFS */
#define ID_SFS_BE_DISK          AROS_MAKE_ID('S','F','S',0)
#define ID_SFS_LE_DISK          AROS_MAKE_ID('s','f','s',0)

/* Exotic Filesystems */
#define ID_FAT_DISK             AROS_MAKE_ID('F','A','T',0)
#define ID_FAT12_DISK           AROS_MAKE_ID('F','A','T',0)
#define ID_FAT16_DISK           AROS_MAKE_ID('F','A','T',1)
#define ID_FAT32_DISK           AROS_MAKE_ID('F','A','T',2)

#define ID_NTFS_DISK 		AROS_MAKE_ID('N','T','F','S')

#define ID_MAC_DISK2            (0x4d414300L) /* MAC\0 - xfs mac disk */
#define ID_MNX1_DISK            (0x4d4e5801L) /* MNX\1 - xfs minix disk */
#define ID_MNX2_DISK            (0x4d4e5802L) /* MNX\2 - xfs minix disk */
#define ID_QL5A_DISK            (0x514c3541L) /* QL5A - xfs ql 720k / ed disk */
#define ID_QL5B_DISK            (0x514c3542L) /* QL5B - xfs ql 1440k disk */
#define ID_ZXS0_DISK            (0x5a585300L) /* Spectrum Disciple - xfs */
#define ID_ZXS1_DISK            (0x5a585301L) /* Spectrum UniDos - xfs */
#define ID_ZXS2_DISK            (0x5a585302L) /* Spectrum SamDos - xfs */
#define ID_ZXS4_DISK            (0x5a585304L) /* Spectrum Opus 180k - xfs */
#define ID_ARME_DISK            (0x41524d44L) /* Archimedes - xfs */
#define ID_ARMD_DISK            (0x41524d43L) /* Archimedes - xfs */
#define ID_CPM_DISK             (0x43505c4dL) /* CP/M - xfs */
#define ID_ZXS3_DISK            (0x5a585303L) /* ZXS\3 - Plus3Dos xfs */
#define ID_1541_DISK            (0x31353431L) /* 1541 - xfs */
#define ID_1581_DISK            (0x31353831L) /* 1581 - xfs */
#define ID_MAC_DISK             (0x4d534800L) /* MSH\0 - CrossDos MACDisk ?! */
#define ID_P2A0_DISK            (0x50324130L)

#define ID_EXT2_DISK            (0x45585432L) /* Extended 2 - Linux */

#endif /* DOS_FILESYSTEMIDS_H */
