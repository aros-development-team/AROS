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
/*000*/	char	id[4];		/* RDSK */
/*004*/	long 	size; 		/* 64 longs */
/*008*/	ULONG	checksum;
/*00c*/	long	hostID; 	/* 7 */
/*010*/ 	long 	blockSize; 	/* 512 bytes */
/*014*/ 	long 	flags; 		/* 0x17 */
/*018*/ 	long 	badBlockList;
/*01c*/ 	long 	partitionList;
/*020*/ 	long 	fileSysHdrList;
/*024*/ 	long 	driveInit;
/*028*/ 	long 	r1[6];		/* -1 */
/*040*/ 	long 	cylinders;
/*044*/ 	long 	sectors;
/*048*/ 	long 	heads;
/*04c*/ 	long 	interleave;
/*050*/ 	long 	parkingZone;
/*054*/	long 	r2[3]; 	 	/* 0 */
/*060*/	long 	writePreComp;
/*064*/	long 	reducedWrite;
/*068*/	long 	stepRate;
/*06c*/	long 	r3[5]; 		/* 0 */
/*080*/	long 	rdbBlockLo;
/*084*/ 	long 	rdbBlockHi;
/*088*/ 	long 	loCylinder;
/*08c*/ 	long 	hiCylinder;
/*090*/ 	long 	cylBlocks;
/*094*/ 	long 	autoParkSeconds;
/*098*/ 	long 	highRDSKBlock;
/*09c*/ 	long 	r4; 		/* 0 */
/*0a0*/ 	char 	diskVendor[8];
/*0a8*/ 	char 	diskProduct[16];
/*0b8*/ 	char 	diskRevision[4];
/*0bc*/	char 	controllerVendor[8];
/*0c4*/ 	char 	controllerProduct[16];
/*0d4*/	char 	controllerRevision[4];
/*0d8*/ 	long 	r5[10]; 	/* 0 */
/*100*/
};


struct bBADBentry {
/*000*/	long 	badBlock;
/*004*/	long 	goodBlock;
};


struct bBADBblock {
/*000*/	char	id[4]; 		/* BADB */
/*004*/	long 	size; 		/* 128 longs */
/*008*/	ULONG	checksum; 	
/*00c*/	long	hostID; 	/* 7 */
/*010*/ 	long 	next;
/*014*/ 	long 	r1;
/*018*/ 	struct bBADBentry blockPairs[61];
};



struct bPARTblock {
/*000*/	char	id[4]; 		/* PART */
/*004*/	long 	size; 		/* 64 longs */
/*008*/	ULONG	checksum;
/*00c*/	long	hostID; 	/* 7 */
/*010*/ 	long 	next;
/*014*/ 	long 	flags;
/*018*/ 	long 	r1[2];
/*020*/ 	long 	devFlags;
/*024*/ 	char 	nameLen;
/*025*/ 	char 	name[31];
/*044*/ 	long 	r2[15];

/*080*/ 	long 	vectorSize; 	/* often 16 longs */
/*084*/ 	long 	blockSize; 	/* 128 longs */
/*088*/ 	long 	secOrg;
/*08c*/ 	long 	surfaces;
/*090*/ 	long 	sectorsPerBlock; /* == 1 */
/*094*/ 	long 	blocksPerTrack;
/*098*/ 	long 	dosReserved;
/*09c*/ 	long 	dosPreAlloc;
/*0a0*/ 	long 	interleave;
/*0a4*/ 	long 	lowCyl;
/*0a8*/ 	long 	highCyl;
/*0ac*/	long 	numBuffer;
/*0b0*/ 	long 	bufMemType;
/*0b4*/ 	long 	maxTransfer;
/*0b8*/ 	long 	mask;
/*0bc*/ 	long 	bootPri;
/*0c0*/ 	char 	dosType[4];
/*0c4*/ 	long 	r3[15];
};


struct bLSEGblock {
/*000*/	char	id[4]; 		/* LSEG */
/*004*/	long 	size; 		/* 128 longs */
/*008*/	ULONG	checksum;
/*00c*/	long	hostID; 	/* 7 */
/*010*/ 	long 	next;
/*014*/ 	char 	loadData[123*4];
};


struct bFSHDblock {
/*000*/	char	id[4]; 		/* FSHD */
/*004*/	long 	size; 		/* 64 */
/*008*/	ULONG	checksum;
/*00c*/	long	hostID; 	/* 7 */
/*010*/ 	long 	next;
/*014*/ 	long 	flags;
/*018*/ 	long 	r1[2];
/*020*/ 	char 	dosType[4];
/*024*/ 	short 	majVersion;
/*026*/ 	short 	minVersion;
/*028*/ 	long 	patchFlags;

/*02c*/ 	long 	type;
/*030*/ 	long 	task;
/*034*/ 	long 	lock;
/*038*/ 	long 	handler;
/*03c*/ 	long 	stackSize;
/*040*/ 	long 	priority;
/*044*/ 	long 	startup;
/*048*/ 	long 	segListBlock;
/*04c*/ 	long 	globalVec;
/*050*/ 	long 	r2[23];
/*0ac*/ 	long 	r3[21];
};


#endif /* _HD_BLK_H */
/*##########################################################################*/
