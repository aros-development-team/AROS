#ifndef _ADF_DISK_H
#define _ADF_DISK_H 1

/*
 *  ADF Library. (C) 1997-1999 Laurent Clevy
 *
 *  adf_disk.h
 *
 */

#include "prefix.h"

#include "adf_str.h"
#include "adf_defs.h"

PREFIX RETCODE adfInstallBootBlock(struct Volume *vol,unsigned char*);

PREFIX BOOL isSectNumValid(struct Volume *vol, SECTNUM nSect);

PREFIX struct Volume* adfMount( struct Device *dev, int nPart, BOOL readOnly );
PREFIX void adfUnMount(struct Volume *vol);
PREFIX void adfVolumeInfo(struct Volume *vol);
struct Volume* adfCreateVol( struct Device* dev, long start, long len, 
    char* volName, int volType );

/*void adfReadBitmap(struct Volume* , long nBlock, struct bRootBlock* root);
void adfUpdateBitmap(struct Volume*);
*/
PREFIX RETCODE adfReadBlock(struct Volume* , long nSect, unsigned char* buf);
PREFIX RETCODE adfWriteBlock(struct Volume* , long nSect, unsigned char* buf);

#endif /* _ADF_DISK_H */

/*##########################################################################*/
