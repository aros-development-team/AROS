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
 
    adfEnvInitDefault();

//	adfSetEnvFct(0,0,MyVer,0);

    /* mount existing device : FFS */
    hd = adfMountDev( argv[1],FALSE );
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

    file = adfOpenFile(vol, "mod.and.distantcall","r");
    if (!file) return 1;
    out = fopen("mod.distant","wb");
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

    file = adfOpenFile(vol, "emptyfile", "r");
    if (!file) { 
		adfUnMount(vol); adfUnMountDev(hd); 
        fprintf(stderr, "can't open file\n");
		exit(1); 
	}
 
    n = adfReadFile(file, 2, buf);

    adfCloseFile(file);


    adfUnMount(vol);
    adfUnMountDev(hd);


    /* ofs */

    hd = adfMountDev( argv[2],FALSE );
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

    file = adfOpenFile(vol, "moon.gif","r");
    if (!file) return 1;
    out = fopen("moon_gif","wb");
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
