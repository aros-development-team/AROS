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
    struct Volume *vol, *vol2;

    /* initialisation */
    adfEnvInitDefault();

    /*** a real harddisk ***/
    hd = adfMountDev( argv[1],FALSE );
    if (!hd) {
        fprintf(stderr, "can't mount device\n");
        adfEnvCleanUp(); exit(1);
    }
    adfDeviceInfo(hd);

    /* mount the 2 partitions */
    vol = adfMount(hd, 0, FALSE);
    if (!vol) {
        adfUnMountDev(hd);
        fprintf(stderr, "can't mount volume\n");
        adfEnvCleanUp(); exit(1);
    }
    adfVolumeInfo(vol);

    vol2 = adfMount(hd, 1, FALSE);
    if (!vol2) {
        adfUnMountDev(hd);
        fprintf(stderr, "can't mount volume\n");
        adfEnvCleanUp(); exit(1);
    }
    adfVolumeInfo(vol2);

    /* unmounts */
    adfUnMount(vol);
    adfUnMount(vol2);
    adfUnMountDev(hd);


    /*** a dump of a zip disk ***/
    hd = adfMountDev( argv[2],FALSE );
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

    /* clean up */
    adfEnvCleanUp();

    return 0;
}
