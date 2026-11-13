/*
 * Copyright (C) 2012-2026, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef PRINTER_INTERN_H
#define PRINTER_INTERN_H

#include <exec/devices.h>
#include <libraries/asl.h>
#include <devices/prtbase.h>
#include <devices/printer.h>

#include <prefs/printertxt.h>
#include <prefs/printergfx.h>
#include <dos/dos.h>

#include <turboprint.h>

/*********************************************************************************************/

struct PrinterBase;

struct PrinterPrefs {
    struct PrinterTxtPrefs pp_Txt;
    struct PrinterUnitPrefs pp_Unit;
    struct PrinterDeviceUnitPrefs pp_DeviceUnit;
    struct PrinterGfxPrefs pp_Gfx;
};

struct PrinterUnit {
    struct PrinterData  pu_PrinterData;
    struct PrinterBase *pu_PrinterBase;
    struct PrinterPrefs pu_Prefs;
    TEXT                pu_OpenName[DEVICENAMESIZE + 8];
    struct Process     *pu_Process;
    struct Hook        *pu_ErrHook;

    struct PrinterUnitText {
        BYTE pt_CurrentLine;
        BYTE pt_CRLF;
        BYTE pt_Spacing;
    } pu_Text;

    /* What is being printed, passed on to the output device (PRD_SETJOBINFO)
       when it lists the command; one job per unit, the last opener's */
    BOOL                pu_JobInfoSupported;
    TEXT                pu_JobApplication[64];
    TEXT                pu_JobDocument[128];

    /* A unit is shared: the job runs from its first open to its last close.
       pu_Opens is counted under the unit's pb_UnitLock. */
    ULONG               pu_Opens;
    BOOL                pu_DriverOpen;  /* ped_Open succeeded: ped_Close is owed */

    /* The first error the output device gave since the current command began */
    LONG                pu_IOError;

    /* CMD_STOP: requests that print wait here, in order, for CMD_START */
    BOOL                pu_Stopped;
    struct MinList      pu_Held;

    union printerIO    *pu_Current;     /* the request the unit task is working on */
};

/* Set by AbortIO() in a request that is queued, held or being worked on */
#define IOB_ABORTED     1
#define IOF_ABORTED     (1 << IOB_ABORTED)

/*********************************************************************************************/

#define PRINTER_UNITS       10      /* Same as the max # of printers */

struct PrinterBase {
    struct Device pb_Device;

    struct PrinterUnit *pb_Unit[PRINTER_UNITS];
    struct SignalSemaphore pb_UnitLock[PRINTER_UNITS];

    struct Library *pb_DOSBase;
};

#define DOSBase PrinterBase->pb_DOSBase

/*********************************************************************************************/

BOOL Printer_LoadPrefs(struct PrinterBase *PrinterBase, LONG unit, struct PrinterPrefs *prefs);

struct PrinterUnit *Printer_Unit(struct PrinterBase *PrinterBase, LONG unit);

LONG Printer_Gfx_DumpRPort(struct IODRPReq *pio, struct TagItem *tags);

LONG Printer_Text_Write(struct PrinterData *pd, UBYTE *text, LONG length);
LONG Printer_Text_Command(struct PrinterData *pd, UWORD command, UBYTE p0, UBYTE p1, UBYTE p2, UBYTE p3);

/* AbortIO() was called on the request being worked on */
BOOL Printer_Aborted(struct PrinterData *pd);

#endif /* PRINTER_INTERN_H */
