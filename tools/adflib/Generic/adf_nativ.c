/*
 * adf_nativ.c
 *
 * file
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"adf_str.h"
#include"adf_nativ.h"
#include"adf_err.h"

extern struct Env adfEnv;

/*
 * myInitDevice
 *
 * must fill 'dev->size'
 */
RETCODE myInitDevice(struct Device* dev, char* name,BOOL ro)
{
    struct nativeDevice* nDev;

    nDev = (struct nativeDevice*)dev->nativeDev;

    nDev = (struct nativeDevice*)malloc(sizeof(struct nativeDevice));
    if (!nDev) {
        (*adfEnv.eFct)("myInitDevice : malloc");
        return RC_ERROR;
    }
    dev->nativeDev = nDev;
    if (!ro)
        /* check if device is writable, if not, force readOnly to TRUE */
        dev->readOnly = FALSE;
    else
        /* mount device as read only */
        dev->readOnly = TRUE;

    dev->size = 0;

    return RC_OK;
}


/*
 * myReadSector
 *
 */
RETCODE myReadSector(struct Device *dev, long n, int size, unsigned char* buf)
{
     return RC_OK;   
}


/*
 * myWriteSector
 *
 */
RETCODE myWriteSector(struct Device *dev, long n, int size, unsigned char* buf)
{
    return RC_OK;
}


/*
 * myReleaseDevice
 *
 * free native device
 */
RETCODE myReleaseDevice(struct Device *dev)
{
    struct nativeDevice* nDev;

    nDev = (struct nativeDevice*)dev->nativeDev;

	free(nDev);

    return RC_OK;
}


/*
 * adfInitNativeFct
 *
 */
void adfInitNativeFct()
{
    struct nativeFunctions *nFct;

    nFct = (struct nativeFunctions*)adfEnv.nativeFct;

    nFct->adfInitDevice = myInitDevice ;
    nFct->adfNativeReadSector = myReadSector ;
    nFct->adfNativeWriteSector = myWriteSector ;
    nFct->adfReleaseDevice = myReleaseDevice ;
    nFct->adfIsDevNative = myIsDevNative;
}


/*
 * myIsDevNative
 *
 */
BOOL myIsDevNative(char *devName)
{
    return (strncmp(devName,"/dev/",5)==0);
}
/*##########################################################################*/
