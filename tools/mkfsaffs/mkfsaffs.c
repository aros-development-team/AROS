#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef linux
# include <linux/fs.h>
# include <fcntl.h>
# include <sys/ioctl.h>
#endif

#include <adflib.h>


int main(int argc, char *argv[])
{
    struct Device *hd;
    struct Volume *vol;
    int fd;
    long size;

    printf("mkfsaffs v0.02\n");

    if (argc != 3)
    {
        printf("Usage: mkfsaffs <devname> <volname>\n");
        exit(1);
    }

    adfEnvInitDefault();

#ifdef linux
    fd = open(argv[1],O_RDONLY);
    if (fd < 0)
    {
        printf("open failed\n");
        exit (1);
    }
    if (ioctl(fd,BLKGETSIZE,&size) < 0)
    {
        printf("Could not get size\n");
        exit(1);
    }
    close(fd);
#else
    printf("OS not supported!\n");
#endif

    hd = adfCreateDumpDevice(argv[1], size, 1,1);
    if (!hd) {
        fprintf(stderr, "can't mount device\n");
        adfEnvCleanUp(); exit(1);
    }

    adfCreateHdFile( hd, argv[2], FSMASK_FFS );

    vol = adfMount(hd, 0, FALSE);
    if (!vol) {
        adfUnMountDev(hd);
        fprintf(stderr, "can't mount volume\n");
        adfEnvCleanUp(); exit(1);
    }
    adfVolumeInfo(vol);

    /* unmounts */
    adfUnMount(vol);
    adfUnMountDev(hd);


    adfEnvCleanUp();

    return 0;
}

