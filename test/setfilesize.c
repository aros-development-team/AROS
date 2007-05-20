#include <exec/types.h>
#include <dos/dosextens.h>
#include <dos/bptr.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    BPTR fh;
    LONG size;

    if (argc != 3) {
        printf("usage: %s filename newsize\n", argv[0]);
        return 1;
    }

    fh = Open(argv[1], MODE_READWRITE);
    if (fh == NULL) {
        printf("Open failed: %ld\n", IoErr());
        return 0;
    }

    size = SetFileSize(fh, atol(argv[2]), OFFSET_BEGINNING);
    if (size < 0) {
        printf("SetFileSize: %ld\n", IoErr());
        return 0;
    }

    printf ("new size is %ld bytes\n", size);

    Close(fh);

    return 0;
}
