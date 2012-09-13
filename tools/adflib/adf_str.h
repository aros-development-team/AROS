#ifndef _ADF_STR_H
#define _ADF_STR_H 1

/*
 *  ADF Library. (C) 1997-1999 Laurent Clevy
 *
 *  adf_str.h
 *
 *  structures/constants definitions
 */

#include"adf_defs.h"
#include"adf_blk.h"
#include"adf_err.h"

/* ----- VOLUME ----- */

struct Volume {
    struct Device* dev;

    SECTNUM firstBlock;     /* first block of data area (from beginning of device) */
    SECTNUM lastBlock;      /* last block of data area  (from beginning of device) */
    SECTNUM rootBlock;      /* root block (from firstBlock) */
    int bootBlocks;         /* boot blocks */

    char dosType;           /* FFS/OFS, DIRCACHE, INTERNATIONAL */
    BOOL bootCode;
    BOOL readOnly;
    int datablockSize;      /* 488 or 512 */
    int blockSize;			/* 512 */

    char *volName;

    BOOL mounted;

    ULONG bitmapSize;             /* in blocks */
    SECTNUM *bitmapBlocks;       /* bitmap blocks pointers */
    struct bBitmapBlock **bitmapTable;
    BOOL *bitmapBlocksChg;

    SECTNUM curDirPtr;
};


struct Partition {
    ULONG startCyl;
    ULONG lenCyl;
    char* volName;
    UBYTE volType[4];
};

/* ----- DEVICES ----- */

#define DEVTYPE_FLOPDD 		1
#define DEVTYPE_FLOPHD 		2
#define DEVTYPE_HARDDISK 	3
#define DEVTYPE_HARDFILE 	4

struct Device {
    int   devType;               /* see below */
    BOOL  readOnly;
    ULONG size;                  /* in bytes */

    int   nVol;                  /* partitions */
    struct Volume** volList;  

    ULONG cylinders;            /* geometry */
    ULONG heads;
    ULONG sectors;

    BOOL  isNativeDev;
    void *nativeDev;
};


/* ----- FILE ----- */

struct File {
    struct Volume *volume;

    struct bFileHeaderBlock* fileHdr;
    void *currentData;
    struct bFileExtBlock* currentExt;

    ULONG nDataBlock;
    SECTNUM curDataPtr;
    ULONG pos;

    int posInDataBlk;
    int posInExtBlk;
    BOOL eof, writeMode;
    };


/* ----- ENTRY ---- */

struct Entry{
    int type;
    char* name;
    SECTNUM sector;
    SECTNUM real;
    SECTNUM parent;
    char* comment;
    long  size;
    long  access;
    int year, month, days;
    int hour, mins, secs;
};

struct CacheEntry{
    long header, size, protect;
    short days, mins, ticks;
    signed char type;
    char nLen, cLen;
    char name[MAXNAMELEN+1], comm[MAXCMMTLEN+1];
//    char *name, *comm;

};




struct DateTime{
    int year,mon,day,hour,min,sec;
};

/* ----- ENVIRONMENT ----- */

#define PR_VFCT			1
#define PR_WFCT			2
#define PR_EFCT			3
#define PR_NOTFCT		4
#define PR_USEDIRC		5
#define PR_USE_NOTFCT 	6
#define PR_PROGBAR 		7
#define PR_USE_PROGBAR 	8
#define PR_RWACCESS 	9
#define PR_USE_RWACCESS 10

struct Env{
    void (*vFct)(char*);       /* verbose callback function */
    void (*wFct)(char*);       /* warning callback function */
    void (*eFct)(char*);       /* error callback function */

    void (*notifyFct)(SECTNUM, int);
    BOOL useNotify;

    void (*rwhAccess)(SECTNUM,SECTNUM,BOOL);
    BOOL useRWAccess;

    void (*progressBar)(int);
    BOOL useProgressBar;

    BOOL useDirCache;
	
    void *nativeFct;
};



struct List{         /* generic linked tree */
    void *content;
    struct List* subdir;
    struct List* next;
};

struct GenBlock{
    SECTNUM sect;
    SECTNUM parent;
    int type;
    int secType;
    char *name;	/* if (type == 2 and (secType==2 or secType==-3)) */
};

struct FileBlocks{
    SECTNUM header;
    ULONG nbExtens;
    SECTNUM* extens;
    ULONG nbData;
    SECTNUM* data;
};

struct bEntryBlock {
/*000*/	ULONG	type;		/* T_HEADER == 2 */
/*004*/	ULONG	headerKey;	/* current block number */
        ULONG	r1[3];
/*014*/	ULONG	checkSum;
/*018*/	ULONG	hashTable[HT_SIZE];
        ULONG	r2[2];
/*140*/	ULONG	access;	/* bit0=del, 1=modif, 2=write, 3=read */
/*144*/	ULONG	byteSize;
/*148*/	UBYTE	commLen;
/*149*/	TEXT	comment[MAXCMMTLEN+1];
        UBYTE	r3[91-(MAXCMMTLEN+1)];
/*1a4*/	ULONG	days;
/*1a8*/	ULONG	mins;
/*1ac*/	ULONG	ticks;
/*1b0*/	UBYTE	nameLen;
/*1b1*/	TEXT	name[MAXNAMELEN+1];
        ULONG	r4;
/*1d4*/	ULONG	realEntry;
/*1d8*/	ULONG	nextLink;
        ULONG	r5[5];
/*1f0*/	ULONG	nextSameHash;
/*1f4*/	ULONG	parent;
/*1f8*/	ULONG	extension;
/*1fc*/	ULONG	secType;
	};


#define ENV_DECLARATION struct Env adfEnv


#endif /* _ADF_STR_H */
/*##########################################################################*/
