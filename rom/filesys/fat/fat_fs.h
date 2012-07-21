/*
 * fat.handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007-2012 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#ifndef FAT_HANDLER_H
#define FAT_HANDLER_H

#define DEBUG_DIRENTRY      0
#define DEBUG_FILE          0
#define DEBUG_DUMP          0
#define DEBUG_LOCK          0
#define DEBUG_NAMES         0
#define DEBUG_NOTIFY        0
#define DEBUG_OPS           0
#define DEBUG_PACKETS       0
#define DEBUG_CACHESTATS    0
#define DEBUG_MISC          0

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
#define DEF_POOL_THRESHOLD DEF_POOL_SIZE
#define DEF_BUFF_LINES 128
#define DEF_READ_AHEAD 16*1024


/* a handle on something, file or directory */
struct IOHandle {
    struct FSSuper      *sb;            /* filesystem data */

    ULONG               first_cluster;  /* first cluster of this file */
    ULONG               cur_cluster;    /* cluster that the current sector is within */

    ULONG               cluster_offset; /* cluster number of this cluster within the current file */

    ULONG               first_sector;   /* first sector in the first cluster, for fat12/16 root dir */
    ULONG               cur_sector;     /* sector number our block is currently in */

    ULONG               sector_offset;  /* current sector as an offset in the current cluster
                                           i.e. cur = sector(cur_cluster) + offset */

    APTR block;         /* current block from the cache */
    UBYTE *data;         /* current data buffer (from cache) */
};

/* a handle on a directory */
struct DirHandle {
    struct IOHandle     ioh;

    ULONG               cur_index;      /* last entry returned, for GetNextDirEntry */
};

/* single directory entry */
struct DirEntry {
    struct FSSuper      *sb;            /* filesystem data */

    ULONG               cluster;        /* cluster the containing directory starts at */
    ULONG               index;          /* index of this entry */

    ULONG               pos;            /* byte offset within directory that the entry came from */
    
    union {
        struct FATDirEntry      entry;
        struct FATLongDirEntry  long_entry;
    } e;
};

#define FAT_ROOTDIR_MARK    0xFFFFFFFFlu

struct GlobalLock;

struct ExtFileLock {
    /* struct FileLock */
    BPTR                fl_Link;
    IPTR                fl_Key;
    LONG                fl_Access;
    struct MsgPort *    fl_Task;
    BPTR                fl_Volume;

    ULONG               magic;          /* we set this to ID_FAT_DISK so we can tell
                                           our locks from those of other filesystems */

    struct MinNode      node;           /* node in the list of locks attached to the global lock */
    struct GlobalLock   *gl;            /* pointer to the global lock for this file */

    struct IOHandle     ioh;            /* handle for reads and writes */
    ULONG               pos;            /* current seek position within the file */

    BOOL                do_notify;      /* if set, send notification on file close (ACTION_END) */
    struct FSSuper      *sb;            /* pointer to sb, for unlocking when volume is removed */
};

struct GlobalLock {
    struct MinNode      node;

    ULONG               dir_cluster;    /* first cluster of the directory we're in */
    ULONG               dir_entry;      /* this is our dir entry within dir_cluster */

    LONG                access;         /* access mode, shared or exclusive */

    ULONG               first_cluster;  /* first data cluster */

    ULONG               attr;           /* file attributes, from the dir entry */
    ULONG               size;           /* file size, from the dir entry */

    UBYTE name[FAT_MAX_LONG_FILENAME];  /* copy of the name (bstr) */

    struct MinList      locks;          /* list of ExtFileLocks opened on this file */
};

/* a node in the list of notification requests */
struct NotifyNode {
    struct MinNode          node;

    struct GlobalLock       *gl;        /* pointer to global lock if this file is
                                           locked. if it's not, this is NULL */

    struct NotifyRequest    *nr;        /* the request that DOS passed us */
};

struct VolumeInfo {
    APTR mem_pool;
    ULONG id;
    struct MinList locks;           /* global locks */
    struct GlobalLock root_lock;
    struct MinList notifies;
};

struct VolumeIdentity {
    UBYTE               name[32];     /* BCPL string */
    struct DateStamp    create_time;
};

struct FSSuper {
    struct Node node;
    struct DosList *doslist;

    struct VolumeInfo *info;

    struct cache *cache;
    ULONG        first_device_sector;

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
    UWORD fat_count;

    ULONG free_clusters;
    ULONG next_cluster;

    ULONG volume_id;
    ULONG type;
    ULONG eoc_mark;

    APTR  *fat_blocks;
    UBYTE **fat_buffers;
    ULONG fat_blocks_count;

    ULONG fat_cachesize;
    ULONG fat_cachesize_bits;
    ULONG fat_cache_block;
    UWORD fat_cache_no;    /* FAT number that cached FAT blocks belong to */

    APTR fsinfo_block;
    struct FATFSInfo *fsinfo_buffer;

    ULONG rootdir_cluster;
    ULONG rootdir_sector;

    struct VolumeIdentity volume;

    /* function table */
    ULONG (*func_get_fat_entry)(struct FSSuper *sb, ULONG n);
    void  (*func_set_fat_entry)(struct FSSuper *sb, ULONG n, ULONG val);
    /* ... */
};

struct Globals {
    /* mem/task */
    struct Task *ourtask;
    struct MsgPort *ourport;
    APTR mempool;

    struct MsgPort *notifyport;

    /* fs */
    struct DosList *devnode;
    struct FileSysStartupMsg *fssm;
    LONG quit;
    struct DosPacket *death_packet;
    BOOL autodetect;

    /* io */
    struct IOExtTD *diskioreq;
    struct IOExtTD *diskchgreq;
    struct MsgPort *diskport;
    ULONG diskchgsig_bit;
    struct timerequest *timereq;
    struct MsgPort *timerport;
    ULONG last_num;    /* last block number that was outside boundaries */
    UWORD readcmd;
    UWORD writecmd;
    char timer_active;
    char restart_timer;

    /* volumes */
    struct FSSuper *sb;    /* current sb */
    struct MinList sblist;   /* sbs with outstanding locks or notifies */

    /* disk status */
    LONG disk_inserted;
    LONG disk_inhibited;

    /* Character sets translation */
    UBYTE from_unicode[65536];
    UWORD to_unicode[256];
};

#include "support.h"

/* new definitions as we refactor the code */

/* get the first sector of a cluster */
#define SECTOR_FROM_CLUSTER(sb,cl) ((ULONG) (((cl-2) << sb->cluster_sectors_bits) + sb->first_data_sector))

#define FIRST_FILE_CLUSTER(de)                                       \
    (AROS_LE2WORD((de)->e.entry.first_cluster_lo) |                  \
     (((ULONG) AROS_LE2WORD((de)->e.entry.first_cluster_hi)) << 16))

#define RESET_HANDLE(ioh)                                          \
    do {                                                           \
        (ioh)->cluster_offset = (ioh)->sector_offset = 0xffffffff; \
        if ((ioh)->block != NULL) {                                \
            Cache_FreeBlock((ioh)->sb->cache, (ioh)->block);       \
            (ioh)->block = NULL;                                   \
        }                                                          \
    } while (0);

#define RESET_DIRHANDLE(dh)         \
    do {                            \
        RESET_HANDLE(&((dh)->ioh));   \
        (dh)->cur_index = 0xffffffff; \
    } while(0);

#define GET_NEXT_CLUSTER(sb,cl)     (sb->func_get_fat_entry(sb,cl))
#define SET_NEXT_CLUSTER(sb,cl,val) (sb->func_set_fat_entry(sb,cl,val))

#define CALC_SHORT_NAME_CHECKSUM(name,checksum)                                 \
    do {                                                                        \
        ULONG i;                                                                \
        checksum = 0;                                                           \
        for (i = 0; i < 11; i++)                                                \
            checksum = ((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + name[i]; \
    } while(0);

#define LOCKFROMNODE(A) \
    ((struct ExtFileLock *) \
    (((BYTE *)(A)) - (IPTR)&((struct ExtFileLock *)NULL)->node))

#endif
