/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <aros/debug.h>
#include <aros/printertag.h>

#include <exec/errors.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/asl.h>

#include <devices/printer.h>
#include <datatypes/datatypesclass.h>

#include LC_LIBDEFS_FILE

#include "printer_intern.h"

static int GM_UNIQUENAME(Init)(struct PrinterBase *PrinterBase)
{
    int i;

    D(bug("init printer.device\n"));
    for (i = 0; i < PRINTER_UNITS; i++) {
        InitSemaphore(&PrinterBase->pb_UnitLock[i]);
    }

    if ((PrinterBase->pb_DOSBase = OpenLibrary("dos.library", 35))) {
        D(bug("initted\n"));
        return TRUE;
    }

    return FALSE;
}

static int GM_UNIQUENAME(Expunge)(struct PrinterBase *PrinterBase)
{
    CloseLibrary(PrinterBase->pb_DOSBase);
    D(bug("expunged\n"));

    return TRUE;
}

/* Opening a unit of PrinterBase creates a struct PrinterData * device for
 * it, with its driver and its task, or joins the one that is there: a unit
 * is shared, and its job runs from the first open to the last close.
 */
static int GM_UNIQUENAME(OpenDevice)(struct PrinterBase *PrinterBase, union printerIO *pio, LONG unitnum, ULONG flags)
{
    struct PrinterUnit *pu = NULL;
    struct IOStdReq *io = &pio->ios;

    D(bug("open unit %d\n", unitnum));

    if (unitnum < 0) {
        /* 'system' printer device */
        io->io_Unit = (struct Unit *)PrinterBase;
        return TRUE;
    }

    if (unitnum < 0 || unitnum >= PRINTER_UNITS)
        return FALSE;

    /* The unit task takes this lock for the whole of the last close, so a
     * unit found here stays until our own close, and one that is closing
     * down has gone by the time we get the lock.
     */
    ObtainSemaphore(&PrinterBase->pb_UnitLock[unitnum]);
    pu = PrinterBase->pb_Unit[unitnum];
    if (pu == NULL) {
        pu = Printer_Unit(PrinterBase,(LONG)unitnum);
        PrinterBase->pb_Unit[unitnum] = pu;
    }
    if (pu != NULL)
        pu->pu_Opens++;
    ReleaseSemaphore(&PrinterBase->pb_UnitLock[unitnum]);

    if (pu) {
        io->io_Device = (struct Device *)pu;
        io->io_Unit   = (struct Unit *)(IPTR)unitnum;
        io->io_Error  = 0;
        AROS_LVO_CALL3NR(void,
                AROS_LCA(struct IORequest *,(struct IORequest *)io,A1),
                AROS_LCA(IPTR, unitnum, D0),
                AROS_LCA(ULONG, flags, D1),
                struct Device *, (struct Device *)pu, 1 , dev);
        if (io->io_Error) {
            /* The unit's close gives back an open of printer.device, and
             * this one is not counted: that happens when we return TRUE.
             */
            Forbid();
            PrinterBase->pb_Device.dd_Library.lib_OpenCnt++;
            Permit();
            CloseDevice((struct IORequest *)io);
        }
    } else {
        io->io_Error = IOERR_OPENFAIL;
    }

    D(bug("io device %p (%d)\n", io->io_Device, io->io_Error));
    return (io->io_Error == 0) ? TRUE : FALSE;
}

static int GM_UNIQUENAME(CloseDevice)(struct PrinterBase *PrinterBase, union printerIO *io)
{
    return TRUE;
}

AROS_LH1(void, BeginIO,
        AROS_LHA(struct IORequest *, io, A1),
        struct PrinterBase *, PrinterBase, 5, Printer)
{
    AROS_LIBFUNC_INIT

    D(bug("System BeginIO: io_Command = %d\n", io->io_Command));

    switch (io->io_Command) {
    default:
        io->io_Error = IOERR_NOCMD;
        break;
    }

    if (!(io->io_Flags & IOF_QUICK))
        ReplyMsg(&io->io_Message);

    return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AbortIO,
        AROS_LHA(struct IORequest *, io, A1),
        struct PrinterBase *, PrinterBase, 6, Printer)
{
    AROS_LIBFUNC_INIT
    return IOERR_NOCMD;
    AROS_LIBFUNC_EXIT
}

 
ADD2OPENDEV(GM_UNIQUENAME(OpenDevice), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(CloseDevice), 0)

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge), 0)
