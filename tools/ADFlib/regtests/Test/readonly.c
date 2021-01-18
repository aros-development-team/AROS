/*
 * del_test.c
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
    struct List *list;
    SECTNUM nSect;
 
    adfEnvInitDefault();

    /* mount existing device */
    hd = adfMountDev( argv[1], FALSE );
    if (!hd) {
        fprintf(stderr, "can't mount device\n");
        adfEnvCleanUp(); exit(1);
    }

    vol = adfMount(hd, 0, FALSE);
    if (!vol) {
        adfUnMountDev(hd);
        fprintf(stderr, "can't mount volume\n");
        adfEnvCleanUp(); exit(1);
    }
	
    adfVolumeInfo(vol);

    list = adfGetDirEnt(vol,vol->curDirPtr);
    while(list) {
        printEntry(list->content);
        adfFreeEntry(list->content);
        list = list->next;
    }
    freeList(list);

    putchar('\n');

    adfCreateDir(vol,vol->curDirPtr,"newdir");

    /* cd dir_2 */
    nSect = adfChangeDir(vol, "same_hash");

    list = adfGetDirEnt(vol,vol->curDirPtr);
    while(list) {
        printEntry(list->content);
        adfFreeEntry(list->content);
        list = list->next;
    }
    freeList(list);

    putchar('\n');

    /* not empty */
    adfRemoveEntry(vol, vol->curDirPtr, "mon.paradox");

    /* first in same hash linked list */
    adfRemoveEntry(vol, vol->curDirPtr, "file_3a");
    /* second */
    adfRemoveEntry(vol, vol->curDirPtr, "dir_3");
    /* last */
    adfRemoveEntry(vol, vol->curDirPtr, "dir_1a");

    list = adfGetDirEnt(vol,vol->curDirPtr);
    while(list) {
        printEntry(list->content);
        adfFreeEntry(list->content);
        list = list->next;
    }
    freeList(list);

    putchar('\n');

    adfParentDir(vol);

    adfRemoveEntry(vol, vol->curDirPtr, "mod.And.DistantCall");

    list = adfGetDirEnt(vol,vol->curDirPtr);
    while(list) {
        printEntry(list->content);
        adfFreeEntry(list->content);
        list = list->next;
    }
    freeList(list);

    putchar('\n');

    adfVolumeInfo(vol);

    adfUnMount(vol);
    adfUnMountDev(hd);


    adfEnvCleanUp();

    return 0;
}
