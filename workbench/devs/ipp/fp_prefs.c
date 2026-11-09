/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    FindPrinters: points printer.device at an ipp.device unit through
    printspool.device, which spools the job and prints it in the background:
    printer.device unit 0 -> printspool.device unit n -> PrintSpool/Unit<n>
    = "ipp.device n". The prefs file is read and written as the Printer
    editor does (workbench/prefs/printer): the four chunks with their sizes
    checked, multi-byte fields big-endian. Only the unit chunk's device and
    unit change; the other chunks go back as they were read.
*/

#include <exec/types.h>
#include <aros/macros.h>
#include <dos/dos.h>
#include <prefs/prefhdr.h>
#include <prefs/printertxt.h>
#include <prefs/printergfx.h>
#include <exec/errors.h>
#include <devices/printer.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/iffparse.h>
#include <clib/alib_protos.h>

#include <string.h>

#include "fp_prefs.h"

struct FilePrefHeader
{
    UBYTE ph_Version;
    UBYTE ph_Type;
    UBYTE ph_Flags[4];
};

struct Chunks
{
    struct PrinterTxtPrefs txt;
    struct PrinterUnitPrefs unit;
    struct PrinterDeviceUnitPrefs devunit;
    struct PrinterGfxPrefs gfx;
};

/* The Printer editor's defaults for unit 0 (Prefs_Default), stored big-endian. */
static void defaults(struct Chunks *c)
{
    memset(c, 0, sizeof(*c));
    strcpy((char *)c->txt.pt_Driver, "PostScript");
    c->txt.pt_Port = PP_PARALLEL;
    c->txt.pt_PaperType = AROS_WORD2BE(PT_SINGLE);
    c->txt.pt_PaperSize = AROS_WORD2BE(PS_US_LETTER);
    c->txt.pt_PaperLength = AROS_WORD2BE(65);
    c->txt.pt_Pitch = AROS_WORD2BE(PP_PICA);
    c->txt.pt_Spacing = AROS_WORD2BE(PS_SIX_LPI);
    c->txt.pt_LeftMargin = AROS_WORD2BE(5);
    c->txt.pt_RightMargin = AROS_WORD2BE(75);
    c->txt.pt_Quality = AROS_WORD2BE(PQ_LETTER);
    strcpy((char *)c->devunit.pd_UnitName, "Unit 0");
    c->gfx.pg_Aspect = AROS_WORD2BE(PA_VERTICAL);
    c->gfx.pg_Shade = AROS_WORD2BE(PS_COLOR);
    c->gfx.pg_Image = AROS_WORD2BE(PI_POSITIVE);
    c->gfx.pg_Threshold = AROS_WORD2BE(256);
    c->gfx.pg_Dimensions = PD_IGNORE;
    c->gfx.pg_GraphicFlags = AROS_WORD2BE(PGFB_CENTER_IMAGE);
    c->gfx.pg_PrintDensity = PD_FLOYD;
}

static BOOL load(const char *path, struct Chunks *c)
{
    static LONG stops[] = { ID_PREF, ID_PTXT, ID_PREF, ID_PUNT, ID_PREF, ID_PDEV, ID_PREF, ID_PGFX };
    struct IFFHandle *iff;
    BOOL txt = FALSE;
    BPTR fh;

    if (!(fh = Open(path, MODE_OLDFILE)))
        return FALSE;
    if ((iff = AllocIFF())) {
        iff->iff_Stream = (IPTR)fh;
        InitIFFasDOS(iff);
        if (!OpenIFF(iff, IFFF_READ)) {
            if (!StopChunks(iff, stops, 4)) {
                while (ParseIFF(iff, IFFPARSE_SCAN) == 0) {
                    struct ContextNode *cn = CurrentChunk(iff);
                    APTR to = NULL;
                    LONG size = 0;

                    if (cn->cn_ID == ID_PTXT) { to = &c->txt; size = sizeof(c->txt); }
                    else if (cn->cn_ID == ID_PUNT) { to = &c->unit; size = sizeof(c->unit); }
                    else if (cn->cn_ID == ID_PDEV) { to = &c->devunit; size = sizeof(c->devunit); }
                    else if (cn->cn_ID == ID_PGFX) { to = &c->gfx; size = sizeof(c->gfx); }
                    if (to && cn->cn_Size == size && ReadChunkBytes(iff, to, size) == size && to == &c->txt)
                        txt = TRUE;
                }
            }
            CloseIFF(iff);
        }
        FreeIFF(iff);
    }
    Close(fh);
    return txt;     /* the editor counts a file without PTXT as none */
}

static BOOL chunk(struct IFFHandle *iff, LONG id, APTR data, LONG size)
{
    return !PushChunk(iff, ID_PREF, id, size) && WriteChunkBytes(iff, data, size) == size && !PopChunk(iff);
}

static BOOL store(const char *path, struct Chunks *c)
{
    struct FilePrefHeader head = { PHV_CURRENT, 0, { 0, 0, 0, 0 } };
    struct IFFHandle *iff;
    BOOL ok = FALSE;
    BPTR fh;

    if (!(fh = Open(path, MODE_NEWFILE)))
        return FALSE;
    if ((iff = AllocIFF())) {
        iff->iff_Stream = (IPTR)fh;
        InitIFFasDOS(iff);
        if (!OpenIFF(iff, IFFF_WRITE)) {
            if (!PushChunk(iff, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN)) {
                ok = chunk(iff, ID_PRHD, &head, sizeof(head))
                    && chunk(iff, ID_PTXT, &c->txt, sizeof(c->txt))
                    && chunk(iff, ID_PUNT, &c->unit, sizeof(c->unit))
                    && chunk(iff, ID_PDEV, &c->devunit, sizeof(c->devunit))
                    && chunk(iff, ID_PGFX, &c->gfx, sizeof(c->gfx));
                PopChunk(iff);
            }
            CloseIFF(iff);
        }
        FreeIFF(iff);
    }
    Close(fh);
    return ok;
}

LONG Dest_Set(const char *device, LONG unit, struct TagItem *tags)
{
    struct MsgPort *port;
    struct IOStdReq *io = NULL;
    LONG err = IOERR_OPENFAIL, doserr = 0;

    if ((port = CreateMsgPort()) && (io = (struct IOStdReq *)CreateIORequest(port, sizeof(*io)))
        && OpenDevice(device, unit, (struct IORequest *)io, PRDOF_SETTINGS) == 0) {
        io->io_Command = PRD_SETDESTINATION;
        io->io_Data = tags;
        err = DoIO((struct IORequest *)io);
        doserr = IoErr();
        CloseDevice((struct IORequest *)io);
    }
    if (io)
        DeleteIORequest((struct IORequest *)io);
    if (port)
        DeleteMsgPort(port);
    /* the device ran in this process: a failed file write left its reason here */
    SetIoErr(doserr);
    return err;
}

BOOL Prefs_UseIPP(LONG ippunit, BOOL save, char *driver, LONG drivermax)
{
    struct Chunks c;
    BPTR lock;

    defaults(&c);
    if (!load("ENV:SYS/printer.prefs", &c)) {
        defaults(&c);
        if (!load("ENVARC:SYS/printer.prefs", &c))
            defaults(&c);
    }
    memset(c.unit.pu_DeviceName, 0, sizeof(c.unit.pu_DeviceName));
    strcpy((char *)c.unit.pu_DeviceName, "printspool");
    c.unit.pu_UnitNum = AROS_LONG2BE(ippunit);
    c.unit.pu_OpenDeviceFlags = 0;
    strncpy(driver, (const char *)c.txt.pt_Driver, drivermax - 1);
    driver[drivermax - 1] = 0;

    {
        struct TagItem spool[] = {
            { PDA_Device, (IPTR)"ipp.device" },
            { PDA_Unit,   ippunit },
            { PDA_Save,   save },
            { TAG_DONE,   0 }
        };

        LONG err = Dest_Set("printspool.device", ippunit, spool);

        if (err != 0) {
            if (IoErr() == 0)
                SetIoErr(ERROR_OBJECT_NOT_FOUND);
            return FALSE;
        }
    }
    if ((lock = Lock("ENV:SYS", SHARED_LOCK)) || (lock = CreateDir("ENV:SYS")))
        UnLock(lock);
    if (!store("ENV:SYS/printer.prefs", &c))
        return FALSE;
    if (!save)
        return TRUE;
    if ((lock = Lock("ENVARC:SYS", SHARED_LOCK)) || (lock = CreateDir("ENVARC:SYS")))
        UnLock(lock);
    return store("ENVARC:SYS/printer.prefs", &c);
}
