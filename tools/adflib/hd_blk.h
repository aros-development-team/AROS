/*
 *  ADF Library. (C) 1997-1998 Laurent Clevy
 *
 *  hd_blk.h
 *
 *  hard disk blocks structures
 */


#ifndef _HD_BLK_H
#define _HD_BLK_H 1

#include "adf_str.h"

/* ------- RDSK ---------*/

struct bRDSKblock {
/*000*/ TEXT    id[4];          /* RDSK */
/*004*/ ULONG   size;           /* 64 ULONGs */
/*008*/ ULONG   checksum;
/*00c*/ ULONG   hostID;         /* 7 */
/*010*/ ULONG   blockSize;      /* 512 bytes */
/*014*/ ULONG   flags;          /* 0x17 */
/*018*/ ULONG   badBlockList;
/*01c*/ ULONG   partitionList;
/*020*/ ULONG   fileSysHdrList;
/*024*/ ULONG   driveInit;
/*028*/ ULONG   bootBlockList;  /* Amiga OS 4 Boot Blocks */
/*02c*/ ULONG   r1[5];          /* -1 */
/*040*/ ULONG   cylinders;
/*044*/ ULONG   sectors;
/*048*/ ULONG   heads;
/*04c*/ ULONG   interleave;
/*050*/ ULONG   parkingZone;
/*054*/ ULONG   r2[3];          /* 0 */
/*060*/ ULONG   writePreComp;
/*064*/ ULONG   reducedWrite;
/*068*/ ULONG   stepRate;
/*06c*/ ULONG   r3[5];          /* 0 */
/*080*/ ULONG   rdbBlockLo;
/*084*/ ULONG   rdbBlockHi;
/*088*/ ULONG   loCylinder;
/*08c*/ ULONG   hiCylinder;
/*090*/ ULONG   cylBlocks;
/*094*/ ULONG   autoParkSeconds;
/*098*/ ULONG   highRDSKBlock;
/*09c*/ ULONG   r4;             /* 0 */
/*0a0*/ TEXT    diskVendor[8];
/*0a8*/ TEXT    diskProduct[16];
/*0b8*/ TEXT    diskRevision[4];
/*0bc*/ TEXT    controllerVendor[8];
/*0c4*/ TEXT    controllerProduct[16];
/*0d4*/ TEXT    controllerRevision[4];
/*0d8*/ ULONG   r5[10];         /* 0 */
/*100*/
};


struct bBADBentry {
/*000*/ ULONG   badBlock;
/*004*/ ULONG   goodBlock;
};


struct bBADBblock {
/*000*/ TEXT    id[4];          /* BADB */
/*004*/ ULONG   size;           /* 128 ULONGs */
/*008*/ ULONG   checksum;       
/*00c*/ ULONG   hostID;         /* 7 */
/*010*/ ULONG   next;
/*014*/ ULONG   r1;
/*018*/ struct bBADBentry blockPairs[61];
};



struct bPARTblock {
/*000*/ TEXT    id[4];          /* PART */
/*004*/ ULONG   size;           /* 64 ULONGs */
/*008*/ ULONG   checksum;
/*00c*/ ULONG   hostID;         /* 7 */
/*010*/ ULONG   next;
/*014*/ ULONG   flags;
/*018*/ ULONG   r1[2];
/*020*/ ULONG   devFlags;
/*024*/ UBYTE   nameLen;
/*025*/ TEXT    name[31];
/*044*/ ULONG   r2[15];

/*080*/ ULONG   vectorSize;     /* often 16 ULONGs */
/*084*/ ULONG   blockSize;      /* 128 ULONGs */
/*088*/ ULONG   secOrg;
/*08c*/ ULONG   surfaces;
/*090*/ ULONG   sectorsPerBlock; /* == 1 */
/*094*/ ULONG   blocksPerTrack;
/*098*/ ULONG   dosReserved;
/*09c*/ ULONG   dosPreAlloc;
/*0a0*/ ULONG   interleave;
/*0a4*/ ULONG   lowCyl;
/*0a8*/ ULONG   highCyl;
/*0ac*/ ULONG   numBuffer;
/*0b0*/ ULONG   bufMemType;
/*0b4*/ ULONG   maxTransfer;
/*0b8*/ ULONG   mask;
/*0bc*/ ULONG   bootPri;
/*0c0*/ char    dosType[4];
/*0c4*/ ULONG   r3[15];
};


struct bLSEGblock {
/*000*/ TEXT    id[4];          /* LSEG */
/*004*/ ULONG   size;           /* 128 ULONGs */
/*008*/ ULONG   checksum;
/*00c*/ ULONG   hostID;         /* 7 */
/*010*/ ULONG   next;
/*014*/ TEXT    loadData[123*4];
};

struct bBOOTblock {
/*000*/ TEXT    id[4];          /* BOOT */
/*004*/ ULONG   size;           /* 128 ULONGs */
/*008*/ ULONG   checksum;
/*00c*/ ULONG   hostID;         /* 0 */
/*010*/ ULONG   next;
/*014*/ TEXT    loadData[123*4];
};

struct bFSHDblock {
/*000*/ TEXT    id[4];          /* FSHD */
/*004*/ ULONG   size;           /* 64 */
/*008*/ ULONG   checksum;
/*00c*/ ULONG   hostID;         /* 7 */
/*010*/ ULONG   next;
/*014*/ ULONG   flags;
/*018*/ ULONG   r1[2];
/*020*/ TEXT    dosType[4];
/*024*/ USHORT  majVersion;
/*026*/ USHORT  minVersion;
/*028*/ ULONG   patchFlags;

/*02c*/ ULONG   type;
/*030*/ ULONG   task;
/*034*/ ULONG   lock;
/*038*/ ULONG   handler;
/*03c*/ ULONG   stackSize;
/*040*/ ULONG   priority;
/*044*/ ULONG   startup;
/*048*/ ULONG   segListBlock;
/*04c*/ ULONG   globalVec;
/*050*/ ULONG   r2[23];
/*0ac*/ ULONG   r3[21];
};


#endif /* _HD_BLK_H */
/*##########################################################################*/
