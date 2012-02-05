/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 *
 * Code related to Gfx printing
 */

#include <string.h>

#include <aros/printertag.h>

#include <proto/exec.h>
#include <proto/arossupport.h>

#include <SDI/SDI_hook.h>

#include "printer_intern.h"

/* Default function that generates a raster line
 *
 * AROS differs from AOS 3.9, in that it passes the 
 * hook the struct IODRPReq * for the command, instead
 * of NULL, as the object argument.
 */
HOOKPROTONH(rastSource, VOID, const struct IODRPReq *io, struct DRPSourceMsg *msg)
{
    /* Clear the buffer */
    memset(msg->buf, 0, msg->width * msg->height * sizeof (ULONG));
    return;
}

MakeStaticHook(rastSourceHook, rastSource);

LONG  Printer_Gfx_DumpRPort(const struct IODRPReq *io, struct TagItem *tags)
{
    struct PrinterData *pd = (struct PrinterData *)io->io_Device;
    struct PrinterExtendedData *ped = &pd->pd_SegmentData->ps_PED;
    LONG (*Render)(SIPTR ct, LONG x, LONG y, LONG status) = ped->ped_Render;
    struct Hook *srcHook = &rastSourceHook;
    struct TagItem *tag;
    LONG aspectX = 1, aspectY = 1;
    LONG err;

    if (!(ped->ped_PrinterClass & 1) || Render == NULL) {
        /* Not graphics. */
        return PDERR_NOTGRAPHICS;
    }

    while ((tag = LibNextTagItem(&tags))) {
        switch (tag->ti_Tag) {
            case DRPA_SourceHook:
                srcHook = (struct Hook *)tag->ti_Data;
                break;
            case DRPA_AspectX:
                aspectX = tag->ti_Data;
                break;
            case DRPA_AspectY:
                aspectY = tag->ti_Data;
                break;
            default:
                break;
        }
    }

    (void)aspectX;
    (void)aspectY;
    (void)srcHook;

    /* Initialize page for printing */
    if (!(err = Render((SIPTR)io, io->io_SrcWidth, io->io_SrcHeight, PRS_INIT))) {
        err = Render(err, io->io_Special, 0, PRS_CLOSE);
    }

    return 0;
}


