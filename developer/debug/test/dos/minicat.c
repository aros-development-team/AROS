/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/bptr.h>
#include <proto/dos.h>

int main(int argc, char **argv) {
    BPTR in, out;
    char buf[256];
    LONG len;

    if (argc > 1) {
        if ((in = Open(argv[1], MODE_OLDFILE)) == BNULL) {
            Fault(IoErr(), "minicat", buf, 255);
            fprintf(stderr, "%s\n", buf);
            return 1;
        }
    }
    else
        in = Input();

    out = Output();

    while ((len = Read(in, buf, 256)) > 0)
        Write(out, buf, len);

    if (argc > 1)
        Close(in);

    return 0;
}
