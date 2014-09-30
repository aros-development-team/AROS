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
    if (fh == BNULL) {
        PrintFault(IoErr(), "SetFileSize");
        return 0;
    }

    size = SetFileSize(fh, atol(argv[2]), OFFSET_BEGINNING);
    if (size < 0) {
        PrintFault(IoErr(), "SetFileSize");
        Close(fh);
        return 0;
    }

    Printf("New size is %ld bytes\n", size);

    Close(fh);

    return 0;
}
