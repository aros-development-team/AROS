/*
 *  ADF Library. (C) 1997-1998 Laurent Clevy
 *
 *  adf_raw.c
 *
 * logical disk/volume code
 */

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "adf_str.h"
#include "adf_raw.h"
#include "adf_blk.h"
#include "adf_disk.h"
#include "adf_util.h"
#include "adf_err.h"
#include "defendian.h"

extern struct Env adfEnv;

int swapTable[MAX_SWTYPE+1][15]={
    { 4, SW_CHAR, 2, SW_LONG, 1012, SW_CHAR, 0, 1024 },     /* first bytes of boot */
    { 108, SW_LONG, 40, SW_CHAR, 10, SW_LONG, 0, 512 },        /* root */
    { 6, SW_LONG, 488, SW_CHAR, 0, 512 },                      /* data */
                                                            /* file, dir, entry */
    { 82, SW_LONG, 92, SW_CHAR, 3, SW_LONG, 36, SW_CHAR, 11, SW_LONG, 0, 512 },
    { 6, SW_LONG, 0, 24 },                                       /* cache */
    { 128, SW_LONG, 0, 512 },                                /* bitmap, fext */
		                                                    /* link */                                        
    { 6, SW_LONG, 64, SW_CHAR, 86, SW_LONG, 32, SW_CHAR, 12, SW_LONG, 0, 512 },
    { 4, SW_CHAR, 39, SW_LONG, 56, SW_CHAR, 10, SW_LONG, 0, 256 }, /* RDSK */
    { 4, SW_CHAR, 127, SW_LONG, 0, 512 },                          /* BADB */
    { 4, SW_CHAR, 8, SW_LONG, 32, SW_CHAR, 31, SW_LONG, 4, SW_CHAR, /* PART */
      15, SW_LONG, 0, 256 },
    { 4, SW_CHAR, 7, SW_LONG, 4, SW_CHAR, 55, SW_LONG, 0, 256 }, /* FSHD */
    { 4, SW_CHAR, 4, SW_LONG, 492, SW_CHAR, 0, 512 },            /* LSEG */
    { 4, SW_CHAR, 4, SW_LONG, 492, SW_CHAR, 0, 512 }             /* BOOT */
    };


/*
 * swapEndian
 *
 * magic :-) endian swap function (big -> little for read, little to big for write)
 */

    void
swapEndian( UBYTE *buf, int type )
{
    int i,j;
    int p;

    i=0;
    p=0;

    if (type>MAX_SWTYPE || type<0)
        adfEnv.eFct("SwapEndian: type do not exist");

    while( swapTable[type][i]!=0 ) {
        for(j=0; j<swapTable[type][i]; j++) {
            switch( swapTable[type][i+1] ) {
            case SW_LONG:
                *(ULONG*)(buf+p)=Long(buf+p);
                p+=4;
                break;
            case SW_SHORT:
                *(USHORT*)(buf+p)=Short(buf+p);
                p+=2;
                break;
            case SW_CHAR:
                p++;
                break;
            default:
                ;
            }
        }
    i+=2;
    }
    if (p!=swapTable[type][i+1]) 
        (*adfEnv.wFct)("Warning: Endian Swapping length\n");
    

}





/*
 * adfReadRootBlock
 *
 * ENDIAN DEPENDENT
 */
RETCODE
adfReadRootBlock(struct Volume* vol, ULONG nSect, struct bRootBlock* root)
{
	unsigned char buf[LOGICAL_BLOCK_SIZE];

	if (adfReadBlock(vol, nSect, buf)!=RC_OK)
		return RC_ERROR;

	memcpy(root, buf, LOGICAL_BLOCK_SIZE);
#ifdef LITT_ENDIAN
    swapEndian((unsigned char*)root, SWBL_ROOT);    
#endif

	if (root->type!=T_HEADER || root->secType!=ST_ROOT) {
		(*adfEnv.wFct)("adfReadRootBlock : id not found");
        return RC_ERROR;
    }
	if (root->checkSum!=adfNormalSum(buf, 20, LOGICAL_BLOCK_SIZE)) {
		(*adfEnv.wFct)("adfReadRootBlock : invalid checksum");
        return RC_ERROR;
    }
		
    return RC_OK;
}


/*
 * adfWriteRootBlock
 *
 * 
 */
RETCODE adfWriteRootBlock(struct Volume* vol, ULONG nSect, struct bRootBlock* root)
{
    unsigned char buf[LOGICAL_BLOCK_SIZE];
	ULONG newSum;


    root->type = T_HEADER;
    root->headerKey = 0L;
    root->highSeq = 0L;
    root->hashTableSize = HT_SIZE;
    root->firstData = 0L;
    /* checkSum, hashTable */
    /* bmflag */
    /* bmPages, bmExt */
    root->nextSameHash = 0L;
    root->parent = 0L;
    root->secType = ST_ROOT;

    memcpy(buf, root, LOGICAL_BLOCK_SIZE);
#ifdef LITT_ENDIAN
    swapEndian(buf, SWBL_ROOT);
#endif

	newSum = adfNormalSum(buf,20,LOGICAL_BLOCK_SIZE);
    swLong(buf+20, newSum);
//	*(ULONG*)(buf+20) = swapLong((unsigned char*)&newSum);

// 	dumpBlock(buf);
	if (adfWriteBlock(vol, nSect, buf)!=RC_OK)
        return RC_ERROR;
//printf("adfWriteRootBlock %ld\n",nSect);
    return RC_OK;
}


/*
 * adfReadBootBlock
 *
 * ENDIAN DEPENDENT
 */
RETCODE
adfReadBootBlock(struct Volume* vol, struct bBootBlock* boot)
{
	unsigned char buf[1024];
	
//puts("22");
	if (adfReadBlock(vol, 0, buf)!=RC_OK)
		return RC_ERROR;
//puts("11");
    if (adfReadBlock(vol, 1, buf+LOGICAL_BLOCK_SIZE)!=RC_OK)
		return RC_ERROR;

    memcpy(boot, buf, LOGICAL_BLOCK_SIZE*2);
#ifdef LITT_ENDIAN
    swapEndian((unsigned char*)boot,SWBL_BOOTBLOCK);
#endif
	if ( strncmp("DOS",boot->dosType,3)!=0 ) {
		(*adfEnv.wFct)("adfReadBootBlock : DOS id not found");
		return RC_ERROR;
    }

	if ( boot->data[0]!=0 && adfBootSum(buf)!=boot->checkSum ) {
printf("compsum=%lx sum=%lx\n",	(long)adfBootSum(buf),(long)boot->checkSum );
		(*adfEnv.wFct)("adfReadBootBlock : incorrect checksum"); 
    }

    return RC_OK;
}

/*
 * adfWriteBootBlock
 *
 *
 *     write bootcode ?
 */
RETCODE
adfWriteBootBlock(struct Volume* vol, struct bBootBlock* boot)
{
    unsigned char buf[LOGICAL_BLOCK_SIZE*2];
	ULONG newSum;

    boot->dosType[0] = 'D';
    boot->dosType[1] = 'O';
    boot->dosType[2] = 'S';
	memcpy(buf, boot, LOGICAL_BLOCK_SIZE*2);
#ifdef LITT_ENDIAN
    swapEndian(buf, SWBL_BOOTBLOCK);
#endif

    if (boot->rootBlock==880 || boot->data[0]!=0) {
        newSum = adfBootSum(buf);
//fprintf(stderr,"sum %x %x\n",newSum,adfBootSum2(buf));
        swLong(buf+4,newSum);
//        *(ULONG*)(buf+4) = swapLong((unsigned char*)&newSum);
    }

/*	dumpBlock(buf);
	dumpBlock(buf+512);
*/	
    if (adfWriteBlock(vol, 0, buf)!=RC_OK)
		return RC_ERROR;
	if (adfWriteBlock(vol, 1,  buf+512)!=RC_OK)
		return RC_ERROR;
//puts("adfWriteBootBlock");
    return RC_OK;
}


/*
 * NormalSum
 *
 * buf = where the block is stored
 * offset = checksum place (in bytes)
 * bufLen = buffer length (in bytes)
 */
    ULONG
adfNormalSum( UCHAR* buf, int offset, int bufLen )
{
    ULONG newsum;
    int i;

    newsum=0L;
    for(i=0; i < (bufLen/4); i++)
        if ( i != (offset/4) )       /* old chksum */
            newsum+=Long(buf+i*4);
    newsum=(-newsum);	/* WARNING */

    return(newsum);
}

/*
 * adfBitmapSum
 *
 */
	ULONG 
adfBitmapSum(unsigned char *buf)
{
	ULONG newSum;
	int i;
	
	newSum = 0L;
	for(i=1; i<128; i++)
		newSum-=Long(buf+i*4);
	return(newSum);
}


/*
 * adfBootSum
 *
 */
    ULONG 
adfBootSum(unsigned char *buf)
{
    ULONG d, newSum;
    int i;
	
    newSum=0L;
    for(i=0; i<256; i++) {
        if (i!=1) {
            d = Long(buf+i*4);
            if ( (ULONG_MAX-newSum)<d )
                newSum++;
            newSum+=d;
        }
    }
    newSum = ~newSum;	/* not */

    return(newSum);
}

    ULONG 
adfBootSum2(unsigned char *buf)
{
    ULONG prevsum, newSum;
    int i;

    prevsum = newSum=0L;
    for(i=0; i<1024/sizeof(ULONG); i++) {
        if (i!=1) {
            prevsum = newSum;
            newSum += Long(buf+i*4);
            if (newSum < prevsum)
                newSum++;
        }
    }
    newSum = ~newSum;	/* not */

    return(newSum);
}


/*#######################################################################################*/
