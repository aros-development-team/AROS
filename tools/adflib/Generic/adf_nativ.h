/*
 * adf_nativ_.h
 *
 * file
 */

#ifndef ADF_NATIV_H
#define ADF_NATIV_H

#include<stdio.h>
#include"adf_str.h"

#define NATIVE_FILE  8001

#ifndef BOOL
#define BOOL int
#endif

#ifndef RETCODE
#define RETCODE long
#endif

struct nativeDevice{
    FILE* fd;
};

struct nativeFunctions{
    /* called by adfMount() */
    RETCODE (*adfInitDevice)(struct Device*, char*,BOOL);
    /* called by adfReadBlock() */
    RETCODE (*adfNativeReadSector)(struct Device*, long, int, unsigned char*);
    /* called by adfWriteBlock() */
    RETCODE (*adfNativeWriteSector)(struct Device*, long, int, unsigned char*);
    /* called by adfMount() */
    BOOL (*adfIsDevNative)(char*);
    /* called by adfUnMount() */
    RETCODE (*adfReleaseDevice)();
};

void adfInitNativeFct();


RETCODE myReadSector(struct Device *dev, long n, int size, unsigned char* buf);
RETCODE myWriteSector(struct Device *dev, long n, int size, unsigned char* buf);
RETCODE myInitDevice(struct Device *dev, char* name,BOOL);
RETCODE myReleaseDevice(struct Device *dev);
BOOL myIsDevNative(char*);

#endif /* ADF_NATIV_H */

/*#######################################################################################*/
