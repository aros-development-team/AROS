/*
 * rename.c
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

    /* create and mount one device */
    hd = adfCreateDumpDevice("newdev", 80, 2, 11);
    if (!hd) {
        fprintf(stderr, "can't mount device\n");
        adfEnvCleanUp(); exit(1);
    }

    if (adfCreateFlop( hd, "empty", FSMASK_FFS|FSMASK_DIRCACHE )!=RC_OK) {
		fprintf(stderr, "can't create floppy\n");
        adfUnMountDev(hd);
        adfEnvCleanUp(); exit(1);
    }

    vol = adfMount(hd, 0, FALSE);
    if (!vol) {
        adfUnMountDev(hd);
        fprintf(stderr, "can't mount volume\n");
        adfEnvCleanUp(); exit(1);
    }

    adfDeviceInfo(hd);

    adfVolumeInfo(vol);


    cell = list = adfGetDirEnt(vol,vol->curDirPtr);
    while(cell) {
        printEntry(cell->content);
        cell = cell->next;
    }
    adfFreeDirList(list);

    putchar('\n');

    adfCreateDir(vol,vol->curDirPtr,"dir_5u");

    adfCreateDir(vol,883,"dir_51");

    adfCreateDir(vol,vol->curDirPtr,"toto");
printf("[dir = %ld]\n",vol->curDirPtr);
    cell = list = adfGetDirEnt(vol, vol->curDirPtr);
    while(cell) {
        printEntry(cell->content);
        cell = cell->next;
    }
    adfFreeDirList(list);
printf("[dir = %ld]\n",883L);
    cell = list = adfGetDirEnt(vol,883);
    while(cell) {
        printEntry(cell->content);
        cell = cell->next;
    }
    adfFreeDirList(list);

    adfRenameEntry(vol, 883,"dir_51", vol->curDirPtr,"dir_55");
putchar('\n');

printf("[dir = %ld]\n",vol->curDirPtr);
    cell = list = adfGetDirEnt(vol, vol->curDirPtr);
    while(cell) {
        printEntry(cell->content);
        cell = cell->next;
    }
    adfFreeDirList(list);
printf("[dir = %ld]\n",883L);
    cell = list = adfGetDirEnt(vol,883);
    while(cell) {
        printEntry(cell->content);
        cell = cell->next;
    }
    adfFreeDirList(list);

    adfRenameEntry(vol, vol->curDirPtr,"toto", 883,"moved_dir");

putchar('\n');

printf("[dir = %ld]\n",vol->curDirPtr);
    cell = list = adfGetDirEnt(vol, vol->curDirPtr);
    while(cell) {
        printEntry(cell->content);
        cell = cell->next;
    }
    adfFreeDirList(list);
printf("[dir = %ld]\n",883L);
    cell = list = adfGetDirEnt(vol,883);
    while(cell) {
        printEntry(cell->content);
        cell = cell->next;
    }
    adfFreeDirList(list);

	    adfUnMount(vol);
    adfUnMountDev(hd);

    adfEnvCleanUp();

    return 0;
}
