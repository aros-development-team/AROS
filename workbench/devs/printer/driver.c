/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 *
 * This file handles the creation of the per-unit driver task,
 * and the creation of the per-unit 'struct PrinterData',
 * and the pd_PWrite()/pd_PRead() etc IO functions.
 */

#include <aros/debug.h>
#include <aros/printertag.h>

#include <exec/errors.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include <devices/printer.h>
#include <datatypes/datatypesclass.h>

#define CMD_OPENDEVICE       (0x100)
#define CMD_CLOSEDEVICE      (0x101)

#include LC_LIBDEFS_FILE

#include "printer_intern.h"

/* pd_Flags values */
#define PDF_IOREQ       (1 << 0)        /* IORequest 0 or 1 */
#define PDF_NOIO        (1 << 1)        /* PRTA_NoIO was true */
#define PDF_CONVERT     (1 << 2)        /* PRTA_ConvertSource was true */
#define PDF_8BITGUNS    (1 << 3)        /* PRTA_8BitGuns was true */

const TEXT driverID[] = "printer.driver";

const struct EasyStruct driverMisuse = { \
    .es_StructSize = sizeof(struct EasyStruct),
    .es_Flags = 0,
    .es_Title = "Improper use of printer.device",
    .es_TextFormat = "\"%s\" attempted to use the private\n"
                     "printer.driver method %s.\n"
                     "Only CMD_RAWWRITE is supported.\n",
    .es_GadgetFormat = "Ok",
};

#define TASK_PRINTERDATA(pd)   \
    struct PrinterData *pd =(struct PrinterData *)FindTask(NULL)->tc_UserData; \
    if (pd == NULL ||                                                   \
        pd->pd_Device.dd_Device.lib_Node.ln_Type != NT_DEVICE ||       \
        pd->pd_Device.dd_Device.lib_IdString != driverID) {            \
        struct Library *IntuitionBase;                                  \
        if ((IntuitionBase = TaggedOpenLibrary(TAGGEDOPEN_INTUITION))) { \
            IPTR args[] = { (IPTR)FindTask(NULL)->tc_Node.ln_Name,      \
                            (IPTR)__func__ };                           \
            EasyRequestArgs(NULL, (struct EasyStruct *)&driverMisuse,   \
                            0, args);                                   \
            CloseLibrary(IntuitionBase);                                \
        }                                                               \
        return IOERR_NOCMD; \
    }


static BOOL initMsgPort(struct MsgPort *port)
{
    BYTE sb = AllocSignal(-1);
    
    if (sb < 0)
        return FALSE;

    port->mp_SigBit = sb;
    port->mp_SigTask = FindTask(NULL);
    port->mp_Flags = PA_SIGNAL;
    port->mp_Node.ln_Type = NT_MSGPORT;
    NEWLIST(&port->mp_MsgList);

    return TRUE;
}

STATIC AROS_LH3(LONG, OpenDevice,
                AROS_LHA(struct IORequest *, io, A1),
                AROS_LHA(IPTR, unitNumber, D0),
                AROS_LHA(ULONG, flags, D1),
                struct PrinterUnit *, pu, 1, PrinterUnit)
{
    AROS_LIBFUNC_INIT

    struct MsgPort *reply;

    if ((reply = CreateMsgPort())) {
        struct IORequest ior = *io;
        ior.io_Message.mn_ReplyPort = reply;
        ior.io_Command = CMD_OPENDEVICE;
        return DoIO(&ior);
    }

    return IOERR_OPENFAIL;

    AROS_LIBFUNC_EXIT
}

STATIC AROS_LH1(BPTR, CloseDevice,
                AROS_LCA(struct IORequest *,io, A1),
                struct PrinterUnit *, pu, 2, PrinterUnit)
{
    AROS_LIBFUNC_INIT

    struct MsgPort *reply;

    if ((reply = CreateMsgPort())) {
        struct IORequest ior = *io;
        ior.io_Message.mn_ReplyPort = reply;
        ior.io_Command = CMD_CLOSEDEVICE;
        DoIO(&ior);
        return AROS_LC1(BPTR, Expunge,
                   AROS_LCA(struct PrinterUnit *, pu, D0),
                   struct PrinterUnit *, pu, 3, PrinterUnit);
    }

    return BNULL;

    AROS_LIBFUNC_EXIT
}

STATIC AROS_LH1(BPTR, Expunge,
                AROS_LHA(struct Library *, extralib, D0),
                struct PrinterUnit *, pu, 3, PrinterUnit)
{
    AROS_LIBFUNC_INIT

    struct Library *lib = (struct Library *)pu;

    if (lib->lib_OpenCnt == 0) {
        BPTR seg = pu->pu_PrinterData.pd_PrinterSegment;

        Remove((struct Node *)lib);
        FreeMem((UBYTE *)lib - lib->lib_NegSize,
                (ULONG) (lib->lib_NegSize +
                         lib->lib_PosSize));

        D(bug("%s: Return segment %p\n", __func__, BADDR(seg)));
        return seg;
    }

    lib->lib_Flags |= LIBF_DELEXP;

    return BNULL;

    AROS_LIBFUNC_EXIT
}


STATIC AROS_LH1(void, BeginIO,
 AROS_LHA(union printerIO *, pio, A1),
          struct PrinterUnit *, pu, 5, PrinterUnit)
{
    AROS_LIBFUNC_INIT

    struct IOStdReq *io = &pio->ios; 
    struct PrinterData *pd = &pu->pu_PrinterData;

    D(bug("BeginIO: io_Command = %d, Unit Port %p\n", io->io_Command, &pd->pd_Unit));

    io->io_Flags &= ~IOF_QUICK;
    PutMsg(&pd->pd_Unit, &io->io_Message);

    return;

    AROS_LIBFUNC_EXIT
}

STATIC AROS_LH1(LONG, AbortIO,
 AROS_LHA(struct IORequest *, pio, A1), 
          struct PrinterUnit *, pd, 6, PrinterUnit)
{
    AROS_LIBFUNC_INIT
    return IOERR_NOCMD;
    AROS_LIBFUNC_EXIT
}

/* These wrappers make sure that we don't 
 * make WaitIO() hang or corrupt memory
 * if called on an already completed IO
 */
static inline LONG WaitIOStd(struct IOStdReq *io)
{
    WaitIO((struct IORequest *)io);
    io->io_Message.mn_Node.ln_Type = 0;
    return io->io_Error;
}

static inline LONG DoIOStd(struct IOStdReq *io)
{
    DoIO((struct IORequest *)io);
    io->io_Message.mn_Node.ln_Type = 0;
    return io->io_Error;
}

static LONG pd_PWrite(APTR data, LONG len)
{
    struct IOStdReq *io;
    TASK_PRINTERDATA(pd);

    if (pd->pd_Flags & PDF_NOIO)
        return IOERR_OPENFAIL;

    io = (pd->pd_Flags & PDF_IOREQ) ?
                (struct IOStdReq *)&pd->pd_ior1 :
                (struct IOStdReq *)&pd->pd_ior0;
    WaitIOStd(io);
    /* TODO: Call error hook if there is an error */
    io->io_Command = CMD_WRITE;
    io->io_Flags = 0;
    io->io_Actual = 0;
    io->io_Length = len;
    io->io_Data = data;
    io->io_Offset = 0;
    io->io_Message.mn_Length = sizeof(*io);
    SendIO((struct IORequest *)io);

    pd->pd_Flags ^= PDF_IOREQ;

    return 0;
}

static LONG pd_PBothReady(VOID)
{
    TASK_PRINTERDATA(pd);

    D(bug("%s:\n", __func__));
    if (pd->pd_Flags & PDF_NOIO)
        return IOERR_OPENFAIL;

    WaitIOStd((struct IOStdReq *)&pd->pd_ior0);
    WaitIOStd((struct IOStdReq *)&pd->pd_ior1);

    return 0;
}

static LONG pd_PRead(char * buffer, LONG *length, struct timeval *tv)
{
    ULONG sigs;
    struct IOStdReq *io;
    LONG err;
    TASK_PRINTERDATA(pd);

    if (pd->pd_Flags & PDF_NOIO)
        return IOERR_OPENFAIL;

    D(bug("%s:\n", __func__));
    io = (pd->pd_Flags & PDF_IOREQ) ?
                (struct IOStdReq *)&pd->pd_ior1 :
                (struct IOStdReq *)&pd->pd_ior0;
    WaitIOStd(io);
    /* TODO: Call error hook if there is an error */
    pd->pd_TIOR.tr_node.io_Command = TR_ADDREQUEST;
    pd->pd_TIOR.tr_node.io_Flags = 0;
    pd->pd_TIOR.tr_node.io_Message.mn_Length = sizeof(pd->pd_TIOR);
    pd->pd_TIOR.tr_time = *tv;
    SendIO((struct IORequest *)&pd->pd_TIOR);

    io->io_Command = CMD_READ;
    io->io_Flags = 0;
    io->io_Actual = 0;
    io->io_Length = *length;
    io->io_Data = buffer;
    io->io_Offset = 0;
    io->io_Message.mn_Length = sizeof(*io);
    SendIO((struct IORequest *)io);
    sigs = Wait((1 << io->io_Message.mn_ReplyPort->mp_SigBit) |
                (1 << pd->pd_IORPort.mp_SigBit));
    if (sigs & (1 << pd->pd_IORPort.mp_SigBit)) {
        WaitIO((struct IORequest *)&pd->pd_TIOR);
        if (!CheckIO((struct IORequest *)io))
            AbortIO((struct IORequest *)io);
    }
    WaitIOStd(io);
    err = io->io_Error;
    if (err == 0)
        *length = io->io_Actual;

    /* No need to swap units, as this one has been completed */

    return err;
}

static LONG pd_CallErrHook(struct Hook *hook, union printerIO *ior, struct PrtErrMsg *pem)
{
    /* TODO */
    return 0;
}

/* Only designed to work on the serial port. */
static LONG pd_PQuery(LONG *numofchars)
{
    LONG err;
    struct IOStdReq *io;

    TASK_PRINTERDATA(pd);

    if (pd->pd_Flags & PDF_NOIO)
        return IOERR_OPENFAIL;

    D(bug("%s:\n", __func__));
    io = (pd->pd_Flags & PDF_IOREQ) ?
                (struct IOStdReq *)&pd->pd_ior1 :
                (struct IOStdReq *)&pd->pd_ior0;
    WaitIOStd(io);
    /* TODO: Call error hook if there is an error */
    io->io_Command = SDCMD_QUERY;
    io->io_Flags = 0;
    io->io_Actual = 0;
    io->io_Length = 0;
    io->io_Data = NULL;
    io->io_Offset = 0;
    io->io_Message.mn_Length = sizeof(*io);
    err = DoIOStd(io);
    if (err == 0)
        *numofchars = io->io_Actual;
    else
        *numofchars = 0;

    /* No need to swap units, as this one has been completed */

    return err;
}

/* Only designed to work on the serial and parallel port. */
static LONG pd_Query(struct IOStdReq *sio)
{
    LONG err;
    struct IOStdReq *io;

    TASK_PRINTERDATA(pd);

    D(bug("%s:\n", __func__));
    if (pd->pd_PUnit->pu_Prefs.pp_Unit.pu_DeviceName[0] != 0 ||
        (pd->pd_Flags & PDF_NOIO)) {
        sio->io_Actual = 0;
        return IOERR_OPENFAIL;
    }

    io = (pd->pd_Flags & PDF_IOREQ) ?
                (struct IOStdReq *)&pd->pd_ior1 :
                (struct IOStdReq *)&pd->pd_ior0;
    WaitIOStd(io);
    /* TODO: Call error hook if there is an error */
    io->io_Command = SDCMD_QUERY;
    io->io_Flags = 0;
    io->io_Actual = 0;
    io->io_Length = 0;
    io->io_Data = NULL;
    io->io_Offset = 0;
    io->io_Message.mn_Length = sizeof(*io);
    err = DoIOStd(io);
    if (err == 0) {
        UBYTE *data = sio->io_Data;
        if (data) {
            UWORD status;
            
            switch (pd->pd_PUnit->pu_Prefs.pp_Txt.pt_Port) {
            case PP_SERIAL:
                status = ((struct IOExtSer *)io)->io_Status;
                break;
            case PP_PARALLEL:
                status = ((struct IOExtPar *)io)->io_Status;
                break;
            default:
                status = 0;
                break;
            }
            data[0] = (status >> 0) & 0xff;
            data[1] = (status >> 8) & 0xff;
        }
        sio->io_Actual = pd->pd_PUnit->pu_Prefs.pp_Txt.pt_Port + 1;
    }

    /* No need to swap units, as this one has been completed */

    return err;
}

static LONG pd_Init(struct PrinterData *pd)
{
    struct PrinterExtendedData *ped = &pd->pd_SegmentData->ps_PED;
    TEXT devname[sizeof(pd->pd_Preferences.PrtDevName) + 7 + 1];

    /* Initialize the unit */
    strcpy(devname, pd->pd_Preferences.PrtDevName);
    strcat(devname, ".device");

    D(bug("%s: create msgport %p\n", __func__, &pd->pd_Unit));
    if (initMsgPort(&pd->pd_Unit)) {
        D(bug("%s: Call ped_Init => %p\n", __func__, pd->pd_SegmentData->ps_PED.ped_Init));
        if (0 == pd->pd_SegmentData->ps_PED.ped_Init(pd)) {
            if (pd->pd_Flags & PDF_NOIO)
                return 0;

            if ((pd->pd_ior0.pd_p0.IOPar.io_Message.mn_ReplyPort=CreateMsgPort())) {
                pd->pd_ior0.pd_p0.IOPar.io_Message.mn_Length = sizeof(pd->pd_ior0);
                if (0 == OpenDevice(devname,
                                    pd->pd_Preferences.DefaultPrtUnit,
                                    (struct IORequest *)&pd->pd_ior0, 0)) {
                    D(bug("%s: open %s %d for io 0\n", __func__, devname, pd->pd_Preferences.DefaultPrtUnit));
                    pd->pd_ior0.pd_p0.IOPar.io_Message.mn_Node.ln_Type = 0;
                    if ((pd->pd_ior1.pd_p1.IOPar.io_Message.mn_ReplyPort=CreateMsgPort())) {
                        pd->pd_ior1.pd_p1.IOPar.io_Message.mn_Length = sizeof(pd->pd_ior1);
                        if (0 == OpenDevice(devname,
                                            pd->pd_Preferences.DefaultPrtUnit,
                                            (struct IORequest *)&pd->pd_ior1, 0)) {
                            pd->pd_ior1.pd_p1.IOPar.io_Message.mn_Node.ln_Type = 0;
                            D(bug("%s: open %s %d for io 1\n", __func__, devname, pd->pd_Preferences.DefaultPrtUnit));
                            if (initMsgPort(&pd->pd_IORPort)) {
                                pd->pd_TIOR.tr_node.io_Message.mn_ReplyPort=&pd->pd_IORPort;
                                pd->pd_TIOR.tr_node.io_Message.mn_Length = sizeof(pd->pd_TIOR);
                                if (0 == OpenDevice("timer.device", UNIT_VBLANK,
                                            (struct IORequest *)&pd->pd_TIOR, 0)) {
                                    D(bug("%s: open timer.device %d\n", __func__, UNIT_VBLANK));
                                    if (ped->ped_Render) {
                                        LONG err = ped->ped_Render(0, 0, 0, PRS_PREINIT);
                                        if (err == 0)
                                            return 0;
                                    } else {
                                        return 0;
                                    }
                                }
                                FreeSignal(pd->pd_IORPort.mp_SigBit);
                            }
                            CloseDevice((struct IORequest *)&pd->pd_ior1);
                        }
                        DeleteMsgPort(pd->pd_ior1.pd_p1.IOPar.io_Message.mn_ReplyPort);
                    }
                    CloseDevice((struct IORequest *)&pd->pd_ior0);
                }
                DeleteMsgPort(pd->pd_ior0.pd_p0.IOPar.io_Message.mn_ReplyPort);
            }
            pd->pd_SegmentData->ps_PED.ped_Expunge();
        }
        FreeSignal(pd->pd_Unit.mp_SigBit);
    }

    return -1;
}

static VOID pd_Close(struct PrinterData *pd, union printerIO *pio)
{
    struct PrinterBase *PrinterBase = pd->pd_PUnit->pu_PrinterBase;
    struct PrinterExtendedData *ped = &pd->pd_SegmentData->ps_PED;
    LONG unitnum = pd->pd_PUnit->pu_Prefs.pp_DeviceUnit.pd_UnitNum;

    ped->ped_Close(pio);

    if (!(pd->pd_Flags & PDF_NOIO)) {
        CloseDevice((struct IORequest *)&pd->pd_TIOR);
        FreeSignal(pd->pd_IORPort.mp_SigBit);
        CloseDevice((struct IORequest *)&pd->pd_ior1);
        DeleteMsgPort(pd->pd_ior1.pd_p1.IOPar.io_Message.mn_ReplyPort);
        CloseDevice((struct IORequest *)&pd->pd_ior0);
        DeleteMsgPort(pd->pd_ior0.pd_p0.IOPar.io_Message.mn_ReplyPort);
    }
    FreeSignal(pd->pd_Unit.mp_SigBit);

    ped->ped_Expunge();

    /* Remove from the parent printer.device */
    ObtainSemaphore(&PrinterBase->pb_UnitLock[unitnum]);
    PrinterBase->pb_Unit[unitnum] = NULL;
    Forbid();
    PrinterBase->pb_Device.dd_Library.lib_OpenCnt--;
    Permit();
    ReleaseSemaphore(&PrinterBase->pb_UnitLock[unitnum]);
}

static LONG pd_DoPreferences(const union printerIO *pio, LONG command)
{
    LONG err;

    TASK_PRINTERDATA(pd);

    if (pd->pd_SegmentData->ps_Version >= 44 &&
        (pd->pd_SegmentData->ps_PED.ped_PrinterClass & PPCF_EXTENDED) &&
        pd->pd_SegmentData->ps_PED.ped_DoPreferences != NULL) {
        err = pd->pd_SegmentData->ps_PED.ped_DoPreferences((union printerIO *)pio, command);
    } else {
        switch (command) {
        case PRD_RESETPREFS:
        case PRD_LOADPREFS:
        case PRD_USEPREFS:
        case PRD_SAVEPREFS:
        case PRD_READPREFS:
        case PRD_WRITEPREFS:
        case PRD_EDITPREFS:
        default:
            err = IOERR_NOCMD;
            break;
        }
    }

    return err;
}

/* A driver task is created on an OpenDevice() call,
 * and is killed by a CloseDevice() call.
 */
static LONG pd_DriverTask(VOID)
{
    TASK_PRINTERDATA(pd);

    struct Process *me = (struct Process *)FindTask(NULL);
    struct PrinterExtendedData *ped = &pd->pd_SegmentData->ps_PED;
    struct MagicMessage *msg = NULL;
    union printerIO *pio;
    UWORD cmd;
    BOOL stopped = FALSE;
    LONG ret;

    /* Wait for startup message -
     * we use the DOS port because the 
     * pd_Unit has not been created yet
     */
    D(bug("%s: Waiting for startup. pd=%p\n", __func__, pd));
    WaitPort(&me->pr_MsgPort);
    msg = (struct MagicMessage *)GetMsg(&me->pr_MsgPort);

    D(bug("%s: Initializing driver, Unit Port %p\n", __func__, &pd->pd_Unit));
    ret = pd_Init(pd);

    D(bug("%s: Replying with %d\n", __func__, ret));
    msg->mn_Version = ret;
    ReplyMsg((struct Message *)msg);

    if (0 != ret)
        return ret;

    /* Wait for unit messages on the pd_Unit */
    do {
        LONG err = 0;

        D(bug("%s: Waiting for command on port %p\n", __func__, &pd->pd_Unit));
        WaitPort(&pd->pd_Unit);
        pio = (union printerIO *)GetMsg(&pd->pd_Unit);
        cmd = pio->ios.io_Command;

        D(bug("%s: Command = %d\n", __func__, cmd));
        switch (cmd) {
        case CMD_OPENDEVICE:
            err = ped->ped_Open(pio);
            if (err == 0)
                Printer_Text_Write(pd, "\033#1", 3); /* aRIN */
            break;
        case CMD_CLOSEDEVICE:
            pd_Close(pd, pio);
            break;
        case CMD_FLUSH:
            AbortIO((struct IORequest *)&pd->pd_ior0);
            WaitIOStd((struct IOStdReq *)&pd->pd_ior0);
            AbortIO((struct IORequest *)&pd->pd_ior1);
            WaitIOStd((struct IOStdReq *)&pd->pd_ior1);
            break;
        case CMD_RESET:
            if (stopped)
                err = PDERR_CANCEL;
            else {
                err = Printer_Text_Command(pd, aRIN, 0, 0, 0, 0);
            }
            break;
        case CMD_STOP:
            stopped = TRUE;
            break;
        case CMD_START:
            stopped = FALSE;
            break;
        case CMD_WRITE:
            if (stopped)
                err = PDERR_CANCEL;
            else {
                err = Printer_Text_Write(pd, pio->ios.io_Data, pio->ios.io_Length);
                if (err == 0)
                    pio->ios.io_Actual = pio->ios.io_Length;
            }
            break;
        case PRD_RAWWRITE:
            if (stopped)
                err = PDERR_CANCEL;
            else {
                err = pd_PWrite(pio->ios.io_Data, pio->ios.io_Length);
                if (err == 0)
                    pio->ios.io_Actual = pio->ios.io_Length;
            }
            break;
        case PRD_RESETPREFS:
        case PRD_LOADPREFS:
        case PRD_USEPREFS:
        case PRD_SAVEPREFS:
        case PRD_READPREFS:
        case PRD_EDITPREFS:
            err = pd_DoPreferences(pio, pio->ios.io_Command);
            break;
        case PRD_SETERRHOOK:
            pd->pd_PUnit->pu_ErrHook = ((struct IOPrtErrReq *)pio)->io_Hook;
            break;
        case PRD_DUMPRPORTTAGS:
            if (stopped)
                err = PDERR_CANCEL;
            else
                err = Printer_Gfx_DumpRPort((struct IODRPReq *)pio, ((struct IODRPTagsReq *)pio)->io_TagList);
            break;
        case PRD_DUMPRPORT:
            if (stopped)
                err = PDERR_CANCEL;
            else
                err = Printer_Gfx_DumpRPort((struct IODRPReq *)pio, NULL);
            break;
        case PRD_QUERY:
            err = pd_Query(&pio->ios);
            break;
        default:
            err = IOERR_NOCMD;
            break;
        }
        pio->ios.io_Error = err;
        D(bug("%s: Command = %d, Result = %d\n", __func__, cmd, err));

        ReplyMsg((struct Message *)pio);
    } while (cmd != CMD_CLOSEDEVICE);

    D(bug("%s: Shutting down\n", __func__));

    return 0;
}

/* Synchronize old-style prefs with new style prefs
 */
static void pd_SyncPrefs(struct PrinterData *pd)
{
    struct Preferences *dprefs = &pd->pd_Preferences;
    struct PrinterPrefs *uprefs = &pd->pd_PUnit->pu_Prefs;

    dprefs->PrinterType = pd->pd_PrinterType;

    strncpy(dprefs->PrinterFilename, uprefs->pp_Txt.pt_Driver,
            sizeof(dprefs->PrinterFilename));
    dprefs->PrinterFilename[sizeof(dprefs->PrinterFilename)-1] = 0;

    dprefs->PrintPitch = uprefs->pp_Txt.pt_Pitch;
    dprefs->PrintQuality = uprefs->pp_Txt.pt_Quality;
    dprefs->PrintSpacing = uprefs->pp_Txt.pt_Spacing;
    dprefs->PrintLeftMargin = uprefs->pp_Txt.pt_LeftMargin;
    dprefs->PrintRightMargin = uprefs->pp_Txt.pt_RightMargin;
    dprefs->PrintImage = uprefs->pp_Gfx.pg_Image;
    dprefs->PrintAspect = uprefs->pp_Gfx.pg_Aspect;
    dprefs->PrintShade = uprefs->pp_Gfx.pg_Shade;
    dprefs->PrintThreshold = uprefs->pp_Gfx.pg_Threshold;
    dprefs->PaperSize = uprefs->pp_Txt.pt_PaperSize;
    dprefs->PaperType = uprefs->pp_Txt.pt_PaperType;
    dprefs->PaperLength = uprefs->pp_Txt.pt_PaperLength;

    if (uprefs->pp_Unit.pu_DeviceName[0] == 0) {
        if (uprefs->pp_Txt.pt_Port == PP_PARALLEL) {
            strcpy(dprefs->PrtDevName, "parallel");
        } else if (uprefs->pp_Txt.pt_Port == PP_SERIAL) {
            strcpy(dprefs->PrtDevName, "serial");
        } else {
            strcpy(dprefs->PrtDevName, "printtofile");
        }
    } else {
        strncpy(dprefs->PrtDevName, uprefs->pp_Unit.pu_DeviceName, sizeof(dprefs->PrtDevName));
        dprefs->PrtDevName[sizeof(dprefs->PrtDevName)-1]=0;
    }

    dprefs->DefaultPrtUnit = uprefs->pp_Unit.pu_UnitNum;
    dprefs->PrintFlags = uprefs->pp_Gfx.pg_GraphicFlags;
    dprefs->PrintMaxWidth = uprefs->pp_Gfx.pg_PrintMaxWidth;
    dprefs->PrintMaxHeight = uprefs->pp_Gfx.pg_PrintMaxHeight;
    dprefs->PrintDensity = uprefs->pp_Gfx.pg_PrintDensity;
    dprefs->PrintXOffset = uprefs->pp_Gfx.pg_PrintXOffset;
}

/* Create a PrinterData plugin
 */
struct PrinterUnit *Printer_Unit(struct PrinterBase *PrinterBase, LONG unitnum)
{
    struct PrinterUnit *pu;
    struct PrinterPrefs prefs;
    BPTR olddir, dir, driverseg;

    if (!Printer_LoadPrefs(PrinterBase, unitnum, &prefs) || prefs.pp_Txt.pt_Driver[0] == 0) {
        D(bug("%s: No valid prefs for printer.device %d\n", __func__, unitnum));
        return NULL;
    }

    if ((dir = Lock("DEVS:Printers", SHARED_LOCK)) != BNULL) {
        olddir = CurrentDir(dir);
        driverseg = LoadSeg(prefs.pp_Txt.pt_Driver);
        CurrentDir(olddir);
        UnLock(dir);

        D(bug("%s: %s => %p\n", __func__, prefs.pp_Txt.pt_Driver, BADDR(driverseg)));

        if (driverseg) {
            struct PrinterSegment *prtseg = BADDR(driverseg);

            D(bug("%s: magic 0x%08x, expect 0x%08x\n", __func__, prtseg->ps_runAlert, AROS_PRINTER_MAGIC));
            if (prtseg->ps_runAlert == AROS_PRINTER_MAGIC) {
                APTR funcs[] = {
                    AROS_SLIB_ENTRY(OpenDevice,PrinterUnit,1),
                    AROS_SLIB_ENTRY(CloseDevice,PrinterUnit,2),
                    AROS_SLIB_ENTRY(Expunge,PrinterUnit,3),
                    NULL,
                    AROS_SLIB_ENTRY(BeginIO,PrinterUnit,5),
                    AROS_SLIB_ENTRY(AbortIO,PrinterUnit,6),
                    (APTR)-1,
                };

                if ((pu = (struct PrinterUnit *)MakeLibrary(funcs, NULL, NULL, sizeof(*pu), driverseg))) {
                    struct Process *proc;
                    struct PrinterData *pd = &pu->pu_PrinterData;
                    struct Device *dev = (struct Device *)pu;

                    /* Loop back to self */
                    pu->pu_PrinterBase = PrinterBase;
                    pd->pd_PUnit = pu;

                    /* Duplicate the prefs */
                    CopyMem(&prefs, &pu->pu_Prefs, sizeof(prefs));

                    /* Update pd->pd_Preferences from pu->pu_Prefs */
                    pd_SyncPrefs(pd);

                    dev->dd_Library.lib_Node.ln_Name = pu->pu_Prefs.pp_DeviceUnit.pd_UnitName;
                    dev->dd_Library.lib_Version  = prtseg->ps_Version;
                    dev->dd_Library.lib_Revision = prtseg->ps_Revision;
                    /* Magic token for TASK_PRINTERDATA() macro */
                    dev->dd_Library.lib_IdString = (APTR)driverID;

                    pd->pd_Device.dd_Segment = BADDR(driverseg);
                    pd->pd_Device.dd_ExecBase = SysBase;
                    pd->pd_Device.dd_CmdVectors = prtseg->ps_PED.ped_Commands;
                    pd->pd_Device.dd_CmdBytes = NULL;
                    pd->pd_Device.dd_NumCommands = aRAW + 1;
                    pd->pd_PrinterSegment = driverseg;
                    pd->pd_PrinterType = 0;
                    pd->pd_SegmentData = prtseg;
                    pd->pd_PWrite = pd_PWrite;
                    pd->pd_PBothReady = pd_PBothReady;
                    pd->pd_PRead  = pd_PRead;
                    pd->pd_CallErrHook = (APTR)pd_CallErrHook;
                    pd->pd_PQuery = pd_PQuery;
                    pd->pd_UnitNumber = unitnum;
                    pd->pd_DriverName = &pd->pd_Preferences.PrinterFilename[0];

                    /* Make RemDevice() and friends happy */
                    AddDevice(dev);

                    proc = CreateNewProcTags(NP_Entry, pd_DriverTask,
                                             NP_Name, prefs.pp_DeviceUnit.pd_UnitName,
                                             NP_Priority, 0,
                                             NP_Arguments, NULL,
                                             NP_UserData, pd);

                    D(bug("%s: Driver process %p\n", __func__, proc));
                    if (proc != NULL) {
                        struct MsgPort *port;

                        /* Store the process here... */
                        pu->pu_Process = proc;


                        if ((port = CreateMsgPort())) {
                            struct MagicMessage startup, *reply;

                            D(bug("%s: Driver unit port %p\n", __func__, port));
                            startup.mn_ReplyPort=port;
                            startup.mn_Length = sizeof(startup);
                            startup.mn_Magic = AROS_MAKE_ID('p','r','u','n');
                            startup.mn_Version = 0;
                            PutMsg(&proc->pr_MsgPort, (struct Message *)&startup);
                            WaitPort(port);
                            D(bug("%s: Driver replied\n", __func__));
                            reply = (struct MagicMessage *)GetMsg(port);
                            D(bug("%s: Driver reply = %p\n", __func__, reply));
                            DeleteMsgPort(port);
                            D(bug("%s: Driver port %p gone\n", __func__, port));
                            if (reply == &startup &&
                                reply->mn_Length == sizeof(*reply) &&
                                reply->mn_Magic == AROS_MAKE_ID('p','r','u','n') &&
                                reply->mn_Version == 0) {
                                /* Success! */
                                D(bug("%s: Driver started\n", __func__));
                                return pu;
                            }
                            D(bug("%s: Driver startup failed\n", __func__));
                        }
                        /* pd_DriverTask will kill itself on failure */
                    }
                    /* pd_Expunge() calls UnLoadSeg() automatically */
                    RemDevice((struct Device *)pd);
                    driverseg = BNULL;
                }
            }
            if (driverseg)
                UnLoadSeg(driverseg);
        }
    }

    return NULL;
}
