#ifndef _ADF_HD_H
#define _ADF_HD_H 1

/*
 *  ADF Library. (C) 1997-1998 Laurent Clevy
 *
 *  adf_hd.h
 *
 * Harddisk and devices code
 */

#include"prefix.h"

#include "adf_str.h"
#include "hd_blk.h"
#include "adf_err.h"

int adfDevType(struct Device *dev);
PREFIX void adfDeviceInfo(struct Device *dev);

RETCODE adfMountHd(struct Device *dev);
RETCODE adfMountFlop(struct Device* dev);
PREFIX struct Device* adfMountDev( char* filename,BOOL);
PREFIX void adfUnMountDev( struct Device* dev);

RETCODE adfCreateHdHeader(struct Device* dev, int n, struct Partition** partList );
PREFIX RETCODE adfCreateFlop(struct Device* dev, char* volName, int volType );
PREFIX RETCODE adfCreateHd(struct Device* dev, int n, struct Partition** partList );
PREFIX RETCODE adfCreateHdFile(struct Device* dev, char* volName, int volType);

struct Device* adfCreateDev(char* filename, long cylinders, long heads, long sectors);

RETCODE adfReadBlockDev( struct Device* dev, long nSect, long size, unsigned char* buf );
RETCODE adfWriteBlockDev(struct Device* dev, long nSect, long size, unsigned char* buf );
RETCODE adfReadRDSKblock( struct Device* dev, struct bRDSKblock* blk );
RETCODE adfWriteRDSKblock(struct Device *dev, struct bRDSKblock* rdsk);
RETCODE adfReadPARTblock( struct Device* dev, long nSect, struct bPARTblock* blk );
RETCODE adfWritePARTblock(struct Device *dev, long nSect, struct bPARTblock* part);
RETCODE adfReadFSHDblock( struct Device* dev, long nSect, struct bFSHDblock* blk);
RETCODE adfWriteFSHDblock(struct Device *dev, long nSect, struct bFSHDblock* fshd);
RETCODE adfReadLSEGblock(struct Device* dev, long nSect, struct bLSEGblock* blk);
RETCODE adfWriteLSEGblock(struct Device *dev, long nSect, struct bLSEGblock* lseg);


#endif /* _ADF_HD_H */

/*##########################################################################*/
