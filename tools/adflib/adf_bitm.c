/*
 *  ADF Library. (C) 1997-1999 Laurent Clevy
 *
 *  adf_bitm.c
 *
 *  bitmap code
 */

#include <stdlib.h>
#include <string.h>

#include"adf_raw.h"
#include"adf_bitm.h"
#include"adf_err.h"
#include"adf_disk.h"
#include"adf_util.h"
#include"defendian.h"

extern struct Env adfEnv;

/*
 * adfUpdateBitmap
 *
 */
RETCODE adfUpdateBitmap(struct Volume *vol)
{
	int i;
    struct bRootBlock root;

/*printf("adfUpdateBitmap\n");*/
        
    if (adfReadRootBlock(vol, vol->rootBlock,&root)!=RC_OK)
		return RC_ERROR;

    root.bmFlag = BM_INVALID;
    if (adfWriteRootBlock(vol,vol->rootBlock,&root)!=RC_OK)
		return RC_ERROR;

    for(i=0; i<vol->bitmapSize; i++)
    if (vol->bitmapBlocksChg[i]) {
        if (adfWriteBitmapBlock(vol, vol->bitmapBlocks[i], vol->bitmapTable[i])!=RC_OK)
			return RC_ERROR;
  	    vol->bitmapBlocksChg[i] = FALSE;
    }

    root.bmFlag = BM_VALID;
    adfTime2AmigaTime(adfGiveCurrentTime(),&(root.days),&(root.mins),&(root.ticks));
    if (adfWriteRootBlock(vol,vol->rootBlock,&root)!=RC_OK)
		return RC_ERROR;

    return RC_OK;
}


/*
 * adfCountFreeBlocks
 *
 */
ULONG adfCountFreeBlocks(struct Volume* vol)
{
    ULONG freeBlocks;
    int j;

	freeBlocks = 0L;
    for(j=vol->firstBlock+2; j<=(vol->lastBlock - vol->firstBlock); j++)
        if ( adfIsBlockFree(vol,j) )
            freeBlocks++;

    return freeBlocks;
}


/*
 * adfReadBitmap
 *
 */
RETCODE adfReadBitmap(struct Volume* vol, SECTNUM nBlock, struct bRootBlock* root)
{
	ULONG mapSize, nSect;
	ULONG j, i;
	struct bBitmapExtBlock bmExt;

    mapSize = nBlock / (127*32);
    if ( (nBlock%(127*32))!=0 )
        mapSize++;
    vol->bitmapSize = mapSize;

    vol->bitmapTable = (struct bBitmapBlock**) malloc(sizeof(struct bBitmapBlock*)*mapSize);
    if (!vol->bitmapTable) { 
		(*adfEnv.eFct)("adfReadBitmap : malloc, vol->bitmapTable");
        return RC_MALLOC;
    }
	vol->bitmapBlocks = (SECTNUM*) malloc(sizeof(SECTNUM)*mapSize);
    if (!vol->bitmapBlocks) {
        free(vol->bitmapTable);
		(*adfEnv.eFct)("adfReadBitmap : malloc, vol->bitmapBlocks");
        return RC_MALLOC;
    }
	vol->bitmapBlocksChg = (BOOL*) malloc(sizeof(BOOL)*mapSize);
    if (!vol->bitmapBlocksChg) { 
        free(vol->bitmapTable); free(vol->bitmapBlocks);
		(*adfEnv.eFct)("adfReadBitmap : malloc, vol->bitmapBlocks");
        return RC_MALLOC;
    }
    for(i=0; i<mapSize; i++) {
        vol->bitmapBlocksChg[i] = FALSE;

		vol->bitmapTable[i] = (struct bBitmapBlock*)malloc(sizeof(struct bBitmapBlock));
		if (!vol->bitmapTable[i]) {
            free(vol->bitmapBlocksChg); free(vol->bitmapBlocks);
            for(j=0; j<i; j++) 
                free(vol->bitmapTable[j]);
            free(vol->bitmapTable);
	        (*adfEnv.eFct)("adfReadBitmap : malloc, vol->bitmapBlocks");
            return RC_MALLOC;
        }
    }

	j=0; i=0;
    /* bitmap pointers in rootblock : 0 <= i <BM_SIZE */
	while(i<BM_SIZE && root->bmPages[i]!=0) {
		vol->bitmapBlocks[j] = nSect = root->bmPages[i];
        if ( !isSectNumValid(vol,nSect) ) {
			(*adfEnv.wFct)("adfReadBitmap : sector out of range");
        }

		if (adfReadBitmapBlock(vol, nSect, vol->bitmapTable[j])!=RC_OK) {
            adfFreeBitmap(vol);
            return RC_ERROR;
		}
		j++; i++;
	}
	nSect = root->bmExt;
	while(nSect!=0) {
        /* bitmap pointers in bitmapExtBlock, j <= mapSize */
        if (adfReadBitmapExtBlock(vol, nSect, &bmExt)!=RC_OK) {
            adfFreeBitmap(vol);
            return RC_ERROR;
        }
		i=0;
		while(i<127 && j<mapSize) {
            nSect = bmExt.bmPages[i];
            if ( !isSectNumValid(vol,nSect) )
                (*adfEnv.wFct)("adfReadBitmap : sector out of range");
			vol->bitmapBlocks[j] = nSect;

			if (adfReadBitmapBlock(vol, nSect, vol->bitmapTable[j])!=RC_OK) {
                adfFreeBitmap(vol);
                return RC_ERROR;
            }
			i++; j++;
		}
		nSect = bmExt.nextBlock;
	}

    return RC_OK;
}


/*
 * adfIsBlockFree
 *
 */
BOOL adfIsBlockFree(struct Volume* vol, SECTNUM nSect)
{
    int sectOfMap = nSect-2;
    int block = sectOfMap/(127*32);
    int indexInMap = (sectOfMap/32)%127;

    return ( (vol->bitmapTable[ block ]->map[ indexInMap ]
        & (1UL << (sectOfMap & 0x1f)))!=0 );
}


/*
 * adfSetBlockFree OK
 *
 */
void adfSetBlockFree(struct Volume* vol, SECTNUM nSect)
{
    int sectOfMap = nSect-2;
    int block = sectOfMap/(127*32);
    int indexInMap = (sectOfMap/32)%127;

    vol->bitmapTable[ block ]->map[ indexInMap ]
	    |= (1UL << (sectOfMap & 0x1f));

    vol->bitmapBlocksChg[ block ] = TRUE;
}


/*
 * adfSetBlockUsed
 *
 */
void adfSetBlockUsed(struct Volume* vol, SECTNUM nSect)
{
    int sectOfMap = nSect-2;
    int block = sectOfMap/(127*32);
    int indexInMap = (sectOfMap/32)%127;

    vol->bitmapTable[ block ]->map[ indexInMap ]
	    &= (~(1UL << (sectOfMap & 0x1f)));
    vol->bitmapBlocksChg[ block ] = TRUE;
}


/*
 * adfGet1FreeBlock
 *
 */
SECTNUM adfGet1FreeBlock(struct Volume *vol) {
    SECTNUM block[1];
    if (!adfGetFreeBlocks(vol,1,block))
        return((SECTNUM)-1);
    else
        return(block[0]);
}

/*
 * adfGetFreeBlocks
 *
 */
BOOL adfGetFreeBlocks(struct Volume* vol, int nbSect, SECTNUM* sectList)
{
	int i, j;
    BOOL endSearch;
    ULONG block = vol->rootBlock;

    i = 0;
    endSearch = FALSE;
//printf("lastblock=%ld\n",vol->lastBlock);
    while( i<nbSect && !endSearch ) {
        if ( adfIsBlockFree(vol, block) ) {
            sectList[i] = block;
			i++;
        }
/*        if ( block==vol->lastBlock )
            block = vol->firstBlock+2;*/
        if ( (block+vol->firstBlock)==vol->lastBlock )
            block = 2;
        else
        {
            block++;
            if (block == vol->rootBlock)
                return FALSE;
        }
    }

    if (i==nbSect)
        for(j=0; j<nbSect; j++)
            adfSetBlockUsed( vol, sectList[j] );

    return (i==nbSect);
}


/*
 * adfCreateBitmap
 *
 * create bitmap structure in vol
 */
RETCODE adfCreateBitmap(struct Volume *vol)
{
    ULONG nBlock, mapSize ;
    int i, j;

    nBlock = vol->lastBlock - vol->firstBlock +1 - 2;

    mapSize = nBlock / (127*32);
    if ( (nBlock%(127*32))!=0 )
        mapSize++;
    vol->bitmapSize = mapSize;

    vol->bitmapTable = (struct bBitmapBlock**)malloc( sizeof(struct bBitmapBlock*)*mapSize );
    if (!vol->bitmapTable) {
        (*adfEnv.eFct)("adfCreateBitmap : malloc, vol->bitmapTable");
        return RC_MALLOC;
    }

	vol->bitmapBlocksChg = (BOOL*) malloc(sizeof(BOOL)*mapSize);
    if (!vol->bitmapBlocksChg) {
        free(vol->bitmapTable);
        (*adfEnv.eFct)("adfCreateBitmap : malloc, vol->bitmapBlocksChg");
        return RC_MALLOC;
    }

	vol->bitmapBlocks = (SECTNUM*) malloc(sizeof(SECTNUM)*mapSize);
    if (!vol->bitmapBlocks) {
        free(vol->bitmapTable); free(vol->bitmapBlocksChg);
        (*adfEnv.eFct)("adfCreateBitmap : malloc, vol->bitmapBlocks");
        return RC_MALLOC;
    }

    for(i=0; i<mapSize; i++) {
        vol->bitmapTable[i] = (struct bBitmapBlock*)malloc(sizeof(struct bBitmapBlock));
        if (!vol->bitmapTable[i]) {
            free(vol->bitmapTable); free(vol->bitmapBlocksChg);
            for(j=0; j<i; j++) 
                free(vol->bitmapTable[j]);
            free(vol->bitmapTable);
			(*adfEnv.eFct)("adfCreateBitmap : malloc");
            return RC_MALLOC;
        }
    }

    for(i=2; i<=(vol->lastBlock - vol->firstBlock); i++)
        adfSetBlockFree(vol, i);

    return RC_OK;
}


/*
 * adfWriteNewBitmap
 *
 * write ext blocks and bitmap
 *
 * uses vol->bitmapSize, 
 */
RETCODE adfWriteNewBitmap(struct Volume *vol)
{
    struct bBitmapExtBlock bitme;
    SECTNUM *bitExtBlock;
    int n, i, k;
    int nExtBlock;
    int nBlock;
    SECTNUM *sectList;
    struct bRootBlock root;

    sectList=(SECTNUM*)malloc(sizeof(SECTNUM)*vol->bitmapSize);
    if (!sectList) {
		(*adfEnv.eFct)("adfCreateBitmap : sectList");
        return RC_MALLOC;
    }

    if (!adfGetFreeBlocks(vol, vol->bitmapSize, sectList)) {
        free(sectList);
		return RC_ERROR;
    }
	
    if (adfReadRootBlock(vol, vol->rootBlock, &root)!=RC_OK) {
        free(sectList);
		return RC_ERROR;
    }
    nBlock = 0;
    n = min( vol->bitmapSize, BM_SIZE );
    for(i=0; i<n; i++) {
        root.bmPages[i] = vol->bitmapBlocks[i] = sectList[i];
    }
    nBlock = n;

    /* for devices with more than 25*127 blocks == hards disks */
    if (vol->bitmapSize>BM_SIZE) {

        nExtBlock = (vol->bitmapSize-BM_SIZE)/127;
        if ((vol->bitmapSize-BM_SIZE)%127)
            nExtBlock++;

        bitExtBlock=(SECTNUM*)malloc(sizeof(SECTNUM)*nExtBlock);
        if (!bitExtBlock) {
            free(sectList);
			adfEnv.eFct("adfWriteNewBitmap : malloc failed");
            return RC_MALLOC;
        }

        if (!adfGetFreeBlocks(vol, nExtBlock, bitExtBlock)) {  
           free(sectList); free(bitExtBlock);
           return RC_MALLOC;
        }

        k = 0;
        root.bmExt = bitExtBlock[ k ];
        while( nBlock<vol->bitmapSize ) {
            i=0;
            while( i<127 && nBlock<vol->bitmapSize ) {
                bitme.bmPages[i] = vol->bitmapBlocks[nBlock] = sectList[i];
                i++;
                nBlock++;
            }
            if ( k+1<nExtBlock )
                bitme.nextBlock = bitExtBlock[ k+1 ];
            else
                bitme.nextBlock = 0;
            if (adfWriteBitmapExtBlock(vol, bitExtBlock[ k ], &bitme)!=RC_OK) {
                free(sectList); free(bitExtBlock);
				return RC_ERROR;
            }
            k++;
        }
        free( bitExtBlock );

    }
    free( sectList);

    if (adfWriteRootBlock(vol,vol->rootBlock,&root)!=RC_OK)
        return RC_ERROR;
    
    return RC_OK;
}

/*
 * adfReadBitmapBlock
 *
 * ENDIAN DEPENDENT
 */
RETCODE
adfReadBitmapBlock(struct Volume* vol, SECTNUM nSect, struct bBitmapBlock* bitm)
{
	unsigned char buf[LOGICAL_BLOCK_SIZE];

//printf("bitmap %ld\n",nSect);
	if (adfReadBlock(vol, nSect, buf)!=RC_OK)
		return RC_ERROR;

	memcpy(bitm, buf, LOGICAL_BLOCK_SIZE);
#ifdef LITT_ENDIAN
    /* big to little = 68000 to x86 */
    swapEndian((unsigned char*)bitm, SWBL_BITMAP);
#endif

	if (bitm->checkSum!=adfNormalSum(buf,0,LOGICAL_BLOCK_SIZE))
		(*adfEnv.wFct)("adfReadBitmapBlock : invalid checksum");

    return RC_OK;
}


/*
 * adfWriteBitmapBlock
 *
 * OK
 */
RETCODE
adfWriteBitmapBlock(struct Volume* vol, SECTNUM nSect, struct bBitmapBlock* bitm)
{
    unsigned char buf[LOGICAL_BLOCK_SIZE];
	ULONG newSum;
	
	memcpy(buf,bitm,LOGICAL_BLOCK_SIZE);
#ifdef LITT_ENDIAN
    /* little to big */
    swapEndian(buf, SWBL_BITMAP);
#endif

	newSum = adfNormalSum(buf, 0, LOGICAL_BLOCK_SIZE);
    swLong(buf,newSum);

/*	dumpBlock((unsigned char*)buf);*/
	if (adfWriteBlock(vol, nSect, (unsigned char*)buf)!=RC_OK)
		return RC_ERROR;

    return RC_OK;
}


/*
 * adfReadBitmapExtBlock
 *
 * ENDIAN DEPENDENT
 */
RETCODE
adfReadBitmapExtBlock(struct Volume* vol, SECTNUM nSect, struct bBitmapExtBlock* bitme)
{
	unsigned char buf[LOGICAL_BLOCK_SIZE];

	if (adfReadBlock(vol, nSect, buf)!=RC_OK)
		return RC_ERROR;

	memcpy(bitme, buf, LOGICAL_BLOCK_SIZE);
#ifdef LITT_ENDIAN
    swapEndian((unsigned char*)bitme, SWBL_BITMAP);
#endif

    return RC_OK;
}


/*
 * adfWriteBitmapExtBlock
 *
 */
RETCODE
adfWriteBitmapExtBlock(struct Volume* vol, SECTNUM nSect, struct bBitmapExtBlock* bitme)
{
	unsigned char buf[LOGICAL_BLOCK_SIZE];
	
	memcpy(buf,bitme, LOGICAL_BLOCK_SIZE);
#ifdef LITT_ENDIAN
    /* little to big */
    swapEndian(buf, SWBL_BITMAPE);
#endif

/*	dumpBlock((unsigned char*)buf);*/
	if (adfWriteBlock(vol, nSect, (unsigned char*)buf)!=RC_OK)
		return RC_ERROR;

    return RC_OK;
}


/*
 * adfFreeBitmap
 *
 */
void adfFreeBitmap(struct Volume* vol)
{
    int i;

    for(i=0; i<vol->bitmapSize; i++)
        free(vol->bitmapTable[i]);
    vol->bitmapSize = 0;

    free(vol->bitmapTable);
	vol->bitmapTable = 0;

    free(vol->bitmapBlocks);
	vol->bitmapBlocks = 0;

    free(vol->bitmapBlocksChg);
	vol->bitmapBlocksChg = 0;
}


/*#######################################################################################*/
