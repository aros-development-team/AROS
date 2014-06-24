/* $Id$
 * $Log: blocks.h $
 * Revision 1.21  1999/06/13  16:46:01  Michiel
 * MAXNUMRESERVED raised
 *
 * Revision 1.20  1999/05/14  11:31:34  Michiel
 * Long filename support implemented; bugfixes
 *
 * Revision 1.19  1999/03/09  10:40:11  Michiel
 * Deldir extension and bitwise reserved roving
 *
 * Revision 1.18  1998/09/27  11:26:37  Michiel
 * MAXNUMRESERVED increased
 * BETA id changed
 *
 * Revision 1.17  1998/09/03  07:12:14  Michiel
 * version 17.4
 * bugfixes 118, 121, 123 and superindexblocks and td64 support
 *
 * Revision 1.16  1998/08/05  00:33:23  Michiel
 * MAXDISKSIZE, ID_MUPFS_DISK added
 *
 * Revision 1.15  1998/05/29  19:13:27  Michiel
 * Wrongly defined ID_PFS2_DISK
 *
 * Revision 1.14  1998/05/27  22:35:51  Michiel
 * Version 17.1 update
 *
 * Revision 1.12  1998/05/22  22:59:58  Michiel
 * Datestamps added
 *
 * Revision 1.11  1998/05/22  21:00:22  Michiel
 * Added anode roving pointer (overdue checkin)
 *
 * Revision 1.10  1995/12/29  11:01:53  Michiel
 * rollover extrafields added
 * reserved roving stored in rootblockextension
 *
 * Revision 1.9  1995/11/15  15:38:27  Michiel
 * rootblock extension defined
 * cachedblock 'done' field removed
 *
 * Revision 1.8  1995/11/07  17:31:22  Michiel
 * cachedblock changeflag and done now are bytes (instead of words)
 *
 * Revision 1.7  1995/08/04  04:15:30  Michiel
 * added DELENTRY_PROT_AND_MASK and DELENTRY_PROT_OR_MASK
 * added disksize (MODE_SIZEFIELD) to rootblock
 *
 * Revision 1.6  1995/07/28  08:26:55  Michiel
 * FNSIZE replaced by PATHSIZE and FILENAMESIZE
 *
 * Revision 1.5  1995/07/21  06:42:07  Michiel
 * More deldir stuff
 *
 * Revision 1.4  1995/07/11  09:24:38  Michiel
 * DELDIR stuff
 *
 * Revision 1.3  1995/07/10  15:17:13  Michiel
 * ID_MUAF_DISK and ID_AFS_DISK added
 *
 * Revision 1.2  1995/02/28  18:29:28  Michiel
 * extrafields added
 *
 * Revision 1.1  1995/02/15  16:46:45  Michiel
 * Initial revision
 *
 */

#ifndef _BLOCKS_H
#define _BLOCKS_H 1

#ifndef DOS_DOSEXTENS_H
#include <dos/dosextens.h>
#endif
#ifndef DEVICES_TRACKDISK_H
#include <devices/trackdisk.h>
#endif
#ifndef PROTO_EXEC_H
#include <proto/exec.h>
#endif
#ifndef PROTO_DOS_H
#include <proto/dos.h>
#endif
#ifndef _STDDEFH
#include <stddef.h>
#endif

#if defined(__GNUC__) || defined(__VBCC__)
/* Force SAS/C compatible alignment rules for all these structures */
#pragma pack(2)
#endif

struct bootblock
{
    LONG disktype;          /* PFS\1                            */
    UBYTE reserved[508];
};

typedef struct rootblock
{
    LONG disktype;
    ULONG options;          /* bit 0 is harddisk mode           */
    ULONG datestamp;        /* current datestamp */
    UWORD creationday;      /* days since Jan. 1, 1978 (like ADOS; WORD instead of LONG) */
    UWORD creationminute;   /* minutes past modnight            */
    UWORD creationtick;     /* ticks past minute                */
    UWORD protection;       /* protection bits (ala ADOS)       */
    UBYTE diskname[32];     /* disk label (pascal string)       */
    ULONG lastreserved;     /* reserved area. blocknumbers      */
    ULONG firstreserved;
    ULONG reserved_free;    /* number of reserved blocks (blksize blocks) free  */
    UWORD reserved_blksize;          /* size of reserved blocks in bytes */
    UWORD rblkcluster;      /* number of blocks in rootblock, including bitmap  */
    ULONG blocksfree;       /* blocks free                      */
    ULONG alwaysfree;       /* minimum number of blocks free    */
    ULONG roving_ptr;       /* current LONG bitmapfield nr for allocation       */
    ULONG deldir;           /* deldir location (<= 17.8)        */
    ULONG disksize;         /* disksize in sectors              */
    ULONG extension;        /* rootblock extension (16.4)       */
    ULONG not_used;
    union
    {
        UWORD anodeblocks[208];         /* SMALL: 200*84 = 16800 anodes */
        struct
        {
            ULONG bitmapindex[5];       /* 5 bitmap indexblocks with 253 bitmap blocks each */
            ULONG indexblocks[99];      /* 99 index blocks with 253 anode blocks each       */
        } small;
        struct 
        {
            ULONG bitmapindex[104];		/* 104 bitmap indexblocks = max 104 G */
        } large;
    } idx;
} rootblock_t;


/* structure for both normal as reserved bitmap
 * normal: normal clustersize
 * reserved: directly behind rootblock. As long as necessary
 */
typedef struct bitmapblock
{
    UWORD id;               /* BM (bitmap block)                */
    UWORD not_used;
    ULONG datestamp;
    ULONG seqnr;
    ULONG bitmap[0];        /* the bitmap.                      */
} bitmapblock_t;

#if defined(__GNUC__) || defined(__VBCC__)
#pragma pack()
#endif

typedef struct cbitmapblock
{
	struct cbitmapblock *next;
	struct cbitmapblock *prev;
	struct volumedata *volume;
	ULONG blocknr;
	ULONG oldblocknr;
	UWORD used;
	UBYTE changeflag;
	UBYTE dummy;
	struct bitmapblock blk;
} cbitmapblock_t;

#if defined(__GNUC__) || defined(__VBCC__)
#pragma pack(2)
#endif

typedef struct indexblock
{
    UWORD id;               /* AI or BI (anode- bitmap index)   */
    UWORD not_used;
    ULONG datestamp;
    ULONG seqnr;
    LONG index[0];          /* the indices                      */
} indexblock_t;

#if defined(__GNUC__) || defined(__VBCC__)
#pragma pack()
#endif

typedef struct cindexblock
{
	struct cindexblock *next;
	struct cindexblock *prev;
	struct volumedata *volume;
	ULONG blocknr;
	ULONG oldblocknr;
	UWORD used;
	UBYTE changeflag;
	UBYTE dummy;
	struct indexblock blk;
} cindexblock_t;

#if defined(__GNUC__) || defined(__VBCC__)
#pragma pack(2)
#endif

typedef struct anode
{
    ULONG clustersize;
    ULONG blocknr;
    ULONG next;
} anode_t;

typedef struct
{
	UWORD seqnr;
	UWORD offset;
} anodenr_t;

typedef struct anodeblock
{
    UWORD id;               /* AB                               */
    UWORD not_used;
    ULONG datestamp;
    ULONG seqnr;
    ULONG not_used_2;
    struct anode nodes[0];
} anodeblock_t;

#if defined(__GNUC__) || defined(__VBCC__)
#pragma pack()
#endif

struct canodeblock
{
	struct canodeblock *next;	/* for cache list				*/
	struct canodeblock *prev;
	struct volumedata *volume;
	ULONG  blocknr;				/* for already cached check		*/
	ULONG  oldblocknr;
	UWORD  used;				/* locknr						*/
	UBYTE  changeflag;			/* dirty flag					*/
	UBYTE  dummy;
	struct anodeblock blk;
};

#if defined(__GNUC__) || defined(__VBCC__)
#pragma pack(2)
#endif

// Cached Allocation NODE
typedef struct canode
{
	ULONG clustersize;	// number of blocks in a cluster
	ULONG blocknr;		// the block number
	ULONG next;			// next anode (anodenummer), 0 = eof
	ULONG nr;			// the anodenr
} canode_t;

struct dirblock 
{
    UWORD id;               /* 'DB'                             */
    UWORD not_used;
    ULONG datestamp;
    UWORD not_used_2[2];
    ULONG anodenr;          /* anodenr belonging to this directory (points to FIRST block of dir) */
    ULONG parent;           /* parent                           */
    UBYTE entries[0];       /* entries                          */
};

#if defined(__GNUC__) || defined(__VBCC__)
#pragma pack()
#endif

struct cdirblock
{
	struct cdirblock *next;		// volume cachelist
	struct cdirblock *prev;	
	struct volumedata *volume;
	ULONG  blocknr;		// voor already cached check
	ULONG  oldblocknr;
	UWORD  used;
	UBYTE  changeflag;	// size is word, could be byte
	UBYTE  dummy;
	struct dirblock blk;
};

struct lru_cdirblock
{
	struct lru_cdirblock *next;
	struct lru_cdirblock *prev;
	struct cdirblock cblk;
};

#if defined(__GNUC__) || defined(__VBCC__)
#pragma pack(2)
#endif

struct direntry
{
    UBYTE next;             /* sizeof direntry                  */
    BYTE  type;             /* dir, file, link etc              */
    ULONG anode;            /* anode number                     */
    ULONG fsize;            /* sizeof file                      */
    UWORD creationday;      /* days since Jan. 1, 1978 (like ADOS; WORD instead of LONG) */
    UWORD creationminute;   /* minutes past modnight            */
    UWORD creationtick;     /* ticks past minute                */
    UBYTE protection;       /* protection bits (like DOS)       */
    UBYTE nlength;          /* lenght of filename               */
    UBYTE startofname;      /* filename, followed by filenote length & filenote */
    UBYTE pad;              /* make size even                   */
};

struct extrafields
{
	ULONG link;				/* link anodenr						*/
	UWORD uid;				/* user id							*/
	UWORD gid;				/* group id							*/
	ULONG prot;				/* byte 1-3 of protection			*/
	// rollover fields
	ULONG virtualsize;		/* virtual rollover filesize in bytes (as shown by Examine()) */
	ULONG rollpointer;		/* current start of file AND end of file pointer */
	// extended file size
	UWORD fsizex;           /* extended bits 32-47 of direntry.fsize */
};

#if defined(__GNUC__) || defined(__VBCC__)
#pragma pack()
#endif

#if DELDIR
/*
 * del dir
 */

#if defined(__GNUC__) || defined(__VBCC__)
#pragma pack(2)
#endif

struct deldirentry
{
	ULONG anodenr;			/* anodenr							*/
	ULONG fsize;			/* size of file						*/
	UWORD creationday;		/* datestamp						*/
	UWORD creationminute;
	UWORD creationtick;
	UBYTE filename[16];		/* filename; filling up to 30 chars	*/
	// was previously filename[18]
	// now last two bytes used for extended file size
	UWORD fsizex;			/* extended bits 32-47 of fsize		*/
};

struct deldirblock
{
	UWORD id;				/* 'DD'								*/
	UWORD not_used;
	ULONG datestamp;
	ULONG seqnr;
	UWORD not_used_2[2];
	UWORD not_used_3;		/* roving in older versions	(<17.9)	*/	
	UWORD uid;				/* user id							*/
	UWORD gid;				/* group id							*/
	ULONG protection;
	UWORD creationday;
	UWORD creationminute;
	UWORD creationtick;
	struct deldirentry entries[0];	/* 31 entries				*/
};

#if defined(__GNUC__) || defined(__VBCC__)
#pragma pack()
#endif

struct cdeldirblock
{
	struct cdeldirblock *next;	/* currently not used 			*/
	struct cdeldirblock *prev;	/* currently not used			*/
	struct volumedata *volume;
	ULONG blocknr;
	ULONG oldblocknr;
	UWORD used;
	UBYTE changeflag;
	UBYTE dummy;
	struct deldirblock blk;
};

#endif

#if VERSION23

#if defined(__GNUC__) || defined(__VBCC__)
#pragma pack(2)
#endif

struct postponed_op
{
	ULONG operation_id;		/* which operation is postponed */
	ULONG argument1;		/* operation arguments, e.g. number of blocks */
	ULONG argument2;
	ULONG argument3;
};

struct rootblockextension
{
	UWORD id;					/* id ('EX') */
	UWORD not_used_1;
	ULONG ext_options;
	ULONG datestamp;
	ULONG pfs2version;			/* pfs2 revision under which the disk was formatted */
	UWORD root_date[3];			/* root directory datestamp */
	UWORD volume_date[3];		/* volume datestamp */
	struct postponed_op tobedone;	/* postponed operation (curr. only delete) */
	ULONG reserved_roving;		/* reserved roving pointer */
	UWORD rovingbit;			/* bitnr in rootblock->roving_ptr bitmap field */
	UWORD curranseqnr;			/* anodeallocation roving pointer */
	UWORD deldirroving;			/* deldir roving pointer */
	UWORD deldirsize;			/* size of deldir */
	UWORD fnsize;				/* filename size (18.1) */
	UWORD not_used_2[3];
	ULONG superindex[16];		/* MODE_SUPERINDEX only. */
	UWORD dd_uid;				/* deldir user id (17.9)			*/
	UWORD dd_gid;				/* deldir group id					*/
	ULONG dd_protection;		/* deldir protection				*/
	UWORD dd_creationday;		/* deldir datestamp					*/
	UWORD dd_creationminute;
	UWORD dd_creationtick;
	UWORD not_used_3;
	ULONG deldir[32];			/* 32 deldir blocks					*/
};

#if defined(__GNUC__) || defined(__VBCC__)
#pragma pack()
#endif

struct crootblockextension
{
	struct crootblockextension *next;	/* currently not used */
	struct crootblockextension *prev;	/* currently not used */
	struct volumedata *volume;
	ULONG blocknr;
	ULONG oldblocknr;
	UWORD used;
	UBYTE changeflag;
	UBYTE dummy;
	struct rootblockextension blk;
};

typedef struct crootblockextension crootblockextension_t;

/* postponed operations operation_id's */
#define PP_FREEBLOCKS_FREE 1
#define PP_FREEBLOCKS_KEEP 2
#define PP_FREEANODECHAIN 3

/* */
#endif

/* Cached blocks in general
*/
struct cachedblock
{
	struct cachedblock	*next;
	struct cachedblock	*prev;
	struct volumedata	*volume;
	ULONG	blocknr;				// the current (new) blocknumber of the block
	ULONG	oldblocknr;				// the blocknr before reallocation. NULL if not reallocated.
	UWORD	used;					// block locked if used == g->locknr
	UBYTE	changeflag;				// dirtyflag
	UBYTE	dummy;					// pad to make offset even
	UBYTE	data[0];				// the datablock;
};

struct lru_cachedblock
{
	struct lru_cachedblock *next;
	struct lru_cachedblock *prev;
	struct cachedblock cblk;
};


/* max length of filename, diskname and comment
 * FNSIZE is 108 for compatibilty. Used for searching
 * files.
 */
#define FNSIZE 108
#define PATHSIZE 256
#define FILENAMESIZE (g->fnsize)
#define DNSIZE 32
#define CMSIZE 80
#define MAX_ENTRYSIZE (sizeof(struct direntry) + FNSIZE + CMSIZE + 34)

/* limits */
#define MAXSMALLBITMAPINDEX 4
#define MAXBITMAPINDEX 103
// was 28576. was 119837. Nu max reserved bitmap 256K.
#define MAXNUMRESERVED (4096 + 255*1024*8)
#define MAXSUPER 15
#define MAXSMALLINDEXNR 98
#if LARGE_FILE_SIZE
// last two bytes used for extended file size
#define DELENTRYFNSIZE 16
#else
#define DELENTRYFNSIZE 18
#endif
/* maximum disksize in sectors, limited by number of bitmapindexblocks
 * smalldisk = 10.241.440 blocks of 512 byte = 5G
 * normaldisk = 213.021.952 blocks of 512 byte = 104G
 * 2k reserved blocks = 104*509*509*32 blocks of 512 byte = 411G
 * 4k reserved blocks = 1,6T
 *  */
#define MAXSMALLDISK (5*253*253*32)
#define MAXDISKSIZE1K (104*253*253*32)
#define MAXDISKSIZE2K (104*509*509*32)
#define MAXDISKSIZE4K ((ULONG)104*1021*1021*32)
#define MAXDISKSIZE MAXDISKSIZE4K

/* disk id 'PFS\1'  */
//#ifdef BETAVERSION
//#define ID_PFS_DISK		(0x42455441L)	/*	'BETA'	*/
//#else
#define ID_PFS_DISK		(0x50465301L)	/*  'PFS\1' */
//#endif
#define ID_BUSY			(0x42555359L)	/*	'BUSY'  */

#define ID_MUAF_DISK	(0x6d754146L)	/*	'muAF'	*/
#define ID_MUPFS_DISK	(0x6d755046L)	/*	'muPF'	*/
#define ID_AFS_DISK		(0x41465301L)	/*	'AFS\1' */
#define ID_PFS2_DISK	(0x50465302L)	/*	'PFS\2'	*/
#define ID_AFS_USER_TEST (0x41465355L)	/*	'AFSU'	*/

/* block id's		*/
#define DBLKID 0x4442
#define ABLKID 0x4142
#define IBLKID 0x4942
#define BMBLKID 0x424D
#define BMIBLKID 0x4D49
#define DELDIRID 0x4444
#define EXTENSIONID 0x4558	/* 'EX' */
#define SBLKID 0x5342	/* 'SB' */

/* macros on cachedblocks */
#define IsDirBlock(blk) (((UWORD *)(blk->data))[0] == DBLKID)
#define IsAnodeBlock(blk) (((UWORD *)(blk->data))[0] == ABLKID)
#define IsIndexBlock(blk) (((UWORD *)(blk->data))[0] == IBLKID)
#define IsBitmapBlock(blk) (((UWORD *)(blk->data))[0] == BMBLKID)
#define IsBitmapIndexBlock(blk) (((UWORD *)(blk->data))[0] == BMIBLKID)
#define IsDeldir(blk) (((UWORD *)(blk->data))[0] == DELDIRID)
#define IsSuperBlock(blk) (((UWORD *)(blk->data))[0] == SBLKID)

/* size of reserved blocks in bytes and blocks
 * place you can find rootblock
 */
#define SIZEOF_RESBLOCK (g->rootblock->reserved_blksize)
#define SIZEOF_CACHEDBLOCK (sizeof(struct cachedblock) + SIZEOF_RESBLOCK)
#define SIZEOF_LRUBLOCK (sizeof(struct lru_cachedblock) + SIZEOF_RESBLOCK)
#define RESCLUSTER (g->currentvolume->rescluster)
#define BOOTBLOCK1 0
#define BOOTBLOCK2 1
#define ROOTBLOCK 2

/* Longs per bitmapblock */
#define LONGS_PER_BMB ((g->rootblock->reserved_blksize/4)-3)

/* get filenote from directory entry */
#define FILENOTE(de) ((UBYTE*)(&((de)->startofname) + (de)->nlength))

/* get next directory entry */
#define NEXTENTRY(de) ((struct direntry*)((UBYTE*)(de) + (de)->next))
#define DB_HEADSPACE (sizeof(struct dirblock))
#define DB_ENTRYSPACE (SIZEOF_RESBLOCK - sizeof(struct dirblock))

/* disk options */
#define MODE_HARDDISK	1
#define MODE_SPLITTED_ANODES 2
#define MODE_DIR_EXTENSION 4
#define MODE_DELDIR 8
#define MODE_SIZEFIELD 16
// rootblock extension
#define MODE_EXTENSION 32
// if enabled the datestamp was on at format time (!)
#define MODE_DATESTAMP 64
#define MODE_SUPERINDEX 128
#define MODE_SUPERDELDIR 256
#define MODE_EXTROVING 512
#define MODE_LONGFN 1024
#define MODE_LARGEFILE 2048

/* direntry macros */
// comment: de is struct direntry *
#define COMMENT(de) ((UBYTE*)(&((de)->startofname) + (de)->nlength))
/* DIRENTRYNAME: DSTR of name. */
#define DIRENTRYNAME(de) ((UBYTE*)(&(de->nlength)))
// get a pointer to the next direntry; de = struct direntry *
#define NEXTENTRY(de) ((struct direntry*)((UBYTE*)(de) + (de)->next))
// get first direntry; blok is dirblock
#define FIRSTENTRY(blok) ((struct direntry*)((blok)->blk.entries))
#define ENTRYLEN(de, comment) ((sizeof(struct direntry) + (de)->nlength + strlen(comment))&0xfffe)

#if DELDIR
#define DELENTRY_SEP '@'
#define DELENTRY_PROT 0x0005
#define DELENTRY_PROT_AND_MASK 0xaa0f
#define DELENTRY_PROT_OR_MASK 0x0005
/* maximum number of entries per block, max deldirblock seqnr */
#define DELENTRIES_PER_BLOCK 31
#define MAXDELDIR 31
#endif /* DELDIR */

#if ROLLOVER
#define ST_ROLLOVERFILE -16
#endif

/* predefined anodes */
#define ANODE_EOF			0
#define ANODE_RESERVED_1	1	// not used by MODE_BIG
#define ANODE_RESERVED_2	2	// not used by MODE_BIG
#define ANODE_RESERVED_3	3	// not used by MODE_BIG
#define ANODE_BADBLOCKS		4	// not used yet
#define ANODE_ROOTDIR		5
#define ANODE_USERFIRST		6

/* Max size reported in DOS ULONG size fields */
#define MAXFILESIZE32 0x7fffffff

#endif /* _BLOCKS_H */
