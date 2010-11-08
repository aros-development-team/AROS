/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>

typedef uint8_t  UBYTE;
typedef uint32_t ULONG;

struct BootBlock {
    UBYTE       DiskType[4];
    ULONG       Chksum;
    ULONG       RootBlock;
    UBYTE       BootBlockCode[0];
} __attribute__((packed));

const UBYTE m68k_boot[] = {
    0x43, 0xfa, 0x00, 0x18,   0x4e, 0xae, 0xff, 0xa0,
    0x4a, 0x80, 0x67, 0x0a,   0x20, 0x40, 0x20, 0x68,
    0x00, 0x16, 0x70, 0x00,   0x4e, 0x75, 0x70, 0xff,
    0x60, 0xfa, 0x64, 0x6f,   0x73, 0x2e, 0x6c, 0x69,
    0x62, 0x72, 0x61, 0x72,   0x79, 0x00 };

#define BOOTBLOCKSIZE   1024

/* Adapted from rom/boot/strap.c */
static void BootBlockChecksum(UBYTE *bootblock, size_t size)
{
       ULONG crc = 0;
       int i;

       memset(bootblock+4, 0, 4);

       for (i = 0; i < size; i += 4) {
           ULONG v = (bootblock[i] << 24) | (bootblock[i + 1] << 16) |
(bootblock[i + 2] << 8) | bootblock[i + 3];
           if (crc + v < crc)
               crc++;
           crc += v;
       }
       crc ^= 0xffffffff;

       bootblock[4+0] = (crc >> 24) & 0xff;
       bootblock[4+1] = (crc >> 16) & 0xff;
       bootblock[4+2] = (crc >>  8) & 0xff;
       bootblock[4+3] = (crc >>  0) & 0xff;
}


int main(int argc, char **argv)
{
    int fd, err, i;
    const char *image = argv[1];
    UBYTE buff[BOOTBLOCKSIZE];
    struct BootBlock *bootblock = (void *)&buff[0];
    ULONG csum, psum;

    fd = open(image, O_RDWR);
    if (fd < 0) {
        perror(image);
        return EXIT_FAILURE;
    }

    err = read(fd, buff, sizeof(buff)); 
    if (err < 0) {
        perror(image);
        close(fd);
    }

    lseek(fd, 0, SEEK_SET);

    if (memcmp(bootblock->DiskType, "DOS", 3) != 0) {
        fprintf(stderr, "%s: Not a DOS (OFS or FFS) disk\n", image);
        close(fd);
        return EXIT_FAILURE;
    }

    memset(bootblock->BootBlockCode, 0, sizeof(buff)-sizeof(struct BootBlock));

    memcpy(bootblock->BootBlockCode, m68k_boot, sizeof(m68k_boot));

    BootBlockChecksum(buff, sizeof(buff));
       
    err = write(fd, buff, 1024);
    if (err < 0) {
        perror(image);
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);
    return EXIT_SUCCESS;
}
