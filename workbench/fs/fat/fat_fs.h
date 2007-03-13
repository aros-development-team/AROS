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
struct Extent
{
	ULONG sector;
	ULONG count;
	ULONG offset;
	ULONG cur_cluster;
	ULONG next_cluster;
	ULONG start_cluster;
	ULONG last_cluster;
};

struct DirCache
{
	struct Extent *e;
	void *buffer;
	ULONG cur_sector;
};
 
#define fl_Key entry

#define FAT_ROOTDIR_MARK	0xFFFFFFFFlu

struct ExtFileLock
{
	/* struct FileLock */
	BPTR            fl_Link;
	ULONG  			entry;
	LONG            fl_Access;
	struct MsgPort *fl_Task;
	BPTR            fl_Volume;

	/* coinsistency check */
	ULONG 			magic;   

	/* my directory start cluster */
	ULONG 		    cluster;

	ULONG 			attr;
	ULONG 			size;
	ULONG 			first_cluster;

	struct Extent	data_ext[1];

	/* dir entry cache for easy and quick management of long files */

	/* used in directory scanning and file reading */
	ULONG 			pos;

	UBYTE 			name[108];

	BOOL 			dircache_active;
	struct DirCache dircache[1];
};


struct FSSuper {
	struct FSSuper *next;
	struct DosList *doslist;

	ULONG sectorsize;
	ULONG sectorsize_bits;

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

	struct Extent first_rootdir_extent[1];

	UBYTE name[32];	/* BCPL string */

	/* function table */
	ULONG (*func_get_fat_entry)(struct FSSuper *sb, ULONG n);
	/* ... */
};


struct Globals {
	/* mem/task */
    struct Task *ourtask;
    struct MsgPort *ourport;
	APTR mempool;

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

#endif
