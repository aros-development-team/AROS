/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>

__startup int _main(void)
{
    struct Library *DOSBase;
    int err = RETURN_FAIL;

    if ((DOSBase = OpenLibrary("dos.library", 0))) {
        BPTR fhw;
        if ((fhw = Open("PIPE:*", MODE_NEWFILE))) {
            TEXT name[64];

            if (NameFromFH(fhw, name, sizeof(name)) != 0) {
                BPTR fhr;

                Printf("PIPE:* => %s\n", name);
                if ((fhr = Open(name, MODE_OLDFILE))) {
                    Write(fhw, "Hello", 5);
                    Read(fhr, name, 5);
                    name[5] = 0;
                    Close(fhr);
                    Printf("Hello => %s\n", name);
                    err = 0;
                }
            } else {
                Printf("Can't get name, Error %d\n", IoErr());
            }
            Close(fhw);
        } else {
            Printf("Can't open PIPE:*\n");
        }
        CloseLibrary(DOSBase);
    }

    return err;
}

            
