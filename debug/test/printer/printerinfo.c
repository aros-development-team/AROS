/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <proto/dos.h>
#include <proto/exec.h>
#include <devices/prtbase.h>

#include <aros/shcommands.h>

AROS_SH0(printerinfo, 1.0)
{
    AROS_SHCOMMAND_INIT

    struct {
        CONST_STRPTR file;
    } Args = { };
    struct RDArgs *ra;
    struct Library *DOSBase;

    if (!(DOSBase = OpenLibrary("dos.library", 0)))
        return RETURN_FAIL;

    if (!(ra = ReadArgs("FILE", (IPTR *)&Args, NULL)))
        return RETURN_FAIL;

    if (Args.file) {
        BPTR seg = LoadSeg(Args.file);
        if (seg) {
            struct PrinterSegment *pseg = BADDR(seg);
            struct PrinterExtendedData *ped = &pseg->ps_PED;
            int i;

            Printf("%s:\n");
            Printf("  ps_runAlert = 0x%08lx\n", pseg->ps_runAlert);
            Printf("  ps_Version  = %ld\n", pseg->ps_Version );
            Printf("  ps_Revision = %ld\n", pseg->ps_Revision);
            Printf("  ped_PrinterName = \"%s\"\n", ped->ped_PrinterName);

            Printf("  ped_DoSpecial= 0x%08lx\n", ped->ped_DoSpecial);
            Printf("  ped_ConvFunc = 0x%08lx\n", ped->ped_ConvFunc);
            Printf("  ped_Commands = 0x%08lx\n", ped->ped_Commands);
            for (i = 0; i < 76; i++) {
                if (ped->ped_Commands[i][0] == 255)
                    continue;
                Printf("  ped_Commands[%ld] = \"%s\"\n", i, ped->ped_Commands[i]);
            }

            UnLoadSeg(seg);
        }
    }

    FreeArgs(ra);
    CloseLibrary(DOSBase);

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}

