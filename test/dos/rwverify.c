/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <dos/dosextens.h>
#include <dos/bptr.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>

#define CHUNK 16

void hexdump(UBYTE *buf, int size) {
    int i,j;
    int count;

    for (i = 0; i < size; i += CHUNK) {
        printf("0x%06x    ", i);

        count = ((size-i) > CHUNK ? CHUNK : size-i);

        for (j = 0; j < count; j++) {
            if (j == CHUNK / 2) fputc(' ', stdout);
            printf("%02x ", buf[i+j]);
        }

        for (j = count; j < CHUNK; j++) {
            if (j == CHUNK / 2) fputc(' ', stdout);
            fputs("     ", stdout);
        }

        fputc('\n', stdout);
    }
}

int round1(char *filename) {
    BPTR fh;
    UBYTE buf[1024];
    LONG bytes;
    int i, n;

    fputs("\nround 1: 1024 buffers of 1024 bytes each\n\n", stdout);

    fh = Open(filename, MODE_NEWFILE);
    if (fh == BNULL) {
        printf("couldn't open '%s' for write (%ld)\n", filename, (long)IoErr());
        return 1;
    }

    printf("opened '%s' for write\n", filename);

    fputs("writing... ", stdout);
    fflush(stdout);

    for (n = 0; n < 1024; n++) {
        for (i = 0; i < 1024; i++)
            buf[i] = (i + n) & 0xff;

        bytes = Write(fh, buf, 1024);
        if (bytes < 0) {
            printf("buffer %d: error (%ld)\n", n, (long)IoErr());
            Close(fh);
            return 1;
        }

        if (bytes < 1024) {
            printf("buffer %d: short write! error is %ld\n", n, (long)IoErr());
            Close(fh);
            return 1;
        }
    }

    fputs("1024 buffers written\n", stdout);

    Close(fh);

    fh = Open(filename, MODE_OLDFILE);
    if (fh == BNULL) {
        printf("couldn't open '%s' for read (%ld)\n", filename, (long)IoErr());
        return 1;
    }

    printf("opened '%s' for read\n", filename);

    fputs("reading... ", stdout);
    fflush(stdout);

    for (n = 0; n < 1024; n++) {
        bytes = Read(fh, buf, 1024);
        if (bytes < 0) {
            printf("buffer %d: error (%ld)\n", n, (long)IoErr());
            Close(fh);
            return 1;
        }
        if (bytes < 1024) {
            printf("buffer %d: short read! error is %ld\n", n, (long)IoErr());
            Close(fh);
            return 1;
        }

        for (i = 0; i < 1024; i++)
            if (buf[i] != ((i + n) & 0xff)) {
                printf("buffer %d: verify error!\n  file pos %d, expected 0x%02x, got 0x%02x\n", n, n * 1024 + i, ((i + n) & 0xff), buf[i]);
                printf("buffer dump (0x%02x bytes):\n", 1024);
                hexdump(buf, 1024);
                Close(fh);
                return 1;
            }
    }

    fputs("1024 buffers read and verified\n", stdout);

    Close(fh);

    DeleteFile(filename);

    fputs("cleaned up\n", stdout);

    return 0;
}

int round2(char *filename) {
    BPTR fh;
    UBYTE buf[1024];
    LONG bytes;
    int i, n;

    fputs("\nround 2: 8192 buffers, increasing length from 0-1023 bytes\n\n", stdout);

    fh = Open(filename, MODE_NEWFILE);
    if (fh == BNULL) {
        printf("couldn't open '%s' for write (%ld)\n", filename, (long)IoErr());
        return 1;
    }

    printf("opened '%s' for write\n", filename);

    fputs("writing... ", stdout);
    fflush(stdout);

    for (n = 0; n < 8192; n++) {
        for (i = 0; i < (n & 1023); i++)
            buf[i] = (i + n) & 0xff;

        bytes = Write(fh, buf, (n & 1023));
        if (bytes < 0) {
            printf("buffer %d: error (%ld)\n", n, (long)IoErr());
            Close(fh);
            return 1;
        }

        if (bytes < (n & 1023)) {
            printf("buffer %d: short write! error is %ld\n", n, (long)IoErr());
            Close(fh);
            return 1;
        }
    }

    fputs("1024 buffers written\n", stdout);

    Close(fh);

    fh = Open(filename, MODE_OLDFILE);
    if (fh == BNULL) {
        printf("couldn't open '%s' for read (%ld)\n", filename, (long)IoErr());
        return 1;
    }

    printf("opened '%s' for read\n", filename);

    fputs("reading... ", stdout);
    fflush(stdout);

    for (n = 0; n < 8192; n++) {
        bytes = Read(fh, buf, (n & 1023));
        if (bytes < 0) {
            printf("buffer %d: error (%ld)\n", n, (long)IoErr());
            Close(fh);
            return 1;
        }
        if (bytes < (n & 1023)) {
            printf("buffer %d: short read! error is %ld\n", n, (long)IoErr());
            Close(fh);
            return 1;
        }

        for (i = 0; i < (n & 1023); i++)
            if (buf[i] != ((i + n) & 0xff)) {
                printf("buffer %d: verify error!\n  file pos %d, expected 0x%02x, got 0x%02x\n", n, n * (n & 1023) + i, ((i + n) & 0xff), buf[i]);
                printf("buffer dump (0x%02x bytes):\n", (n & 1023));
                hexdump(buf, 1024);
                Close(fh);
                return 1;
            }
    }

    fputs("8192 buffers read and verified\n", stdout);

    Close(fh);

    //DeleteFile(filename);

    fputs("cleaned up\n", stdout);

    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("usage: %s filename\n", argv[0]);
        return 1;
    }

    if (round1(argv[1]) || round2(argv[1]))
        return 1;

    return 0;
}
