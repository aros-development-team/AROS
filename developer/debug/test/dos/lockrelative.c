/*
    Copyright (C) 1995-2021, The AROS Development Team. All rights reserved.
*/

#include <stdio.h>
#include <proto/dos.h>

int main (int argc, char **argv) {
    if (argc < 3) {
        printf("usage: lockrelative <base> <file>\n");
        return 1;
    }

    BPTR baselock = Lock(argv[1], ACCESS_READ);
    if (baselock == BNULL) {
        PrintFault(IoErr(), "lockrelative: base lock");
        return 1;
    }

    BPTR filelock = LockRelative(baselock, argv[2], ACCESS_READ);
    if (filelock == BNULL) {
        PrintFault(IoErr(), "lockrelative: file lock");
        UnLock(baselock);
        return 1;
    }

    char path[256];
    if (NameFromLock(filelock, path, sizeof(path)) == 0) {
        PrintFault(IoErr(), "lockrelative: name from lock");
        UnLock(filelock);
        UnLock(baselock);
        return 1;
    }

    printf("%s\n", path);

    UnLock(filelock);
    UnLock(baselock);
    return 0;
}

