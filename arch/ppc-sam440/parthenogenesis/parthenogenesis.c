/*
 * Copyright (C) 2012, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <adf/adflib.h>
#include <adf/adf_hd.h>

#define SZ_1G   (1UL << 30)
#define SZ_2G   (1UL << 31)

static int usage(const char *program)
{
    printf("Usage:\n"
           "%s /dev/sdb /path/to/Parthnope [/path/to/AROS]\n"
           , program);
    return EXIT_FAILURE;
}

static void *openmem(const char *name, size_t *sizep)
{
    size_t size;
    int fd, len;
    void *mem;

    fd = open(name, O_RDONLY);
    if (fd < 0)
        return NULL;

    size = lseek(fd, 0, SEEK_END);
    if (size < 0)
        return NULL;
    lseek(fd, 0, SEEK_SET);
    mem = malloc(size);
    if (mem == NULL)
        return NULL;

    len = read(fd, mem, size);
    if (len != size) {
        free(mem);
        return NULL;
    }

    close(fd);

    *sizep = size;
    return mem;
}

static int copyToVolume(struct Volume *vol, const char *path, int depth)
{
    struct dirent *de;
    DIR *dir = opendir(path);
    char here[PATH_MAX];
    SECTNUM acd = adfCurrentDir(vol);
    int files = 0, rc;

    if (dir == NULL)
        return -1;

    getcwd(here, sizeof(here));
    chdir(path);

    while ((de = readdir(dir))) {
        struct stat st;
        if (strcmp(de->d_name,".") == 0 ||
            strcmp(de->d_name,"..") == 0)
            continue;
        if (stat(de->d_name, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                printf("D %*c%s", depth * 2, ' ', de->d_name);
                if (adfCreateDir(vol, acd, de->d_name) == RC_OK) {
                    if (adfChangeDir(vol, de->d_name) == RC_OK) {
                        printf("\n");
                        rc = copyToVolume(vol, de->d_name, depth+1);
                        if (rc >= 0)
                            files+=rc;
                        adfParentDir(vol);
                    } else {
                        printf(": Can't ChDir\n");
                    }
                } else {
                    printf(": Can't create\n");
                }
            } else if (S_ISREG(st.st_mode)) {
                int fd = open(de->d_name, O_RDONLY);
                printf("F %*c%s\n", depth * 2, ' ', de->d_name);
                if (fd >= 0) {
                    struct File *file = adfOpenFile(vol, de->d_name, "w");
                    if (file) {
                        char buff[256];
                        int len;
                        while ((len = read(fd, buff, 256)) > 0) {
                            adfWriteFile(file, len, buff);
                        }
                        adfCloseFile(file);
                        files++;
                    }
                }
            }
        }
    }

    chdir(here);

    return files;
}

int main(int argc, char **argv)
{
    size_t size;
    void *code;
    struct Device *dev;
    DIR *dir;
    struct Partition part_boot = {
        .volName = "DH0",
        .volType = { 'D', 'O', 'S', 3 },  /* DOS\03 */
    };
    struct Partition part_work = {
        .volName = "DH1",
        .volType = { 'D', 'O', 'S', 3 },  /* DOS\03 */
    };
    struct Partition *part[] = { &part_boot, &part_work };

    if (argc != 3 && argc != 4) {
        return usage(argv[0]);
    }

    code = openmem(argv[2], &size);
    if (!code) {
        perror(argv[2]);
        return EXIT_FAILURE;
    }

    adfEnvInitDefault();

    dev = adfMountDev(argv[1], FALSE);
    if (dev && argc == 3) {
        if (adfWriteBOOT(dev, code, size) == RC_OK) {
            printf("SLB updated\n");
        }
        free(code);
        adfUnMountDev(dev);
    } else if (dev) {
        ULONG lastcyl;
        int parts = 2;

        lastcyl = SZ_1G / 512 / dev->sectors / dev->heads;
        if (lastcyl > dev->cylinders) {
            lastcyl = dev->cylinders;
            parts = 1;
        }
        part_boot.startCyl = 2;
        part_boot.lenCyl = lastcyl - part_boot.startCyl;

        part_work.startCyl = lastcyl;
        part_work.lenCyl = dev->cylinders - lastcyl;

        if (adfCreateHd(dev, parts, part) == RC_OK) {
            /* Add Parthenope to the boot blocks */
            if (adfWriteBOOT(dev, code, size) == RC_OK) {
                struct Volume *vol;
                
                if ((vol = adfMount(dev, 0, FALSE))) {
                    int files = copyToVolume(vol, argv[3], 0);
                    if (files >= 0) {
                        printf("Copied %d files\n", files);
                    }
                    adfVolumeInfo(vol);
                    adfUnMount(vol);
                }
            }
            free(code);
        }
        adfUnMountDev(dev);
    }

    adfEnvCleanUp();

    return 0;
}
