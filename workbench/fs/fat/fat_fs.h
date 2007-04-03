/*
 * fat.handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#ifndef FAT_HANDLER_H
#define FAT_HANDLER_H

#define __DEBUG__ 1

#include <aros/libcall.h>
#include <devices/trackdisk.h>

#include "fat_struct.h"

#include "cache.h"

/* filesystem structures */

#define ID_FAT_DISK 0x46415400UL

#define ID_FAT12_DISK 0x46415400UL
#define ID_FAT16_DISK 0x46415401UL
#define ID_FAT32_DISK 0x46415402UL

#define ACTION_VOLUME_ADD 16000
#define ACTION_VOLUME_REMOVE 16001

extern struct Globals *glob;

#define DEF_POOL_SIZE 65536
#define DEF_POOL_TRESHOLD DEF_POOL_SIZE
#define DEF_BUFF_LINES 128
#define DEF_READ_AHEAD 16*1024
/*
struct CacheBuffer {
    struct CacheBuffer *next;
    ULONG count;
    ULONG block;
    void *data;
    ULONG magic;
};
*/

/* extent is a reference to a chunk of a single file/dir that is held in one
 * or more consecutive clusters */
struct Extent {
    ULONG sector;           /* first sector of cur_cluster */
    ULONG count;            /* number of sectors in the extent */
    ULONG offset;           /* distance (sectors) into the file of the current extent */
    ULONG cur_cluster;      /* first cluster in the extent */
    ULONG next_cluster;     /* cluster at beginning of next extent */
    ULONG start_cluster;    /* first cluster in the file (or at least where we
                               started searching from) */
    ULONG last_cluster;     /* last cluster in the extent */
};


/* a handle on a directory */
struct DirHandle {
    struct FSSuper      *sb;            /* filesystem data */

    ULONG               first_cluster;  /* first cluster of this directory */
    ULONG               cur_cluster;    /* cluster that the current sector is within */

    ULONG               cluster_offset; /* cluster number of this cluster within the current dir */

    ULONG               first_sector;   /* first sector in the first cluster, for fat12/16 root dir */
    ULONG               cur_sector;     /* sector number the current entry is within */

    ULONG               sector_offset;  /* current sector as an offset in the current cluster
                                           ie cur = sector(cur_cluster) + offset */

    ULONG               cur_index;      /* last entry returned, for GetNextDirEntry */

    struct cache_block  *block;         /* current block from the cache */
};

/* single directory entry */
struct DirEntry {
    struct FSSuper      *sb;            /* filesystem data */

    ULONG               cluster;        /* cluster the containing directory starts at */
    ULONG               index;          /* index of this entry */
    
    ULONG               sector;         /* absolute sector number this entry came from */
    ULONG               offset;         /* byte offset within that sector */

    union {
        struct FATDirEntry      entry;
        struct FATLongDirEntry  long_entry;
    } e;
};

#define fl_Key dir_entry

#define FAT_ROOTDIR_MARK    0xFFFFFFFFlu

struct ExtFileLock {
    /* struct FileLock */
    BPTR            fl_Link;
    ULONG           dir_entry;  /* was fl_Key. this is our dir entry within dir_cluster */
    LONG            fl_Access;
    struct MsgPort *fl_Task;
    BPTR            fl_Volume;

    /* coinsistency check */
    ULONG           magic;   

    ULONG           dir_cluster;    /* first cluster of directory we're in */

    ULONG           attr;
    ULONG           size;
    ULONG           first_cluster;

    struct Extent   data_ext[1];

    /* used in directory scanning and file reading */
    ULONG           pos;

    UBYTE           name[108];

    struct DirHandle *dirhandle;
};

struct VolumeInfo {
    UBYTE               name[32];     /* BCPL string */
    struct DateStamp    create_time;
};

struct FSSuper {
    struct FSSuper *next;
    struct DosList *doslist;

    ULONG sectorsize;
    ULONG sectorsize_bits;

    ULONG cluster_sectors;
    ULONG clustersize;
    ULONG clustersize_bits;
    ULONG cluster_sectors_bits;

    ULONG first_fat_sector;
    ULONG first_data_sector;
    ULONG first_rootdir_sector;

    ULONG rootdir_sectors;
    ULONG total_sectors;
    ULONG data_sectors;
    ULONG clusters_count;
    ULONG fat_size;

    ULONG free_clusters;

    ULONG volume_id;
    ULONG type;
    ULONG eoc_mark;

    UBYTE *fat;
    ULONG fat32_cachesize;
    ULONG fat32_cachesize_bits;
    ULONG fat32_cache_block;

    ULONG rootdir_cluster;
    ULONG rootdir_sector;

    struct VolumeInfo volume;

    /* function table */
    ULONG (*func_get_fat_entry)(struct FSSuper *sb, ULONG n);
    /* ... */
};

struct Globals {
    /* mem/task */
    struct Task *ourtask;
    struct MsgPort *ourport;
    APTR mempool;

    struct cache *cache;

    /* fs */
    struct DosList *devnode;
    struct FileSysStartupMsg *fssm;
    LONG quit;
    BOOL autodetect;

    /* io */
    struct IOExtTD *diskioreq;
    struct IOExtTD *diskchgreq;
    struct MsgPort *diskport;
        LONG blocksize;

    /* volumes */
    struct FSSuper *sb;    /* current sb */
    struct FSSuper *sblist;   /* list of sbs with outstanding locks */

    /* disk status */
    LONG disk_inserted;
    LONG disk_inhibited;
};

#include "support.h"

/* new definitions as we refactor the code */

/* get the first sector of a cluster */
#define SECTOR_FROM_CLUSTER(sb,cl) ((ULONG) (((cl-2) << sb->cluster_sectors_bits) + sb->first_data_sector))

#define FIRST_FILE_CLUSTER(de)                                       \
    (AROS_LE2WORD((de)->e.entry.first_cluster_lo) |                  \
     (((ULONG) AROS_LE2WORD((de)->e.entry.first_cluster_hi)) << 16))

#endif
