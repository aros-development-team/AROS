/*
 *  comment.c
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
    struct File *fic;
    unsigned char buf[1];
    struct List *list, *cell;
    BOOL true = TRUE;
 
    adfEnvInitDefault();

    /* create and mount one device */
    hd = adfCreateDumpDevice("newdev", 80, 2, 11);
    if (!hd) {
        fprintf(stderr, "can't mount device\n");
        adfEnvCleanUp(); exit(1);
    }

    adfDeviceInfo(hd);

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

    fic = adfOpenFile(vol, "file_1a","w");
    if (!fic) { adfUnMount(vol); adfUnMountDev(hd); adfEnvCleanUp(); exit(1); }
    adfWriteFile(fic,1,buf);
    adfCloseFile(fic);

    adfVolumeInfo(vol);

    adfCreateDir(vol,vol->curDirPtr,"dir_5u");

    cell = list = adfGetDirEnt(vol, vol->curDirPtr);
    while(cell) {
        printEntry(cell->content);
        cell = cell->next;
    }
    adfFreeDirList(list);

    adfSetEntryComment(vol, vol->curDirPtr, "dir_5u", "dir_5u comment");
    adfSetEntryComment(vol, vol->curDirPtr, "file_1a", "file_1a very very long comment");

    putchar('\n');

    cell = list = adfGetDirEnt(vol,vol->curDirPtr);
    while(cell) {
        printEntry(cell->content);
        cell = cell->next;
    }
    adfFreeDirList(list);

    adfSetEntryComment(vol, vol->curDirPtr, "dir_5u", "");
    adfSetEntryComment(vol, vol->curDirPtr, "file_1a", "new comment" );

    putchar('\n');

    adfChgEnvProp(PR_USEDIRC, &true);

    cell = list = adfGetDirEnt(vol,vol->curDirPtr);
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
