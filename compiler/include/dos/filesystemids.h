#ifndef DOS_FILESYSTEMIDS_H
#define DOS_FILESYSTEMIDS_H

/*
    Copyright © 2014-2019, The AROS Development Team. All rights reserved.
    $Id: dos.h 49655 2014-09-21 23:01:25Z neil $

    Desc: Additional id_DiskType Filesystem Id's
    Lang: english
*/

#ifndef AROS_MACROS_H
#   include <aros/macros.h>
#endif

#define ID_muFS_DISK                    AROS_MAKE_ID('m','u','F','S')           /* muFS FFS file system (intl., no dir cache)   */
#define ID_DOS_muFS_DISK                AROS_MAKE_ID('m','u','F',0)             /* muFS OFS file system (non-intl.)             */
#define ID_FFS_muFS_DISK                AROS_MAKE_ID('m','u','F',1)             /* muFS FFS file system (non-intl.)             */
#define ID_INTER_DOS_muFS_DISK          AROS_MAKE_ID('m','u','F',2)             /* muFS OFS file system (intl., no dir cache)   */
#define ID_INTER_FFS_muFS_DISK          AROS_MAKE_ID('m','u','F',3)             /* muFS FFS file system (intl., no dir cache)   */
#define ID_FASTDIR_DOS_muFS_DISK        AROS_MAKE_ID('m','u','F',4)             /* muFS OFS file system (intl., dir cache)      */
#define ID_FASTDIR_FFS_muFS_DISK        AROS_MAKE_ID('m','u','F',5)             /* muFS FFS file system (intl., dir cache)      */

#define ID_AFS0_DISK                    AROS_MAKE_ID('A','F','S',0)             /* AFS file system                              */
#define ID_AFS1_DISK                    AROS_MAKE_ID('A','F','S',1)             /* AFS file system (experimental)               */
#define ID_AFS_muFS_DISK	        AROS_MAKE_ID('m','u','A','F')           /* muFS AFS file system                         */

#define ID_PFS_DISK                     AROS_MAKE_ID('P','F','S',1)             /* PFS file system 1                            */
#define ID_PFS2_DISK                    AROS_MAKE_ID('P','F','S',2)             /* PFS file system 2                            */
#define ID_PFS3_DISK                    AROS_MAKE_ID('P','F','S',3)             /* PFS file system 3                            */
#define ID_PFS2_SCSI_DISK               AROS_MAKE_ID('P','D','S',2)             /* PFS file system 2, SCSIdirect                */
#define ID_PFS3_SCSI_DISK               AROS_MAKE_ID('P','D','S',3)             /* PFS file system 3, SCSIdirect                */
#define ID_PFS2_muFS_DISK               AROS_MAKE_ID('m','u','P','F')           /* muFS PFS file system                         */
#define ID_FLOPPY_PFS_DISK              AROS_MAKE_ID('P','F','S',0)

#define ID_ACD0_DISK                    AROS_MAKE_ID('A','C','D',0)             /* CD-ROM - AmiCDFS disk                        */
#define ID_CDFS_DISK                    AROS_MAKE_ID('C','D','F','S')           /* CD-ROM - Amiga CDrive or AmiCDFS             */
#define ID_HSIERRA_DISK                 AROS_MAKE_ID('C','D','0','0')           /* CD-ROM - High Sierra format                  */
#define ID_ISO9660_DISK                 AROS_MAKE_ID('C','D','0','1')           /* CD-ROM - ISO9660 format                      */
#define ID_CACHECDFS_DISK               ID_ISO9660_DISK
#define ID_ASIMCDFS_DISK                AROS_MAKE_ID(0x66,0x2d,0xab,0xac)       /* CD-ROM - AsimCDFS                            */

#define ID_SFS_BE_DISK                  AROS_MAKE_ID('S','F','S',0)             /* Smart File System - Big Endian               */
#define ID_SFS_LE_DISK                  AROS_MAKE_ID('s','f','s',0)             /* Smart File System - Little Endian            */

/* Exotic Filesystems */
#define ID_FAT_DISK                     AROS_MAKE_ID('F','A','T',0)
#define ID_FAT12_DISK                   AROS_MAKE_ID('F','A','T',0)
#define ID_FAT16_DISK                   AROS_MAKE_ID('F','A','T',1)
#define ID_FAT32_DISK                   AROS_MAKE_ID('F','A','T',2)

#define ID_NTFS_DISK 		        AROS_MAKE_ID('N','T','F','S')

#define ID_MAC_DISK                     AROS_MAKE_ID('M','S','H',0)             /* MSH\0 - CrossDos MACDisk ?!                  */
#define ID_MAC_DISK2                    AROS_MAKE_ID('M','A','C',0)             /* MAC\0 - xfs mac disk                         */
#define ID_MNX1_DISK                    AROS_MAKE_ID('M','N','X',1)             /* MNX\1 - xfs minix disk                       */
#define ID_MNX2_DISK                    AROS_MAKE_ID('M','N','X',2)             /* MNX\2 - xfs minix disk                       */
#define ID_QL5A_DISK                    AROS_MAKE_ID('Q','L','5','A')           /* QL5A - xfs ql 720k / ed disk                 */
#define ID_QL5B_DISK                    AROS_MAKE_ID('Q','L','5','B')           /* QL5B - xfs ql 1440k disk                     */
#define ID_ZXS0_DISK                    AROS_MAKE_ID('Z','X','S',0)             /* Spectrum Disciple - xfs                      */
#define ID_ZXS1_DISK                    AROS_MAKE_ID('Z','X','S',1)             /* Spectrum UniDos - xfs                        */
#define ID_ZXS2_DISK                    AROS_MAKE_ID('Z','X','S',2)             /* Spectrum SamDos - xfs                        */
#define ID_ZXS3_DISK                    AROS_MAKE_ID('Z','X','S',3)             /* ZXS\3 - Plus3Dos xfs                         */
#define ID_ZXS4_DISK                    AROS_MAKE_ID('Z','X','S',4)             /* Spectrum Opus 180k - xfs                     */
#define ID_ARME_DISK                    AROS_MAKE_ID('A','R','M','E')           /* Archimedes - xfs                             */
#define ID_ARMD_DISK                    AROS_MAKE_ID('A','R','M','D')           /* Archimedes - xfs                             */
#define ID_CPM_DISK                     AROS_MAKE_ID('C','P','\\','M')          /* CP\M - xfs                                   */
#define ID_1541_DISK                    AROS_MAKE_ID('1','5','4','1')           /* 1541 - xfs                                   */
#define ID_1581_DISK                    AROS_MAKE_ID('1','5','8','1')           /* 1581 - xfs                                   */
#define ID_P2A0_DISK                    AROS_MAKE_ID('P','2','A','0')

#define ID_EXT2_DISK                    AROS_MAKE_ID('E','X','T','2')           /* Extended 2 - Linux                           */

#endif /* DOS_FILESYSTEMIDS_H */
