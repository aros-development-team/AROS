/*
 * This file forms part of PFS3 (Professional File System 3)
 * Written by Michiel Pelt
 *
 * PFS3 header file
 */

#ifndef _PFS_H
#define _PFS_H 1

#ifndef EXEC_PORTS_H
#include "exec/ports.h"
#endif /* EXEC_PORTS_H */

#if defined(__GNUC__) || defined(__VBCC__)
/* Force SAS/C compatible alignment rules for all these structures */
#pragma pack(2)
#endif

/****************************************************************************/
/* PFS specific actions (packets)                                           */
/****************************************************************************/

#define ACTION_SLEEP 2200
#define ACTION_UPDATE_ANODE 2201
#define ACTION_PFS_INFO 2202
#define ACTION_PFS_CONFIG 2203
#define ACTION_REMOVE_DIRENTRY 2204
#define ACTION_CREATE_ROLLOVER 2205
#define ACTION_SET_ROLLOVER 2206
#define ACTION_IS_PFS2 2211
#define ACTION_ADD_IDLE_SIGNAL 2220
#define ACTION_SET_DELDIR 2221
#define ACTION_SET_FNSIZE 2222

/****************************************************************************/
/* PFS3 blocks                                                              */
/****************************************************************************/

typedef struct bootblock
{
    LONG disktype;          /* PFS\1 */
    UBYTE not_used[508];
} bootblock_t;

typedef struct rootblock
{
    LONG disktype;
    ULONG options;          /* bit 0 is harddisk mode */
    ULONG datestamp;
    UWORD creationday;      /* days since Jan. 1, 1978 (like ADOS; WORD instead of LONG) */
    UWORD creationminute;   /* minutes past midnight */
    UWORD creationtick;     /* ticks past minute */
    UWORD protection;       /* protection bits (ala ADOS) */
    UBYTE diskname[32];     /* disk label (pascal string) */
    ULONG lastreserved;     /* reserved area. blocknumbers */
    ULONG firstreserved;
    ULONG reserved_free;    /* number of reserved blocks (blksize blocks) free */
    UWORD reserved_blksize; /* size of reserved blocks in bytes */
    UWORD rblkcluster;      /* number of blocks in rootblock, including bitmap */
    ULONG blocksfree;       /* blocks free */
    ULONG alwaysfree;       /* minimum number of blocks free */
    ULONG roving_ptr;       /* current LONG bitmapfield nr for allocation */
    ULONG deldir;           /* (before 4.3) deldir location */
    ULONG disksize;         /* disksize in sectors */
    ULONG extension;        /* rootblock extension */
    ULONG not_used;
    union
    {
        struct
        {
            ULONG bitmapindex[5];   /* 5 bitmap indexblocks */
            ULONG indexblocks[99];  /* 99 indexblocks */
        } small;
        struct
        {
            ULONG bitmapindex[104]; /* 104 bitmap indexblock */
        } large;
    } idx;
} rootblock_t;


typedef struct rootblockextension
{
    UWORD id;                   /* id ('EX') */
    UWORD not_used_1;
    ULONG ext_options;
    ULONG datestamp;
    ULONG pfs2version;          /* pfs2 format revision */
    UWORD root_date[3];         /* root directory datestamp */
    UWORD volume_date[3];       /* volume datestamp */
    ULONG postponed_op[4];      /* postponed operation (private) */
    ULONG reserved_roving;      /* reserved roving pointer */
    UWORD rovingbit;            /* bitnr in rootblock->roving_ptr field */
    UWORD curranseqnr;          /* anode allocation roving pointer  */
    UWORD deldirroving;         /* (5.1) deldir roving pointer */
    UWORD deldirsize;           /* (5.1) size of deldir in blocks, 0 is disabled */
    UWORD fnsize;				/* (5.1) filename size */
    UWORD not_used_2[3];
    ULONG superindex[16];       /* (4.2) MODE_SUPERINDEX only */
    UWORD dd_uid;               /* (5.1) deldir user id (17.9) */
    UWORD dd_gid;               /* (5.1) deldir group id */
    ULONG dd_protection;        /* (5.1) deldir protection */
    UWORD dd_creationday;       /* (5.1) deldir datestamp */
    UWORD dd_creationminute;
    UWORD dd_creationtick;
    UWORD not_used_3;
    ULONG deldir[32];           /* (5.1) 32 deldir blocks */
    ULONG not_used_4[188];
} extensionblock_t;

/* structure for both normal as reserved bitmap
 * normal: normal clustersize
 * reserved: directly behind rootblock. As long as necessary.
 */
typedef struct bitmapblock
{
    UWORD id;               /* 'BM' (bitmap block) */
    UWORD not_used;
    ULONG datestamp;
    ULONG seqnr;
    ULONG bitmap[253];      /* the bitmap. */
} bitmapblock_t;

typedef struct indexblock
{
    UWORD id;               /* 'AI', 'BI', or 'SB' */
    UWORD not_used;
    ULONG datestamp;
    ULONG seqnr;
    LONG index[253];        /* the indices */
} indexblock_t;

typedef struct
{
	UWORD seqnr;
	UWORD offset;
} anodenr_t;

typedef struct anode
{
    ULONG clustersize;
    ULONG blocknr;
    ULONG next;
} anode_t;

typedef struct anodeblock
{
    UWORD id;               /* 'AB' */
    UWORD not_used;
    ULONG datestamp;
    ULONG seqnr;
    ULONG not_used_2;
    struct anode nodes[84];
} anodeblock_t;

typedef struct dirblock 
{
    UWORD id;               /* 'DB' */  
    UWORD not_used;
    ULONG datestamp;
    UWORD not_used_2[2];
    ULONG anodenr;          /* anodenr belonging to this directory (points to FIRST block of dir) */
    ULONG parent;           /* parent */
    UBYTE entries[0];       /* entries */
} dirblock_t;

struct direntry
{
    UBYTE next;             /* sizeof direntry */
    BYTE  type;             /* dir, file, link etc */
    ULONG anode;            /* anode nummer */
    ULONG fsize;            /* sizeof file */
    UWORD creationday;      /* days since Jan. 1, 1978 (like ADOS; WORD instead of LONG) */
    UWORD creationminute;   /* minutes past modnight */
    UWORD creationtick;     /* ticks past minute */
    UBYTE protection;       /* protection bits (like DOS) */
    UBYTE nlength;          /* lenght of filename */
    UBYTE startofname;      /* filename, followed by filenote length & filenote */
    UBYTE pad;              /* make size even */
};

struct extrafields
{
    ULONG link;             /* link anodenr */
    UWORD uid;              /* user id */
    UWORD gid;              /* group id */
    ULONG prot;             /* byte 1-3 of protection */
    ULONG virtualsize;      /* virtual rollover filesize */
    ULONG rollpointer;      /* rollover fileoffset */
    UWORD fsizex;           /* extended bits 32-47 of direntry.fsize */
};

struct deldirentry
{
    ULONG anodenr;          /* anodenr */
    ULONG fsize;            /* size of file */
    UWORD creationday;      /* datestamp */
    UWORD creationminute;
    UWORD creationtick;
    UBYTE filename[16];     /* filename; filling up to 30 chars */
    // was previously filename[18]
    // now last two bytes used for extended file size
    UWORD fsizex;           /* extended bits 32-47 of fsize		*/
};

typedef struct deldirblock
{
    UWORD id;               /* 'DD' */
    UWORD not_used;
    ULONG datestamp;
    ULONG seqnr;
    UWORD not_used_2[3];
    UWORD uid;              /* user id */
    UWORD gid;              /* group id */
    ULONG protection;
    UWORD creationday;
    UWORD creationminute;
    UWORD creationtick;
    struct deldirentry entries[0];  /* 31 entries */
} deldirblock_t;

union reservedblock
{
    UWORD            id;

    anodeblock_t     anodeblock;
    bitmapblock_t    btmapblock;
    bootblock_t      bootblock;
    dirblock_t       dirblock;
    deldirblock_t    deldirblock;
    indexblock_t     indexblock;
    rootblock_t      rootblock;
    extensionblock_t extensionblock;
};

typedef union reservedblock reservedblock_t;

/* limits */
#define MAXSMALLBITMAPINDEX  4
#define MAXBITMAPINDEX     103
#define MAXNUMRESERVED (4096 + 255*1024*8)
#define MAXSUPER            15
#define MAXSMALLINDEXNR     98
#define MAXDELDIRSEQNR      31

/* maximum disksize in blocks, limited by number of bitmapindexblocks */
#define MAXSMALLDISK (5*253*253*32)
#define MAXDISKSIZE1K (104*253*253*32)
#define MAXDISKSIZE2K (104*509*509*32)
#define MAXDISKSIZE4K ((ULONG)104*1021*1021*32)

#define MAXRESBLOCKSIZE 4096

/* disk id 'PFS\1'  */
#define ID_PFS_DISK     (0x50465301L)   /* 'PFS\1' */
#define ID_PFS2_DISK    (0x50465302L)   /* 'PFS\2' */
#define ID_MUPFS_DISK   (0x6d755046L)   /* 'muPF'  */
#define ID_AFS_DISK     (0x41465301L)   /* 'AFS\1' */

/* block id's */
#define DBLKID          0x4442
#define ABLKID          0x4142
#define IBLKID          0x4942
#define BMBLKID         0x424D
#define BMIBLKID        0x4D49
#define DELDIRID        0x4444
#define EXTENSIONID     0x4558
#define SBLKID          0x5342
#define SIZEOF_RESBLOCK (rbl->reserved_blksize)


/* predefined anodes */
#define ANODE_EOF            0
#define ANODE_BADBLOCKS      4
#define ANODE_ROOTDIR        5
#define ANODE_USERFIRST      6

/* disk options */
#define MODE_HARDDISK        1
#define MODE_SPLITTED_ANODES 2
#define MODE_DIR_EXTENSION   4
#define MODE_DELDIR          8
#define MODE_SIZEFIELD      16
#define MODE_EXTENSION      32
#define MODE_DATESTAMP      64
#define MODE_SUPERINDEX    128
#define MODE_SUPERDELDIR   256
#define MODE_EXTROVING     512
#define MODE_LONGFN       1024
#define MODE_LARGEFILE    2048
#define MODE_MASK        (4096-1)

/* seperator used for deldirentries */
#define DELENTRY_SEP '@'

/* rollover filetype */
#define ST_ROLLOVERFILE -16

/* get filenote from directory entry */
#define FILENOTE(de) ((UBYTE*)(&((de)->startofname) + (de)->nlength))

/* get next directory entry */
#define NEXTENTRY(de) ((struct direntry*)((UBYTE*)(de) + (de)->next))

#if defined(__GNUC__) || defined(__VBCC__)
#pragma pack()
#endif

#endif /* _PFS_H */
