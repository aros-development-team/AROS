/*
 * cache_test.c
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
    struct File *file;
    unsigned char buf[600];
    long n;
    FILE *in;
    long len;
     struct List *list;

    adfEnvInitDefault();

//	adfSetEnvFct(0,0,MyVer,0);

    /* mount existing device : FFS */
    hd = adfMountDev( "testffs.bak",FALSE );
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

    list = adfGetDirEnt(vol, vol->curDirPtr);
    while(list) {
        printEntry(list->content);
        adfFreeEntry(list->content);
        list = list->next;
    }
    freeList(list);

//    adfCreateDir(vol,vol->curDirPtr,"dir_1a");
    adfCreateDir(vol,vol->curDirPtr,"dir_1b");



    file = adfOpenFile(vol, "newfile","w");
    adfCloseFile(file);

    file = adfOpenFile(vol, "moon_gif","w");
    if (!file) return 1;
    in = fopen("Check/MOON.GIF","rb");
    if (!in) return 1;
    
    len = 600;
    n = fread(buf,sizeof(unsigned char),len,in);
    while(!feof(in)) {
        adfWriteFile(file, n, buf);
        n = fread(buf,sizeof(unsigned char),len,in);
    }
    if (n>0)
        adfWriteFile(file, n, buf);

//    adfFlushFile(file);

    adfCloseFile(file);
    fclose(in);

    adfRemoveEntry(vol,vol->curDirPtr, "dir_1b");

    list = adfGetDirEnt(vol, vol->curDirPtr);
    while(list) {
        printEntry(list->content);
        adfFreeEntry(list->content);
        list = list->next;
    }
    freeList(list);

    adfUnMount(vol);
    adfUnMountDev(hd);


    adfEnvCleanUp();

    return 0;
}
