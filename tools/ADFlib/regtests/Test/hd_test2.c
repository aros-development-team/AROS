/*
 * hd_test.c
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
    struct Partition part1;
    struct Partition **partList;

    adfEnvInitDefault();


    /* a zip disk */
    hd = adfCreateDumpDevice("newdev",2891,1,68);
    if (!hd) {
        fprintf(stderr, "can't mount device\n");
        adfEnvCleanUp(); exit(1);
    }

    adfDeviceInfo(hd);

    partList = (struct Partition**)malloc(sizeof(struct Partition*));
    if (!partList) exit(1);

    part1.startCyl = 2;
	part1.lenCyl = 2889;
	part1.volName = strdup("zip");
    part1.volType = FSMASK_FFS|FSMASK_DIRCACHE;

    partList[0] = &part1;
    adfCreateHd(hd,1,partList);
    free(partList);
    free(part1.volName);

    vol = adfMount(hd, 0, FALSE);
    if (!vol) {
        adfUnMountDev(hd);
        fprintf(stderr, "can't mount volume\n");
        adfEnvCleanUp(); exit(1);
    }

    adfVolumeInfo(vol);
    adfUnMount(vol);
    adfUnMountDev(hd);


    /* mount the created device */

    hd = adfMountDev("newdev",FALSE);
    if (!hd) {
        fprintf(stderr, "can't mount device\n");
        adfEnvCleanUp(); exit(1);
    }

    adfDeviceInfo(hd);

    vol = adfMount(hd, 0, FALSE);
    if (!vol) {
        adfUnMountDev(hd);
        fprintf(stderr, "can't mount volume\n");
        adfEnvCleanUp(); exit(1);
    }

    adfVolumeInfo(vol);

    adfUnMount(vol);
    adfUnMountDev(hd);


    adfEnvCleanUp();

    return 0;
}
