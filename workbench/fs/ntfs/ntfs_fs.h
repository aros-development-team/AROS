/*
 * ntfs.handler - New Technology FileSystem handler
 *
 * Copyright © 2012 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id $
 */

#ifndef NTFS_FS_H
#define NTFS_FS_H

//#define DEBUG 1

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
#include <libraries/uuid.h>

#include "ntfs_struct.h"

#include "cache.h"

#define NTFS_READONLY

extern struct Globals 		*glob;

/* filesystem structures */
#define ID_NTFS_DISK 		0x4E544653UL

#define ACTION_VOLUME_ADD 	16000
#define ACTION_VOLUME_REMOVE 	16001

#define DEF_POOL_SIZE 		65536
#define DEF_POOL_THRESHOLD 	DEF_POOL_SIZE
#define DEF_BUFF_LINES		128
#define DEF_READ_AHEAD 		16*1024

#define SECTORSIZE_SHIFT 	9

#define RLEFLAG_COMPR		(1 << 0)
#define RLEFLAG_SPARSE		(1 << 1)

struct NTFSMFTAttr
{
    struct MFTAttr 		*emft_buf;
    struct MFTAttr 		*edat_buf;
    struct MFTAttr 		*attr_cur;
    struct MFTAttr 		*attr_nxt;
    struct MFTAttr 		*attr_end;
    struct NTFSMFTEntry 	*mft;
    struct MFTAttr 		*sbuf;
    ULONG 			save_pos;
    UBYTE 			flags;
};

struct NTFSMFTEntry
{
    struct FSData 		*data;
    APTR 			cblock;			/* current block from the cache */
    UBYTE 			*cbuf;			/* current data buffer (from cache) */
    UBYTE 			*buf;			/* MFT Entry Buffer */
    UQUAD 			size;
    UQUAD 			mftrec_no;
    struct NTFSMFTAttr 		attr;
    UBYTE 			buf_filled;
};

struct NTFSRunLstEntry
{
    struct NTFSMFTAttr 		*attr;
    UBYTE 			*mappingpair;
    UQUAD 			target_vcn;
    UQUAD 			curr_vcn;
    UQUAD 			next_vcn;
    UQUAD 			curr_lcn;
    UBYTE 			flags;
 };

/* a handle on something, file or directory */
struct IOHandle {
    struct FSData      		*data;            	/* filesystem data */

    UQUAD               	first_cluster;  	/* first cluster of this file */
    
    struct NTFSMFTEntry 	mft;
    UBYTE			*bitmap;
    UQUAD			bitmap_len;
};

/* a handle on a directory */
struct DirHandle {
    struct IOHandle		ioh;
    UQUAD 			parent_mft;
    UBYTE			*idx_root;
    struct NTFSMFTAttr		idx_attr;
    ULONG               	cur_no;			/* last entry returned, for GetNextDirEntry */
};

struct Index_Key {
    UBYTE 			*indx;
    UBYTE			*bitmap;
    UBYTE			*pos;			/* byte offset within index record that the entry came from */
    ULONG               	current;		/* index no. of this entry */
    UQUAD			i;
    UWORD 			v;
};

/* single directory entry */
struct DirEntry {
    struct FSData      		*data;			/* filesystem data */
    struct Index_Key		*key;
    struct NTFSMFTEntry		*entry;
    char			*entryname;
    ULONG			entrytype;
    ULONG			no;
    UQUAD               	cluster;		/* cluster the containing directory starts at */
};

struct GlobalLock;

struct ExtFileLock {
    /* struct FileLock */
    BPTR                	fl_Link;
    IPTR                	fl_Key;
    LONG                	fl_Access;
    struct MsgPort 		*fl_Task;
    BPTR                	fl_Volume;

    ULONG               	magic;			/* we set this to ID_NTFS_DISK so we can tell
							    our locks from those of other filesystems */

    struct FSData      		*data;			/* pointer to data, for unlocking when volume is removed */
    struct GlobalLock   	*gl;			/* pointer to the global lock for this file */
    struct MinNode      	node;			/* node in the list of locks attached to the global lock */

    struct DirHandle    	*dir;			/* handle for reads and writes */
    struct DirEntry    		*entry;			/* handle for reads and writes */
    UQUAD               	pos;			/* current seek position within the file */

    BOOL                	do_notify;		/* if set, send notification on file close (ACTION_END) */
};

struct GlobalLock {
    struct MinNode      node;

    ULONG               dir_cluster;    		/* first cluster of the directory we're in */
    ULONG               dir_entry;      		/* this is our dir entry within dir_cluster */

    LONG                access;         		/* access mode, shared or exclusive */

    ULONG               first_cluster;  		/* first data cluster */

    ULONG               attr;           		/* file attributes, from the dir entry */
    UQUAD               size;           		/* file size, from the dir entry */

    UBYTE 		name[256];  			/* copy of the name (bstr) */

    struct MinList      locks;          		/* list of ExtFileLocks opened on this file */
};

/* a node in the list of notification requests */
struct NotifyNode {
    struct MinNode          node;

    struct GlobalLock       *gl;        		/* pointer to global lock if this file is
							    locked. if it's not, this is NULL */

    struct NotifyRequest    *nr;        		/* the request that DOS passed us */
};

struct VolumeInfo {
    struct MinList	locks;           		/* global locks */
    struct MinList	notifies;
    APTR		mem_pool;
    struct GlobalLock	root_lock;
    uuid_t		uuid;
};

struct VolumeIdentity {
    UBYTE               name[128];     			/* BCPL string */
    struct DateStamp    create_time;
};

struct FSData {
    struct Node			node;
    struct DosList		*doslist;

    struct VolumeInfo		*info;

    struct cache		*cache;
    ULONG			first_device_sector;

    ULONG			sectorsize;
    ULONG			sectorsize_bits;

    ULONG			cluster_sectors;
    ULONG			clustersize;
    ULONG			clustersize_bits;
    ULONG			cluster_sectors_bits;

    struct NTFSMFTEntry		mft;    		/* Handle for $MFT access */
    struct NTFSMFTEntry		bmmft;  		/* Handle for $Bitmap access */
    struct NTFSMFTEntry		logmft; 		/* Handle for $LogFile access */
    struct NTFSMFTEntry		secmft; 		/* Handle for $LogFile access */

    ULONG			idx_size;
    ULONG			mft_size;
    UQUAD			mft_start;

    uuid_t			uuid;

    struct VolumeIdentity	volume;

    UQUAD			total_sectors;
    UQUAD			used_sectors;
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
    struct FSData *data;    /* current data */
    struct MinList sblist;   /* sbs with outstanding locks or notifies */

    /* disk status */
    LONG disk_inserted;
    LONG disk_inhibited;

    /* Character sets translation */
    UBYTE from_unicode[65536];
    UWORD to_unicode[256];
};

//#include "support.h"

/* new definitions as we refactor the code */

#define RESET_HANDLE(ioh)                                          \
    do {                                                           \
        if ((ioh)->mft.cblock != NULL) {                                \
            Cache_FreeBlock((ioh)->data->cache, (ioh)->mft.cblock);       \
            (ioh)->mft.cblock = NULL;                                   \
        }                                                          \
    } while (0);

#define RESET_DIRHANDLE(dh)         \
    do {                            \
        RESET_HANDLE(&((dh)->ioh));   \
        (dh)->cur_no = -1; \
    } while(0);

#define LOCKFROMNODE(A) \
    ((struct ExtFileLock *) \
    (((BYTE *)(A)) - (IPTR)&((struct ExtFileLock *)NULL)->node))

#define INIT_MFTATTRIB(attrib, mftentry) \
    { \
	struct NTFSMFTAttr *tmpattr = (struct NTFSMFTAttr *)attrib; \
	tmpattr->mft = mftentry;   \
	tmpattr->flags = (tmpattr->mft == &tmpattr->mft->data->mft) ? AF_MMFT : 0;   \
	tmpattr->attr_nxt = (struct MFTAttr *)(tmpattr->mft->buf + AROS_LE2WORD(*((UWORD *)(tmpattr->mft->buf + 0x14))));   \
	tmpattr->attr_end = tmpattr->emft_buf = tmpattr->edat_buf = tmpattr->sbuf = NULL; \
    }

#endif
