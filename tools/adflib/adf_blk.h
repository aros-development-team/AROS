/*
 *  ADF Library. (C) 1997-1999 Laurent Clevy
 *
 *  adf_blk.h
 *
 *  general blocks structures
 */


#ifndef ADF_BLK_H
#define ADF_BLK_H 1

#include "adf_defs.h"

#define LOGICAL_BLOCK_SIZE    512

/* ----- FILE SYSTEM ----- */

#define FSMASK_FFS         1
#define FSMASK_INTL        2
#define FSMASK_DIRCACHE    4

#define isFFS(c)           ((c)&FSMASK_FFS)
#define isOFS(c)           (!((c)&FSMASK_FFS))
#define isINTL(c)          ((c)&FSMASK_INTL)
#define isDIRCACHE(c)      ((c)&FSMASK_DIRCACHE)


/* ----- ENTRIES ----- */

/* access constants */

#define ACCMASK_D	(1<<0)
#define ACCMASK_E	(1<<1)
#define ACCMASK_W	(1<<2)
#define ACCMASK_R	(1<<3)
#define ACCMASK_A	(1<<4)
#define ACCMASK_P	(1<<5)
#define ACCMASK_S	(1<<6)
#define ACCMASK_H	(1<<7)

#define hasD(c)    ((c)&ACCMASK_D)
#define hasE(c)    ((c)&ACCMASK_E)
#define hasW(c)    ((c)&ACCMASK_W)
#define hasR(c)    ((c)&ACCMASK_R)
#define hasA(c)    ((c)&ACCMASK_A)
#define hasP(c)	   ((c)&ACCMASK_P)
#define hasS(c)    ((c)&ACCMASK_S)
#define hasH(c)    ((c)&ACCMASK_H)


/* ----- BLOCKS ----- */

/* block constants */

#define BM_VALID	-1
#define BM_INVALID	0

#define HT_SIZE		72
#define BM_SIZE     25
#define MAX_DATABLK	72

#define MAXNAMELEN	30
#define MAXCMMTLEN	79


/* block primary and secondary types */

#define T_HEADER	2
#define ST_ROOT		1
#define ST_DIR		2
#define ST_FILE		-3
#define ST_LFILE	-4
#define ST_LDIR		4
#define ST_LSOFT	3
#define T_LIST		16
#define T_DATA		8
#define T_DIRC		33


/*--- blocks structures --- */


struct bBootBlock {
/*000*/	TEXT	dosType[4];
/*004*/	ULONG	checkSum;
/*008*/	ULONG	rootBlock;
/*00c*/	UCHAR	data[500+512];
};


struct bRootBlock {
/*000*/	ULONG	type;
        ULONG	headerKey;
        ULONG	highSeq;
/*00c*/	ULONG	hashTableSize;
        ULONG	firstData;
/*014*/	ULONG	checkSum;
/*018*/	ULONG	hashTable[HT_SIZE];		/* hash table */
/*138*/	ULONG	bmFlag;				/* bitmap flag, -1 means VALID */
/*13c*/	ULONG	bmPages[BM_SIZE];
/*1a0*/	ULONG	bmExt;
/*1a4*/	ULONG	cDays; 	/* creation date FFS and OFS */
/*1a8*/	ULONG	cMins;
/*1ac*/	ULONG	cTicks;
/*1b0*/	UBYTE	nameLen;
/*1b1*/	TEXT 	diskName[MAXNAMELEN+1];
        UBYTE	r2[8];
/*1d8*/	ULONG	days;		/* last access : days after 1 jan 1978 */
/*1dc*/	ULONG	mins;		/* hours and minutes in minutes */
/*1e0*/	ULONG	ticks;		/* 1/50 seconds */
/*1e4*/	ULONG	coDays;	/* creation date OFS */
/*1e8*/	ULONG	coMins;
/*1ec*/	ULONG	coTicks;
        ULONG	nextSameHash;	/* == 0 */
        ULONG	parent;		/* == 0 */
/*1f8*/	ULONG	extension;		/* FFS: first directory cache block */
/*1fc*/	ULONG	secType;	/* == 1 */
};


struct bFileHeaderBlock {
/*000*/	ULONG	type;		/* == 2 */
/*004*/	ULONG	headerKey;	/* current block number */
/*008*/	ULONG	highSeq;	/* number of data block in this hdr block */
/*00c*/	ULONG	dataSize;	/* == 0 */
/*010*/	ULONG	firstData;
/*014*/	ULONG	checkSum;
/*018*/	ULONG	dataBlocks[MAX_DATABLK];
/*138*/	ULONG	r1;
/*13c*/	ULONG	r2;
/*140*/	ULONG	access;	/* bit0=del, 1=modif, 2=write, 3=read */
/*144*/	ULONG	byteSize;
/*148*/	UBYTE	commLen;
/*149*/	TEXT	comment[MAXCMMTLEN+1];
        UBYTE	r3[91-(MAXCMMTLEN+1)];
/*1a4*/	ULONG	days;
/*1a8*/	ULONG	mins;
/*1ac*/	ULONG	ticks;
/*1b0*/	UBYTE	nameLen;
/*1b1*/	TEXT	fileName[MAXNAMELEN+1];
        ULONG	r4;
/*1d4*/	ULONG	real;		/* unused == 0 */
/*1d8*/	ULONG	nextLink;	/* link chain */
        ULONG	r5[5];
/*1f0*/	ULONG	nextSameHash;	/* next entry with sane hash */
/*1f4*/	ULONG	parent;		/* parent directory */
/*1f8*/	ULONG	extension;	/* pointer to extension block */
/*1fc*/	ULONG	secType;	/* == -3 */
};


/*--- file header extension block structure ---*/

struct bFileExtBlock {
/*000*/	ULONG	type;		/* == 0x10 */
/*004*/	ULONG	headerKey;
/*008*/	ULONG	highSeq;
/*00c*/	ULONG	dataSize;	/* == 0 */
/*010*/	ULONG	firstData;	/* == 0 */
/*014*/	ULONG	checkSum;
/*018*/	ULONG	dataBlocks[MAX_DATABLK];
        ULONG	r[45];
        ULONG	info;		/* == 0 */
        ULONG	nextSameHash;	/* == 0 */
/*1f4*/	ULONG	parent;		/* header block */
/*1f8*/	ULONG	extension;	/* next header extension block */
/*1fc*/	ULONG	secType;	/* -3 */	
};



struct bDirBlock {
/*000*/	ULONG	type;		/* == 2 */
/*004*/	ULONG	headerKey;
/*008*/	ULONG	highSeq;	/* == 0 */
/*00c*/	ULONG	hashTableSize;	/* == 0 */
        ULONG	r1;		/* == 0 */
/*014*/	ULONG	checkSum;
/*018*/	ULONG	hashTable[HT_SIZE];		/* hash table */
        ULONG	r2[2];
/*140*/	ULONG	access;
        ULONG	r4;		/* == 0 */
/*148*/	UBYTE	commLen;
/*149*/	TEXT	comment[MAXCMMTLEN+1];
        TEXT	r5[91-(MAXCMMTLEN+1)];
/*1a4*/	ULONG	days;		/* last access */
/*1a8*/	ULONG	mins;
/*1ac*/	ULONG	ticks;
/*1b0*/	UBYTE	nameLen;
/*1b1*/	TEXT 	dirName[MAXNAMELEN+1];
        ULONG	r6;
/*1d4*/	ULONG	real;		/* ==0 */
/*1d8*/	ULONG	nextLink;	/* link list */
        ULONG	r7[5];
/*1f0*/	ULONG	nextSameHash;
/*1f4*/	ULONG	parent;
/*1f8*/	ULONG	extension;		/* FFS : first directory cache */
/*1fc*/	ULONG	secType;	/* == 2 */
};



struct bOFSDataBlock{
/*000*/	ULONG	type;		/* == 8 */
/*004*/	ULONG	headerKey;	/* pointer to file_hdr block */
/*008*/	ULONG	seqNum;	/* file data block number */
/*00c*/	ULONG	dataSize;	/* <= 0x1e8 */
/*010*/	ULONG	nextData;	/* next data block */
/*014*/	ULONG	checkSum;
/*018*/	UCHAR	data[488];
/*200*/	};


/* --- bitmap --- */

struct bBitmapBlock {
/*000*/	ULONG	checkSum;
/*004*/	ULONG	map[127];
	};


struct bBitmapExtBlock {
/*000*/	ULONG	bmPages[127];
/*1fc*/	ULONG	nextBlock;
	};


struct bLinkBlock {
/*000*/	ULONG	type;		/* == 2 */
/*004*/	ULONG	headerKey;	/* self pointer */
        ULONG	r1[3];
/*014*/	ULONG	checkSum;
/*018*/	TEXT	realName[64];
        ULONG	r2[83];
/*1a4*/	ULONG	days;		/* last access */
/*1a8*/	ULONG	mins;
/*1ac*/	ULONG	ticks;
/*1b0*/	UBYTE   nameLen;
/*1b1*/	TEXT 	name[MAXNAMELEN+1];
        ULONG	r3;
/*1d4*/	ULONG	realEntry;
/*1d8*/	ULONG	nextLink;
        ULONG	r4[5];
/*1f0*/	ULONG	nextSameHash;
/*1f4*/	ULONG	parent;	
        ULONG	r5;
/*1fc*/	ULONG	secType;	/* == -4, 4, 3 */
	};



/*--- directory cache block structure ---*/

struct bDirCacheBlock {
/*000*/	ULONG	type;		/* == 33 */
/*004*/	ULONG	headerKey;
/*008*/	ULONG	parent;
/*00c*/	ULONG	recordsNb;
/*010*/	ULONG	nextDirC;
/*014*/	ULONG	checkSum;
/*018*/	UBYTE	records[488];
	};


#endif /* ADF_BLK_H */
/*##########################################################################*/
