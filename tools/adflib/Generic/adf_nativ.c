/*
 * adf_nativ.c
 *
 * file
 */

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>

#include "adf_str.h"
#include "adf_nativ.h"
#include "adf_err.h"

extern struct Env adfEnv;

/*
 * myInitDevice
 *
 * must fill 'dev->size'
 */
static RETCODE myInitDevice(struct Device* dev, char* name,BOOL ro)
{
    struct nativeDevice* nDev;
    int fd;
    off_t size;

    fd = open(name, (ro ? O_RDONLY : O_RDWR));
    if (fd < 0) {
        (*adfEnv.eFct)("myInitDevice : open");
        return RC_ERROR;
    }

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

    nDev->fd = fdopen(fd, ro ? "rb" : "wb+");
    size = lseek(fd, 0, SEEK_END);

    dev->sectors = 61;
    dev->heads = 126;
    dev->cylinders = size / 512 / dev->sectors / dev->heads;
    dev->size = dev->cylinders * dev->heads * dev->sectors * 512;
    dev->isNativeDev = TRUE;

    return RC_OK;
}


/*
 * myReadSector
 *
 */
static RETCODE myReadSector(struct Device *dev, long n, int size, unsigned char* buf)
{
    struct nativeDevice *nDev = dev->nativeDev;
    int fd = fileno(nDev->fd);

    if (lseek(fd, (off_t)n * 512, SEEK_SET) != (off_t)-1) 
        if (read(fd, buf, size) == size)
            return RC_OK;

    return RC_ERROR;
}


/*
 * myWriteSector
 *
 */
static RETCODE myWriteSector(struct Device *dev, long n, int size, unsigned char* buf)
{
    struct nativeDevice *nDev = dev->nativeDev;
    int fd = fileno(nDev->fd);

    if (lseek(fd, (off_t)n * 512, SEEK_SET) != (off_t)-1) 
        if (write(fd, buf, size) == size)
            return RC_OK;

    return RC_ERROR;
}


/*
 * myReleaseDevice
 *
 * free native device
 */
static RETCODE myReleaseDevice(struct Device *dev)
{
    struct nativeDevice* nDev;

    nDev = (struct nativeDevice*)dev->nativeDev;

    fclose(nDev->fd);
    free(nDev);

    return RC_OK;
}


/*
 * myIsDevNative
 *
 */
BOOL myIsDevNative(char *devName)
{
    return (strncmp(devName,"/dev/",5)==0);
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

/*##########################################################################*/
