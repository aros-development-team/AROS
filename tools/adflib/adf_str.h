#ifndef _ADF_STR_H
#define _ADF_STR_H 1

/*
 *  ADF Library. (C) 1997-1999 Laurent Clevy
 *
 *  adf_str.h
 *
 *  structures/constants definitions
 */

#include<stdio.h>

#include"adf_defs.h"
#include"adf_blk.h"
#include"adf_err.h"

/* ----- VOLUME ----- */

struct Volume {
    struct Device* dev;

    SECTNUM firstBlock;     /* first block of data area (from beginning of device) */
    SECTNUM lastBlock;      /* last block of data area  (from beginning of device) */
    SECTNUM rootBlock;      /* root block (from firstBlock) */

    char dosType;           /* FFS/OFS, DIRCACHE, INTERNATIONAL */
    BOOL bootCode;
    BOOL readOnly;
    int datablockSize;      /* 488 or 512 */
    int blockSize;			/* 512 */

    char *volName;

    BOOL mounted;

    long bitmapSize;             /* in blocks */
    SECTNUM *bitmapBlocks;       /* bitmap blocks pointers */
    struct bBitmapBlock **bitmapTable;
    BOOL *bitmapBlocksChg;

    SECTNUM curDirPtr;
};


struct Partition {
    long startCyl;
    long lenCyl;
    char* volName;
    int volType;
};

/* ----- DEVICES ----- */

#define DEVTYPE_FLOPDD 		1
#define DEVTYPE_FLOPHD 		2
#define DEVTYPE_HARDDISK 	3
#define DEVTYPE_HARDFILE 	4

struct Device {
    int devType;               /* see below */
    BOOL readOnly;
    long size;                 /* in bytes */

    int nVol;                  /* partitions */
    struct Volume** volList;  
	
    long cylinders;            /* geometry */
    long heads;
    long sectors;

    BOOL isNativeDev;
    void *nativeDev;
};


/* ----- FILE ----- */

struct File {
    struct Volume *volume;

    struct bFileHeaderBlock* fileHdr;
    void *currentData;
    struct bFileExtBlock* currentExt;

    long nDataBlock;
    SECTNUM curDataPtr;
    unsigned long pos;

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
    unsigned long size;
	long access;
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
    long nbExtens;
    SECTNUM* extens;
    long nbData;
    SECTNUM* data;
};

struct bEntryBlock {
/*000*/	long	type;		/* T_HEADER == 2 */
/*004*/	long	headerKey;	/* current block number */
        long	r1[3];
/*014*/	unsigned long	checkSum;
/*018*/	long	hashTable[HT_SIZE];
        long	r2[2];
/*140*/	long	access;	/* bit0=del, 1=modif, 2=write, 3=read */
/*144*/	long	byteSize;
/*148*/	char	commLen;
/*149*/	char	comment[MAXCMMTLEN+1];
        char	r3[91-(MAXCMMTLEN+1)];
/*1a4*/	long	days;
/*1a8*/	long	mins;
/*1ac*/	long	ticks;
/*1b0*/	char	nameLen;
/*1b1*/	char	name[MAXNAMELEN+1];
        long	r4;
/*1d4*/	long	realEntry;
/*1d8*/	long	nextLink;
        long	r5[5];
/*1f0*/	long	nextSameHash;
/*1f4*/	long	parent;
/*1f8*/	long	extension;
/*1fc*/	long	secType;
	};


#define ENV_DECLARATION struct Env adfEnv


#endif /* _ADF_STR_H */
/*##########################################################################*/
