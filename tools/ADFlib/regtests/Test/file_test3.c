/*
 * dir_test.c
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

//	adfSetEnvFct(0,0,MyVer,0);

    /* mount existing device : OFS */
    hd = adfMountDev( argv[1],FALSE );

    vol = adfMount(hd, 0, FALSE);

    adfVolumeInfo(vol);


    /* write one file */
    file = adfOpenFile(vol, "moon_gif","w");
    if (!file) return 1;
    out = fopen( argv[2],"rb");
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


    /* re read this file */
    file = adfOpenFile(vol, "moon_gif","r");
    if (!file) return 1;
    out = fopen("moon__gif","wb");
    if (!out) return 1;

    len = 300;
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
