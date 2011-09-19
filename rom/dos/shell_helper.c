/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <aros/debug.h>
#include <utility/tagitem.h>
#include <dos/dostags.h>
#include <proto/utility.h>
#include <dos/dosextens.h>
#include <aros/asmcall.h>
#include <exec/ports.h>

#include "dos_intern.h"

#include <proto/dos.h>

static BPTR findseg_any(CONST_STRPTR res, CONST_STRPTR file, struct DosLibrary *DOSBase)
{
    BPTR found = BNULL;
    struct Segment *seg;

    D(bug("[%s] Looking for shell %s (aka %s)\n", __func__, res, file));

    Forbid();
    seg = FindSegment(res, NULL, TRUE);
    if (seg != NULL && seg->seg_UC <= 0) {
        found = seg->seg_Seg;
        D(bug("[%s] Found %s in DOS Resident list\n", __func__, res));
    }
    Permit();

    /* Ok, *really* digging for it now.
     */
    if (!found) {
        found = LoadSeg(file);
        if (found) {
            /* We're going to keep this around for a long while. */
            D(bug("[%s] Found %s on disk\n", __func__, file));
            AddSegment(res, found, CMD_SYSTEM);
        } else {
            D(bug("[%s] No such segment available\n", __func__));
        }
    }

    return found;
}

BPTR findseg_cli(BOOL isBoot, struct DosLibrary *DOSBase)
{
    BPTR seg = BNULL;

    if (isBoot)
        seg = DOSBase->dl_Root->rn_ConsoleSegment;

    if (!seg) {
        seg = findseg_any("CLI", "L:Shell-Seg", DOSBase);
        if (seg)
            DOSBase->dl_Root->rn_ConsoleSegment = seg;
    }

    return seg;
}

BPTR findseg_shell(BOOL isBoot, struct DosLibrary *DOSBase)
{
    BPTR seg = BNULL;

    if (isBoot)
        seg = DOSBase->dl_Root->rn_ShellSegment;

    if (!seg) {
        seg = findseg_any("shell", "L:UserShell-Seg", DOSBase);
        if (seg)
            DOSBase->dl_Root->rn_ShellSegment = seg;
    }

    return seg;
}
