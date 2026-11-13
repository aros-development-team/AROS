/*
 *  undel3.c
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
    struct GenBlock *block;
    BOOL true = TRUE;
    struct File *file;
    unsigned char buf[600];
    long n;
    FILE *out;
    long len;
  
    adfEnvInitDefault();

    adfChgEnvProp(PR_USEDIRC,&true);
 
    hd = adfMountDev(argv[1], FALSE);
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

    cell = list = adfGetDirEnt(vol, vol->curDirPtr);
    while(cell) {
        printEntry(cell->content);
        cell = cell->next;
    }
    adfFreeDirList(list);
    adfVolumeInfo(vol);

    puts("\nremove MOON.GIF");
    adfRemoveEntry(vol,vol->curDirPtr,"MOON.GIF");
    adfVolumeInfo(vol);

    cell = list = adfGetDelEnt(vol);
    while(cell) {
        block =(struct GenBlock*) cell->content;
       printf("%s %d %d %ld\n",block->name,block->type,block->secType,
            block->sect);
        cell = cell->next;
    }
    adfFreeDelList(list);

    adfCheckEntry(vol,884,0);
    adfUndelEntry(vol,vol->curDirPtr,884);
    puts("\nundel MOON.GIF");
    adfVolumeInfo(vol);

    cell = list = adfGetDirEnt(vol, vol->curDirPtr);
    while(cell) {
        printEntry(cell->content);
        cell = cell->next;
    }
    adfFreeDirList(list);

    file = adfOpenFile(vol, "MOON.GIF","r");
    if (!file) return 1;
    out = fopen("moon_gif","wb");
    if (!out) return 1;

    len = 600;
    n = adfReadFile(file, len, buf);
    while(!adfEndOfFile(file)) {
        fwrite(buf,sizeof(unsigned char),n,out);
        n = adfReadFile(file, len, buf);
    }
    if (n>0)
        fwrite(buf,sizeof(unsigned char),n,out);

    fclose(out);

    adfCloseFile(file);

    adfUnMount(vol);
    adfUnMountDev(hd);

    adfEnvCleanUp();

    return 0;
}
