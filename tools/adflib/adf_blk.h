/*
 *  ADF Library. (C) 1997-1999 Laurent Clevy
 *
 *  adf_blk.h
 *
 *  general blocks structures
 */


#ifndef ADF_BLK_H
#define ADF_BLK_H 1

#define ULONG   unsigned long
#define USHORT  unsigned short
#define UCHAR   unsigned char

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
/*000*/	char	dosType[4];
/*004*/	ULONG	checkSum;
/*008*/	long	rootBlock;
/*00c*/	UCHAR	data[500+512];
};


struct bRootBlock {
/*000*/	long	type;
        long	headerKey;
        long	highSeq;
/*00c*/	long	hashTableSize;
        long	firstData;
/*014*/	ULONG	checkSum;
/*018*/	long	hashTable[HT_SIZE];		/* hash table */
/*138*/	long	bmFlag;				/* bitmap flag, -1 means VALID */
/*13c*/	long	bmPages[BM_SIZE];
/*1a0*/	long	bmExt;
/*1a4*/	long	cDays; 	/* creation date FFS and OFS */
/*1a8*/	long	cMins;
/*1ac*/	long	cTicks;
/*1b0*/	char	nameLen;
/*1b1*/	char 	diskName[MAXNAMELEN+1];
        char	r2[8];
/*1d8*/	long	days;		/* last access : days after 1 jan 1978 */
/*1dc*/	long	mins;		/* hours and minutes in minutes */
/*1e0*/	long	ticks;		/* 1/50 seconds */
/*1e4*/	long	coDays;	/* creation date OFS */
/*1e8*/	long	coMins;
/*1ec*/	long	coTicks;
        long	nextSameHash;	/* == 0 */
        long	parent;		/* == 0 */
/*1f8*/	long	extension;		/* FFS: first directory cache block */
/*1fc*/	long	secType;	/* == 1 */
};


struct bFileHeaderBlock {
/*000*/	long	type;		/* == 2 */
/*004*/	long	headerKey;	/* current block number */
/*008*/	long	highSeq;	/* number of data block in this hdr block */
/*00c*/	long	dataSize;	/* == 0 */
/*010*/	long	firstData;
/*014*/	ULONG	checkSum;
/*018*/	long	dataBlocks[MAX_DATABLK];
/*138*/	long	r1;
/*13c*/	long	r2;
/*140*/	long	access;	/* bit0=del, 1=modif, 2=write, 3=read */
/*144*/	unsigned long	byteSize;
/*148*/	char	commLen;
/*149*/	char	comment[MAXCMMTLEN+1];
        char	r3[91-(MAXCMMTLEN+1)];
/*1a4*/	long	days;
/*1a8*/	long	mins;
/*1ac*/	long	ticks;
/*1b0*/	char	nameLen;
/*1b1*/	char	fileName[MAXNAMELEN+1];
        long	r4;
/*1d4*/	long	real;		/* unused == 0 */
/*1d8*/	long	nextLink;	/* link chain */
        long	r5[5];
/*1f0*/	long	nextSameHash;	/* next entry with sane hash */
/*1f4*/	long	parent;		/* parent directory */
/*1f8*/	long	extension;	/* pointer to extension block */
/*1fc*/	long	secType;	/* == -3 */
};


/*--- file header extension block structure ---*/

struct bFileExtBlock {
/*000*/	long	type;		/* == 0x10 */
/*004*/	long	headerKey;
/*008*/	long	highSeq;
/*00c*/	long	dataSize;	/* == 0 */
/*010*/	long	firstData;	/* == 0 */
/*014*/	ULONG	checkSum;
/*018*/	long	dataBlocks[MAX_DATABLK];
        long	r[45];
        long	info;		/* == 0 */
        long	nextSameHash;	/* == 0 */
/*1f4*/	long	parent;		/* header block */
/*1f8*/	long	extension;	/* next header extension block */
/*1fc*/	long	secType;	/* -3 */	
};



struct bDirBlock {
/*000*/	long	type;		/* == 2 */
/*004*/	long	headerKey;
/*008*/	long	highSeq;	/* == 0 */
/*00c*/	long	hashTableSize;	/* == 0 */
        long	r1;		/* == 0 */
/*014*/	ULONG	checkSum;
/*018*/	long	hashTable[HT_SIZE];		/* hash table */
        long	r2[2];
/*140*/	long	access;
        long	r4;		/* == 0 */
/*148*/	char	commLen;
/*149*/	char	comment[MAXCMMTLEN+1];
        char	r5[91-(MAXCMMTLEN+1)];
/*1a4*/	long	days;		/* last access */
/*1a8*/	long	mins;
/*1ac*/	long	ticks;
/*1b0*/	char	nameLen;
/*1b1*/	char 	dirName[MAXNAMELEN+1];
        long	r6;
/*1d4*/	long	real;		/* ==0 */
/*1d8*/	long	nextLink;	/* link list */
        long	r7[5];
/*1f0*/	long	nextSameHash;
/*1f4*/	long	parent;
/*1f8*/	long	extension;		/* FFS : first directory cache */
/*1fc*/	long	secType;	/* == 2 */
};



struct bOFSDataBlock{
/*000*/	long	type;		/* == 8 */
/*004*/	long	headerKey;	/* pointer to file_hdr block */
/*008*/	long	seqNum;	/* file data block number */
/*00c*/	long	dataSize;	/* <= 0x1e8 */
/*010*/	long	nextData;	/* next data block */
/*014*/	ULONG	checkSum;
/*018*/	UCHAR	data[488];
/*200*/	};


/* --- bitmap --- */

struct bBitmapBlock {
/*000*/	ULONG	checkSum;
/*004*/	ULONG	map[127];
	};


struct bBitmapExtBlock {
/*000*/	long	bmPages[127];
/*1fc*/	long	nextBlock;
	};


struct bLinkBlock {
/*000*/	long	type;		/* == 2 */
/*004*/	long	headerKey;	/* self pointer */
        long	r1[3];
/*014*/	ULONG	checkSum;
/*018*/	char	realName[64];
        long	r2[83];
/*1a4*/	long	days;		/* last access */
/*1a8*/	long	mins;
/*1ac*/	long	ticks;
/*1b0*/	char	nameLen;
/*1b1*/	char 	name[MAXNAMELEN+1];
        long	r3;
/*1d4*/	long	realEntry;
/*1d8*/	long	nextLink;
        long	r4[5];
/*1f0*/	long	nextSameHash;
/*1f4*/	long	parent;	
        long	r5;
/*1fc*/	long	secType;	/* == -4, 4, 3 */
	};



/*--- directory cache block structure ---*/

struct bDirCacheBlock {
/*000*/	long	type;		/* == 33 */
/*004*/	long	headerKey;
/*008*/	long	parent;
/*00c*/	long	recordsNb;
/*010*/	long	nextDirC;
/*014*/	ULONG	checkSum;
/*018*/	unsigned char records[488];
	};


#endif /* ADF_BLK_H */
/*##########################################################################*/
