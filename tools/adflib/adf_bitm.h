#ifndef ADF_BITM_H
#define ADF_BITM_H
/*
 *  ADF Library. (C) 1997-1999 Laurent Clevy
 *
 *  adf_bitm.h
 *
 *  bitmap code
 */

#include"adf_str.h"

RETCODE adfReadBitmapBlock(struct Volume*, SECTNUM nSect, struct bBitmapBlock*);
RETCODE adfWriteBitmapBlock(struct Volume*, SECTNUM nSect, struct bBitmapBlock*);
RETCODE adfReadBitmapExtBlock(struct Volume*, SECTNUM nSect, struct bBitmapExtBlock*);
RETCODE adfWriteBitmapExtBlock(struct Volume*, SECTNUM, struct bBitmapExtBlock* );

SECTNUM adfGet1FreeBlock(struct Volume *vol);
RETCODE adfUpdateBitmap(struct Volume *vol);
PREFIX uint32_t adfCountFreeBlocks(struct Volume* vol);
RETCODE adfReadBitmap(struct Volume* , struct bRootBlock* root);
BOOL adfIsBlockFree(struct Volume* vol, SECTNUM nSect);
void adfSetBlockFree(struct Volume* vol, SECTNUM nSect);
void adfSetBlockUsed(struct Volume* vol, SECTNUM nSect);
BOOL adfGetFreeBlocks(struct Volume* vol, int nbSect, SECTNUM* sectList);
RETCODE adfCreateBitmap(struct Volume *vol);
RETCODE adfWriteNewBitmap(struct Volume *vol);
void adfFreeBitmap(struct Volume *vol);

#endif /* ADF_BITM_H */

/*#######################################################################################*/
