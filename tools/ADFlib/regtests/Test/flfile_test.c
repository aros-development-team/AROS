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
    struct File *file;
    unsigned char buf[600];
    long n;
    FILE *out;
    long len;
    struct List *list;

    adfEnvInitDefault();


    /* create and mount one device */
    hd = adfCreateDumpDevice("newdev", 80, 11, 2);
    if (!hd) {
        fprintf(stderr, "can't mount device\n");
        adfEnvCleanUp(); exit(1);
    }
    adfCreateFlop( hd, "empty", FSMASK_FFS|FSMASK_DIRCACHE );

    vol = adfMount(hd, 0, FALSE);
    if (!vol) {
        adfUnMountDev(hd);
        fprintf(stderr, "can't mount volume\n");
        adfEnvCleanUp(); exit(1);
    }

    adfVolumeInfo(vol);

    /* the directory */
    list = adfGetDirEnt(vol,vol->curDirPtr);
    while(list) {
        printEntry(list->content);
        adfFreeEntry(list->content);
        list = list->next;
    }
    freeList(list);

    /* write one file */
    file = adfOpenFile(vol, "moon_gif","w");
    if (!file) return 1;
    out = fopen("Check/MOON.GIF","rb");
    if (!out) return 1;
    
    len = 600;
    n = fread(buf,sizeof(unsigned char),len,out);
    while(!feof(out)) {
        adfWriteFile(file, n, buf);
        n = fread(buf,sizeof(unsigned char),len,out);
    }
    if (n>0)
        adfWriteFile(file, n, buf);

    fclose(out);

    adfCloseFile(file);

    /* the directory */
    list = adfGetDirEnt(vol,vol->curDirPtr);
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
