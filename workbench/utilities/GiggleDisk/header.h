#ifndef HEADER_H
#define HEADER_H 1

/*************************************************************************/

#include <dos/filehandler.h>
#include <devices/hardblocks.h>

#include "locale_strings.h"

/*************************************************************************/

#pragma pack(1)         /* force byte alignment */

/*
** This is a standard partition table
*/

struct MBRPartition {
    UBYTE     BootIndicator;  /* 0x80 is active */
    UBYTE     StartingHead;
    UBYTE     StartingSector;
    UBYTE     StartingCylinder;
    UBYTE     PartitionType;
    UBYTE     EndingHead;
    UBYTE     EndingSector;
    UBYTE     EndingCylinder;
    ULONG     StartSector;
    ULONG     NumberOfSectors;
};

struct MBRPartitionBlock {
    UBYTE     Unknown1[0x1be];
    struct    MBRPartition Partition[4]; /* 0x1ce */
    UWORD     Partition_TableFlag; /*  0x1fe */
};

# pragma pack()         /* force default alignment */

#define PART_TABLEFLAG   0x55AA
#define PART_BOOTINDICATOR  0x80

#define SECTORSIZE       0x200


/* Byte values. Highest two bits of sector are added to cylinder. */

#define SECTOR(sector)     (sector & 0x3F)
#define CYLINDER(sector, cylinder) (cylinder | ((sector & 0xc0)<<2))

struct PartitionEntry {
    struct Node Node;
    ULONG       PartitionType;   /* file system type as in MBR or RDB */
    ULONG       PartitionSize;
    ULONG       SectorSize;
    ULONG       StartHead;
    ULONG       StartSector;
    ULONG       StartCylinder;
    ULONG       EndHead;
    ULONG       EndSector;
    ULONG       EndCylinder;
    ULONG       FirstSector;
    ULONG       NumberOfSectors;
/* Amiga Style extensions */
    ULONG       Device;
    ULONG       Unit;
    ULONG       StackSize;
    char        FileSysName[84+2]; /* copy of partitionblock->FileSysName */
    char        DriveName[32];     /* copy of partitionblock->DriveName */
    ULONG       Flags;             /* copy of partitionblock->bp_Flags */
    struct DosEnvec ENV;           /* copy of partitionblock->Environment */
};

#define PBFF_VIRTUAL 64 /* a little add on */

/* some new disk IDs for non standard Amiga file systems */

#define ID_FASTDIRINTER_DOS_DISK (0x444F5306L)	 /* 'DOS\6' */
#define ID_FASTDIRINTER_FFS_DISK (0x444F5307L)	 /* 'DOS\7' */

#define ID_FAT0_DISK              (0x46415400L)  /* 'FAT\0' (FAT95) */
#define ID_FAT1_DISK              (0x46415401L)  /* 'FAT\1' (FAT95) */
#define ID_FAT2_DISK              (0x46415402L)  /* 'FAT\0' (FAT95) */
#define ID_FAT3_DISK              (0x46415403L)  /* 'FAT\1' (FAT95) */
#define ID_FAT4_DISK              (0x46415404L)  /* 'FAT\0' (FAT95) */
#define ID_FAT5_DISK              (0x46415405L)  /* 'FAT\1' (FAT95) */
#define ID_FAT6_DISK              (0x46415406L)  /* 'FAT\0' (FAT95) */
#define ID_FAT7_DISK              (0x46415407L)  /* 'FAT\1' (FAT95) */

#define ID_CFS_DISK               (0x43465300L)  /* 'CFS\0' */
#ifndef ID_SFS_DISK
#define ID_SFS_DISK               (0x53465300L)  /* 'SFS\0' */
#endif

#define ID_NTFS_DISK              (0x4e544653L)  /* 'NTFS'  */
#define ID_EXT2_DISK              (0x45585432L)  /* 'EXT2'  */
#define ID_XFS0_DISK              (0x58465300L)  /* 'XFS\0' */

#define ID_MSH_DISK               (0x4D534800L)  /* 'MSH\0' */

/* some MBR disk ids for internal usage */

#define FSID_EXTENDED          0x05
#define FSID_EXTENDED32        0x0f

#define FSID_DOSFAT12          0x01
#define FSID_DOSFAT16s         0x04
#define FSID_DOSFAT16          0x06
#define FSID_DOSFAT32          0x0b
#define FSID_DOSFAT32LBA       0x0c
#define FSID_DOSFAT16LBA       0x0e

#define FSID_NTFS              0x07
#define FSID_NTFSVSET1         0x86
#define FSID_NTFSVSET2         0x87

#define FSID_AVDH 0x76

#define FSID_EXT2              0x83 /* FIXME only place holder */
#define FSID_SGIX              0x83 /* FIXME only place holder */

/*************************************************************************/

/*
** Simple array for keeping the MBR type and name strings
*/

typedef struct {
    ULONG Type;
    char* Name;

} TYPFSID;

#define FSID_TERMINATED 0xffffffff /* termination ID for array */

extern TYPFSID FSIDARRAY[];

/*************************************************************************/

/*
** Our internal partition list
*/

extern struct List partitionlist;

extern ULONG partitionnumber;

/*************************************************************************/

/*
** Our internal device struct
*/

extern struct MDevice device;

/*************************************************************************/

/*
** our nsd check
*/

extern BOOL device64;

#define MSG_ERROR_NoError 0


/*************************************************************************

Now the foreign file system super blocks

**************************************************************************/


#if !defined(__MORPHOS__) && !defined(__AROS__)
typedef signed long long int QUAD;
typedef unsigned long long int UQUAD;
#endif

# pragma pack(1)

/*************************************************************************/

#define  XFS_SB_MAGIC 0x58465342

struct xfs_super_block
{
    ULONG sb_magicnum;       /* magic number == XFS_SB_MAGIC */
    ULONG sb_blocksize;      /* logical block size, bytes */
    UQUAD sb_dblocks;        /* number of data blocks */
    UQUAD sb_rblocks;        /* number of realtime blocks */
    UQUAD sb_rextents;       /* number of realtime extents */
    UBYTE sb_uuid[16];       /* file system unique id */
    UQUAD sb_logstart;       /* starting block of log if internal */
    UQUAD sb_rootino;        /* root inode number */
    UQUAD sb_rbmino;         /* bitmap inode for realtime extents */
    UQUAD sb_rsumino;        /* summary inode for rt bitmap */
    ULONG sb_rextsize;       /* realtime extent size, blocks */
    ULONG sb_agblocks;       /* size of an allocation group */
    ULONG sb_agcount;        /* number of allocation groups */
    ULONG sb_rbmblocks;      /* number of rt bitmap blocks */
    ULONG sb_logblocks;      /* number of log blocks */
    UWORD sb_versionnum;     /* header version == XFS_SB_VERSION */
    UWORD sb_sectsize;       /* volume sector size, bytes */
    UWORD sb_inodesize;      /* inode size, bytes */
    UWORD sb_inopblock;      /* inodes per block */
    UBYTE sb_fname[12];      /* file system name */
    UBYTE sb_blocklog;       /* log2 of sb_blocksize */
    UBYTE sb_sectlog;        /* log2 of sb_sectsize */
    UBYTE sb_inodelog;       /* log2 of sb_inodesize */
    UBYTE sb_inopblog;       /* log2 of sb_inopblock */
    UBYTE sb_agblklog;       /* log2 of sb_agblocks (rounded up) */
    UBYTE sb_rextslog;       /* log2 of sb_rextents */
    UBYTE sb_inprogress;     /* mkfs is in progress, don't mount */
    UBYTE sb_imax_pct;       /* max % of fs for inode space */
                             /* statistics */
    UQUAD sb_icount;         /* allocated inodes */
    UQUAD sb_ifree;          /* free inodes */
    UQUAD sb_fdblocks;       /* free data blocks */
    UQUAD sb_frextents;      /* free realtime extents */

    UQUAD sb_uquotino;       /* user quota inode */
    UQUAD sb_gquotino;       /* group quota inode */
    UWORD sb_qflags;         /* quota flags */
    UBYTE sb_flags;          /* misc. flags */
    UBYTE sb_shared_vn;      /* shared version number */
    ULONG sb_inoalignmt;     /* inode chunk alignment, fsblocks */
    ULONG sb_unit;           /* stripe or raid unit */
    ULONG sb_width;          /* stripe or raid width */
    UBYTE sb_dirblklog;      /* log2 of dir block size (fsbs) */
    UBYTE sb_logsectlog;     /* log2 of the log sector size */
    UWORD sb_logsectsize;    /* sector size for the log, bytes */
    ULONG sb_logsunit;       /* stripe unit size for the log */
    ULONG sb_features2;      /* additonal feature bits */
};

/*************************************************************************/

#define EXT2_SB_MAGIC 0xEF53

struct ext2_super_block {
    ULONG    s_inodes_count;            /* Inodes count */
    ULONG    s_blocks_count;            /* Blocks count */
    ULONG    s_r_blocks_count;          /* Reserved blocks count */
    ULONG    s_free_blocks_count;       /* Free blocks count */
    ULONG    s_free_inodes_count;       /* Free inodes count */
    ULONG    s_first_data_block;        /* First Data Block */
    ULONG    s_log_block_size;          /* Block size */
    LONG     s_log_frag_size;           /* Fragment size */
    ULONG    s_blocks_per_group;        /* # Blocks per group */
    ULONG    s_frags_per_group;         /* # Fragments per group */
    ULONG    s_inodes_per_group;        /* # Inodes per group */
    ULONG    s_mtime;                   /* Mount time */
    ULONG    s_wtime;                   /* Write time */
    UWORD    s_mnt_count;               /* Mount count */
    WORD     s_max_mnt_count;           /* Maximal mount count */
    UWORD    s_magic;                   /* Magic signature */
    UWORD    s_state;                   /* File system state */
    UWORD    s_errors;                  /* Behaviour when detecting errors */
    UWORD    s_minor_rev_level;         /* minor revision level */
    ULONG    s_lastcheck;               /* time of last check */
    ULONG    s_checkinterval;           /* max. time between checks */
    ULONG    s_creator_os;              /* OS */
    ULONG    s_rev_level;               /* Revision level */
    UWORD    s_def_resuid;              /* Default uid for reserved blocks */
    UWORD    s_def_resgid;              /* Default gid for reserved blocks */
    ULONG    s_first_ino;               /* First non-reserved inode */
    UWORD    s_inode_size;              /* size of inode structure */
    UWORD    s_block_group_nr;          /* block group # of this superblock */
    ULONG    s_feature_compat;          /* compatible feature set */
    ULONG    s_feature_incompat;        /* incompatible feature set */
    ULONG    s_feature_ro_compat;       /* readonly-compatible feature set */
    UBYTE    s_uuid[16];                /* 128-bit uuid for volume */
    UBYTE    s_volume_name[16];         /* volume name */
    UBYTE    s_last_mounted[64];        /* directory where last mounted */
    ULONG    s_algorithm_usage_bitmap;  /* For compression */
    UBYTE    s_prealloc_blocks;         /* Nr of blocks to try to preallocate*/
    UBYTE    s_prealloc_dir_blocks;     /* Nr to preallocate for dirs */
    UWORD    s_padding1;
};

/*************************************************************************/

#define NTFS_SB_MAGIC "NTFS    "

struct ntfs_super_block {
    UBYTE jump[3];                    /* Irrelevant (jump to boot up code).*/
    UBYTE magic_id[8];                /* Magic "NTFS    ". */
    UWORD bytes_per_sector;           /* Size of a sector in bytes. */
    UBYTE sectors_per_cluster;        /* Size of a cluster in sectors. */
    UWORD reserved_sectors;           /* zero */
    UBYTE fats;                       /* zero */
    UWORD root_entries;               /* zero */
    UWORD sectors;                    /* zero */
    UBYTE media_type;                 /* 0xf8 = hard disk */
    UWORD sectors_per_fat;            /* zero */
    UWORD sectors_per_track;          /* irrelevant */
    UWORD heads;                      /* irrelevant */
    ULONG hidden_sectors;             /* zero */
    ULONG large_sectors;              /* zero */
    UBYTE unused[4];
    QUAD number_of_sectors;
    QUAD mft_lcn;                     /* Cluster location of mft data. */
    QUAD mftmirr_lcn;                 /* Cluster location of copy of mft. */
    BYTE clusters_per_mft_record;     /* Mft record size in clusters. */
    UBYTE reserved0[3];
    BYTE  clusters_per_index_record;  /* Index block size in clusters. */
    UBYTE  reserved1[3];
    UQUAD volume_serial_number;       /* Irrelevant (serial number). */
    ULONG checksum;                   /* Boot sector checksum. */
};

/*************************************************************************/

#define MSDOS_SB_MAGIC "MSDOS" /* 4.1" normaly a version is behind this entry to fill the magic */
#define MSWIN_SB_MAGIC "MSWIN" /* 3.1" normaly a version is behind this entry to fill the magic */

struct msdos_super_block {
    UBYTE jump[3];                    /* Irrelevant (jump to boot up code).*/
    UBYTE magic_id[8];                /* Magic */
   /* cutted here */
};

/*************************************************************************/

#define FAT16_SB_MAGIC "FAT16   "

struct fat16_super_block {
    UBYTE jump[0x36];                    /* Irrelevant (jump to boot up code).*/
    UBYTE magic_id[8];                /* Magic */
   /* cutted here */
};

#pragma pack()

/*************************************************************************/

#endif /* HEADER_H */
