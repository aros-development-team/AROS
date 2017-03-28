/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <aros/shcommands.h>

AROS_SH0(autopipe, 1.0)
{
    AROS_SHCOMMAND_INIT

    int err = RETURN_FAIL;

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

    return err;

    AROS_SHCOMMAND_EXIT
}

            
