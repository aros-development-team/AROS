/*
    Copyright © 2008, The AROS Development Team. All rights reserved.
    $Id$

    Test for OpenFromLock(), NameFromLock(), NameFromFH() and DupLockFromFH().
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
    LONG error = 0;
    BPTR fh, lock;
    TEXT buffer[100];

    if (argc != 2) {
        printf("usage: %s filename\n", argv[0]);
        return 1;
    }

    lock = Lock(argv[1], SHARED_LOCK);
    if (lock == BNULL) {
        PrintFault(IoErr(), "openfromlock");
        return 0;
    }

    fh = OpenFromLock(lock);
    if (fh == BNULL) {
        PrintFault(IoErr(), "openfromlock");
        UnLock(lock);
        return 0;
    }

    if (NameFromFH(fh, buffer, 100))
        Printf("Name from filehandle: %s\n", buffer);
    else
    {
        PutStr("Couldn't get name from file handle.\n");
        error = IoErr();
    }

    if (error == 0)
    {
        lock = DupLockFromFH(fh);
        if (lock == BNULL)
        {
            PutStr("Couldn't duplicate lock from file handle.\n");
            error = IoErr();
        }
    }

    if (error == 0)
    {
        if (NameFromLock(lock, buffer, 100))
            Printf("Name from lock: %s\n", buffer);
        else
            PutStr("Couldn't get name from lock.\n");
        UnLock(lock);
    }

    Close(fh);

    return 0;
}
