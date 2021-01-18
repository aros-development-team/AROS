/*
 * bootdisk.c
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include"adflib.h"


void MyVer(char *msg)
{
    fprintf(stderr,"Verbose [%s]\n",msg);
}


/*
 *
 *
 */
int main(int argc, char *argv[])
{
    struct Device *hd;
    struct Volume *vol;
    struct List *list, *cell;

    adfEnvInitDefault();

    /* create and mount one device : 4194304 bytes */
    hd = adfCreateDumpDevice("newdev", 256, 2, 32);
    if (!hd) {
        fprintf(stderr, "can't mount device\n");
        adfEnvCleanUp(); exit(1);
    }

    adfCreateHdFile( hd, "empty", FSMASK_FFS|FSMASK_DIRCACHE );

    adfDeviceInfo(hd);

    vol = adfMount(hd, 0, FALSE);
    if (!vol) {
        adfUnMountDev(hd);
        fprintf(stderr, "can't mount volume\n");
        adfEnvCleanUp(); exit(1);
    }
    adfVolumeInfo(vol);

    cell = list = adfGetDirEnt(vol,vol->curDirPtr);
    while(cell) {
        printEntry(cell->content);
        cell = cell->next;
    }
    adfFreeDirList(list);

    /* unmounts */
    adfUnMount(vol);
    adfUnMountDev(hd);


    adfEnvCleanUp();

    return 0;
}
