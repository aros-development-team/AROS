/*
 * Copyright (C) 2012-2026, The AROS Development Team.  All rights reserved.
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
#include <proto/utility.h>
#include <clib/alib_protos.h>

#include <devices/printer.h>
#include <devices/newstyle.h>
#include <datatypes/datatypesclass.h>

#include <string.h>

#define TPMATCHWORD          0xf10a57ef /* From turboprint's SDK */

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

/* Internal, private message
 */
struct PrinterMessage {
        struct Message mm_Message;
        ULONG   mm_Magic;
        ULONG   mm_Version;
};

#define TASK_PRINTERDATA(pd)   \
    struct PrinterData *pd =(struct PrinterData *)FindTask(NULL)->tc_UserData; \
    if (pd == NULL ||                                                   \
        pd->pd_Device.dd_Device.lib_Node.ln_Type != NT_DEVICE ||        \
        pd->pd_Device.dd_Device.lib_IdString != driverID) {             \
        struct Library *IntuitionBase;                                  \
        if ((IntuitionBase = TaggedOpenLibrary(TAGGEDOPEN_INTUITION))) {\
            CONST_STRPTR args[] = { FindTask(NULL)->tc_Node.ln_Name,    \
                                    __func__ };                         \
            EasyRequestArgs(NULL, (struct EasyStruct *)&driverMisuse,   \
                            0, (RAWARG)&args[0]);                       \
            CloseLibrary(IntuitionBase);                                \
        }                                                               \
        return IOERR_NOCMD; \
    }


static BOOL initMsgPort(struct MsgPort *port)
{
    BYTE sb = AllocSignal(-1);
    
    if (sb < 0)
        return FALSE;

    memset( port, 0, sizeof( *port ) );
    port->mp_SigBit = sb;
    port->mp_SigTask = FindTask(NULL);
    port->mp_Flags = PA_SIGNAL;
    port->mp_Node.ln_Type = NT_MSGPORT;
    NEWLIST(&port->mp_MsgList);

    return TRUE;
}

/* "printer.device unit N" for a prefs file without a PDEV chunk */
static void pd_DefaultUnitName(UBYTE *name, LONG unitnum)
{
    static const char prefix[] = "printer.device unit ";
    char digits[12];
    int n = 0, i;

    CopyMem((APTR)prefix, name, sizeof(prefix) - 1);
    i = sizeof(prefix) - 1;
    do {
        digits[n++] = '0' + unitnum % 10;
        unitnum /= 10;
    } while (unitnum && n < (int)sizeof(digits));
    while (n > 0 && i < UNITNAMESIZE - 1)
        name[i++] = digits[--n];
    name[i] = 0;
}

/* Whether the output device lists PRD_SETJOBINFO in its NSCMD_DEVICEQUERY
   reply; a device that does not is never sent it. */
static void pd_QueryJobInfo(struct PrinterData *pd)
{
    struct PrinterUnit *pu = pd->pd_PUnit;
    struct IOStdReq *io = (struct IOStdReq *)&pd->pd_ior0;
    struct NSDeviceQueryResult nsd;
    UWORD *cmd;

    pu->pu_JobInfoSupported = FALSE;
    memset(&nsd, 0, sizeof(nsd));
    io->io_Command = NSCMD_DEVICEQUERY;
    io->io_Flags = 0;
    io->io_Data = &nsd;
    io->io_Length = sizeof(nsd);
    io->io_Message.mn_Length = sizeof(*io);
    if (DoIO((struct IORequest *)io) != 0 || io->io_Actual < sizeof(nsd) || nsd.SupportedCommands == NULL) {
        /* not an error of anything that was printed */
        io->io_Error = 0;
        return;
    }
    for (cmd = nsd.SupportedCommands; *cmd; cmd++)
        if (*cmd == PRD_SETJOBINFO)
            pu->pu_JobInfoSupported = TRUE;
}

/* The opener's program name: its CLI command name, or its task name. */
static void pd_SetJobApplication(struct PrinterUnit *pu)
{
    struct Task *task = FindTask(NULL);
    CONST_STRPTR name = task->tc_Node.ln_Name;
    TEXT command[64];

    if (task->tc_Node.ln_Type == NT_PROCESS && ((struct Process *)task)->pr_CLI != BNULL) {
        struct CommandLineInterface *cli = BADDR(((struct Process *)task)->pr_CLI);
        LONG len = AROS_BSTR_strlen(cli->cli_CommandName);

        if (len > 0) {
            CONST_STRPTR start;
            STRPTR c;

            if (len >= (LONG)sizeof(command))
                len = sizeof(command) - 1;
            CopyMem(AROS_BSTR_ADDR(cli->cli_CommandName), command, len);
            command[len] = 0;
            start = command;
            for (c = command; *c; c++)
                if (*c == ':' || *c == '/')
                    start = c + 1;
            name = start;
        }
    }
    strncpy(pu->pu_JobApplication, name ? (const char *)name : "", sizeof(pu->pu_JobApplication) - 1);
    pu->pu_JobApplication[sizeof(pu->pu_JobApplication) - 1] = 0;
    pu->pu_JobDocument[0] = 0;
}

STATIC AROS_LH3(LONG, OpenDevice,
                AROS_LHA(struct IORequest *, io, A1),
                AROS_LHA(IPTR, unitNumber, D0),
                AROS_LHA(ULONG, flags, D1),
                struct PrinterUnit *, pu, 1, PrinterUnit)
{
    AROS_LIBFUNC_INIT

    struct MsgPort *mp;
    LONG ret = IOERR_OPENFAIL;

    if ((mp = CreateMsgPort())) {
        struct IORequest ior = *io;

        /* the job is its first opener's */
        if (pu->pu_Opens <= 1)
            pd_SetJobApplication(pu);
        ior.io_Message.mn_ReplyPort = mp;
        ior.io_Command = CMD_OPENDEVICE;
        ret = DoIO(&ior);
        DeleteMsgPort(mp);
    }

    /* printer.device's own OpenDevice looks at the caller's request */
    io->io_Error = ret;
    return ret;

    AROS_LIBFUNC_EXIT
}

STATIC AROS_LH1(BPTR, CloseDevice,
                AROS_LCA(struct IORequest *,io, A1),
                struct PrinterUnit *, pu, 2, PrinterUnit)
{
    AROS_LIBFUNC_INIT

    BPTR ret = BNULL;
    struct MsgPort *mp;

    if ((mp = CreateMsgPort())) {
        struct IORequest ior = *io;
        ior.io_Message.mn_ReplyPort = mp;
        ior.io_Command = CMD_CLOSEDEVICE;
        /* The unit task answers IOERR_UNITBUSY while others have the unit
           open; after the last close it has gone and the unit is freed. */
        if (DoIO(&ior) == 0)
            ret = AROS_LC1(BPTR, Expunge,
                       AROS_LCA(struct PrinterUnit *, pu, D0),
                       struct PrinterUnit *, pu, 3, PrinterUnit);
        DeleteMsgPort(mp);
    }

    return ret;

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

    io->io_Flags &= ~(IOF_QUICK | IOF_ABORTED);
    PutMsg(&pd->pd_Unit, &io->io_Message);

    return;

    AROS_LIBFUNC_EXIT
}

/* The request is only marked here: the unit's port and its list of held
 * requests belong to the unit task, which replies a marked request with
 * IOERR_ABORTED when it meets it - at once if it is waiting for CMD_START,
 * at the next row or character if it is the one being printed.
 */
STATIC AROS_LH1(LONG, AbortIO,
 AROS_LHA(struct IORequest *, io, A1),
          struct PrinterUnit *, pu, 6, PrinterUnit)
{
    AROS_LIBFUNC_INIT

    struct PrinterData *pd = &pu->pu_PrinterData;
    LONG ret = IOERR_NOCMD;

    Forbid();
    if (io->io_Message.mn_Node.ln_Type == NT_MESSAGE) {
        io->io_Flags |= IOF_ABORTED;
        Signal(pd->pd_Unit.mp_SigTask, 1UL << pd->pd_Unit.mp_SigBit);
        ret = 0;
    }
    Permit();

    return ret;

    AROS_LIBFUNC_EXIT
}

BOOL Printer_Aborted(struct PrinterData *pd)
{
    struct IORequest *io = (struct IORequest *)pd->pd_PUnit->pu_Current;

    return (io != NULL && (io->io_Flags & IOF_ABORTED)) ? TRUE : FALSE;
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

/* For a command that is not a write of the job: its error is the caller's
 * to look at, and is not left in the request for pd_Collect() to find.
 */
static inline LONG DoIOStd(struct IOStdReq *io)
{
    LONG err;

    DoIO((struct IORequest *)io);
    io->io_Message.mn_Node.ln_Type = 0;
    err = io->io_Error;
    io->io_Error = 0;
    return err;
}

/* Waits for a write to the output device and keeps the first error any of
 * them gave. pd_PWrite() does not wait for the write it starts, so its
 * error shows up here at the next write; the unit task collects both
 * requests before it replies to the command that caused them.
 */
static void pd_Collect(struct PrinterData *pd, struct IOStdReq *io)
{
    LONG err = WaitIOStd(io);

    if (err != 0) {
        if (pd->pd_PUnit->pu_IOError == 0)
            pd->pd_PUnit->pu_IOError = err;
        io->io_Error = 0;
    }
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
    /* A driver has two buffers and fills the other one as soon as we
     * return, so the write before this one has to be through by then:
     * only the buffer just handed over may still be on its way.
     */
    pd_Collect(pd, (struct IOStdReq *)&pd->pd_ior0);
    pd_Collect(pd, (struct IOStdReq *)&pd->pd_ior1);
    /* Nothing more goes out for a command whose output has failed */
    if (pd->pd_PUnit->pu_IOError != 0)
        return pd->pd_PUnit->pu_IOError;
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

    pd_Collect(pd, (struct IOStdReq *)&pd->pd_ior0);
    pd_Collect(pd, (struct IOStdReq *)&pd->pd_ior1);

    return pd->pd_PUnit->pu_IOError;
}

/* Tells the output device what is being printed; devices that do not know
   PRD_SETJOBINFO ignore it. */
static void pd_SendJobInfo(struct PrinterData *pd)
{
    struct PrinterUnit *pu = pd->pd_PUnit;
    struct IOStdReq *io = (struct IOStdReq *)&pd->pd_ior0;
    struct TagItem tags[] = {
        { PJIA_Application, (IPTR)pu->pu_JobApplication },
        { PJIA_Document,    (IPTR)pu->pu_JobDocument },
        { TAG_END, 0 }
    };

    if ((pd->pd_Flags & PDF_NOIO) || !pu->pu_JobInfoSupported)
        return;
    pd_Collect(pd, io);
    io->io_Command = PRD_SETJOBINFO;
    io->io_Flags = 0;
    io->io_Data = tags;
    io->io_Length = 0;
    io->io_Message.mn_Length = sizeof(*io);
    DoIOStd(io);
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
    pd_Collect(pd, io);
    pd->pd_TIOR.tr_node.io_Command = TR_ADDREQUEST;
    pd->pd_TIOR.tr_node.io_Flags = 0;
    pd->pd_TIOR.tr_node.io_Message.mn_Length = sizeof(pd->pd_TIOR);
    pd->pd_TIOR.tr_time.tv_secs = tv->tv_secs;
    pd->pd_TIOR.tr_time.tv_micro = tv->tv_micro;
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
    err = WaitIOStd(io);
    io->io_Error = 0;
    if (err == 0)
        *length = io->io_Actual;

    /* No need to swap units, as this one has been completed */

    return err;
}

/* PD->pd_CallErrHook(): shows an error message through the hook that
 * PRD_SETERRHOOK set - PDHOOK_STD is a requester, anything else is called
 * with the request as its object and the message as its parameter.
 */
static LONG pd_CallErrHook(struct Hook *hook, union printerIO *ior, struct PrtErrMsg *pem)
{
    TASK_PRINTERDATA(pd);

    if (hook == PDHOOK_NONE || pem == NULL)
        return 0;

    if (hook == PDHOOK_STD) {
        struct Library *IntuitionBase;
        LONG ret = 0;

        if (pem->pe_ES == NULL)
            return 0;
        if ((IntuitionBase = TaggedOpenLibrary(TAGGEDOPEN_INTUITION))) {
            ret = EasyRequestArgs(pem->pe_Window, pem->pe_ES, pem->pe_IDCMP,
                                  (RAWARG)pem->pe_ArgList);
            CloseLibrary(IntuitionBase);
        }
        return ret;
    }

    return (LONG)CallHookA(hook, ior, pem);
}

static CONST_STRPTR pd_ErrorText(LONG err)
{
    switch (err) {
    case PDERR_NOTGRAPHICS:     return "The printer cannot print graphics.";
    case PDERR_BADDIMENSION:    return "The print dimensions are not valid.";
    case PDERR_INTERNALMEMORY:  return "Not enough memory to print.";
    case PDERR_BUFFERMEMORY:    return "Not enough memory for the print buffer.";
    case PDERR_BADPREFERENCES:  return "The printer preferences are not valid.";
    case IOERR_NOCMD:           return "The printer does not support this command.";
    case IOERR_BADLENGTH:       return "The request has a bad length.";
    default:                    return "The printer driver reported an error.";
    }
}

/* Decimal, for messages: EasyRequest() gets strings only, so that the
   argument array has one layout on every CPU. */
static void pd_NumberText(STRPTR buf, LONG n)
{
    TEXT digits[12];
    ULONG u = (n < 0) ? -(ULONG)n : (ULONG)n;
    int i = 0;

    if (n < 0)
        *buf++ = '-';
    do {
        digits[i++] = '0' + u % 10;
        u /= 10;
    } while (u);
    while (i > 0)
        *buf++ = digits[--i];
    *buf = 0;
}

/* A command is about to return an error: tell the hook set by
 * PRD_SETERRHOOK. ioerr is the output device's error, if that is what
 * failed. A driver's own error codes are the driver's to explain.
 */
static void pd_ReportError(struct PrinterData *pd, union printerIO *pio, LONG err, LONG ioerr)
{
    static const struct EasyStruct outputError = {
        .es_StructSize = sizeof(struct EasyStruct),
        .es_Title = "Printer error",
        .es_TextFormat = "%s:\ncannot write to %s unit %s (error %s).",
        .es_GadgetFormat = "OK",
    };
    static const struct EasyStruct commandError = {
        .es_StructSize = sizeof(struct EasyStruct),
        .es_Title = "Printer error",
        .es_TextFormat = "%s:\n%s (error %s)",
        .es_GadgetFormat = "OK",
    };
    struct PrinterUnit *pu = pd->pd_PUnit;
    struct PrinterExtendedData *ped = &pd->pd_SegmentData->ps_PED;
    struct Hook *hook = pu->pu_ErrHook;
    CONST_STRPTR name = pd->pd_Device.dd_Device.lib_Node.ln_Name;
    TEXT code[16], unit[16];
    IPTR args[4];
    struct PrtErrMsg pem = {
        .pe_Version = PDHOOK_VERSION,
        .pe_ErrorLevel = RETURN_ERROR,
        .pe_ArgList = args,
    };

    if (hook == PDHOOK_NONE)
        return;

    if (ioerr != 0) {
        pd_NumberText(code, ioerr);
        pd_NumberText(unit, pd->pd_Preferences.DefaultPrtUnit);
        args[0] = (IPTR)name;
        args[1] = (IPTR)pu->pu_OpenName;
        args[2] = (IPTR)unit;
        args[3] = (IPTR)code;
        pem.pe_ES = (struct EasyStruct *)&outputError;
    } else {
        if (err >= PDERR_FIRSTCUSTOM && err <= PDERR_LASTCUSTOM &&
            pd->pd_SegmentData->ps_Version >= 44 &&
            (ped->ped_PrinterClass & PPCF_EXTENDED) &&
            ped->ped_CallErrHook != NULL) {
            ped->ped_CallErrHook(pio, hook);
            return;
        }
        pd_NumberText(code, err);
        args[0] = (IPTR)name;
        args[1] = (IPTR)pd_ErrorText(err);
        args[2] = (IPTR)code;
        pem.pe_ES = (struct EasyStruct *)&commandError;
    }

    pd_CallErrHook(hook, pio, &pem);
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
    pd_Collect(pd, io);
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
    if ((strcmp(pd->pd_PUnit->pu_OpenName, "parallel.device") != 0 &&
         strcmp(pd->pd_PUnit->pu_OpenName, "usbparallel.device") != 0 &&
         strcmp(pd->pd_PUnit->pu_OpenName, "serial.device") != 0) ||
        (pd->pd_Flags & PDF_NOIO)) {
        sio->io_Actual = 0;
        return IOERR_OPENFAIL;
    }

    io = (pd->pd_Flags & PDF_IOREQ) ?
                (struct IOStdReq *)&pd->pd_ior1 :
                (struct IOStdReq *)&pd->pd_ior0;
    pd_Collect(pd, io);
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
    CONST_STRPTR devname = pd->pd_PUnit->pu_OpenName;

    D(bug("%s: create msgport %p\n", __func__, &pd->pd_Unit));
    if (initMsgPort(&pd->pd_Unit)) {
        D(bug("%s: Call ped_Init => %p\n", __func__, pd->pd_SegmentData->ps_PED.ped_Init));
        if (0 == pd->pd_SegmentData->ps_PED.ped_Init(pd)) {
            if (pd->pd_Flags & PDF_NOIO)
                return 0;

            pd->pd_ior0.pd_p0.io_ParFlags = PARF_SHARED;
            if ((pd->pd_ior0.pd_p0.IOPar.io_Message.mn_ReplyPort=CreateMsgPort())) {
                if (0 == OpenDevice(devname,
                                    pd->pd_Preferences.DefaultPrtUnit,
                                    (struct IORequest *)&pd->pd_ior0, 0)) {
                    D(bug("%s: open %s %d for io 0\n", __func__, devname, pd->pd_Preferences.DefaultPrtUnit));
                    pd->pd_ior0.pd_p0.IOPar.io_Message.mn_Node.ln_Type = 0;
                    pd_QueryJobInfo(pd);

                    pd->pd_ior1.pd_p1.io_ParFlags = PARF_SHARED;
                    if ((pd->pd_ior1.pd_p1.IOPar.io_Message.mn_ReplyPort=CreateMsgPort())) {
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
                                        /* the density the preferences ask for, as in a dump
                                           that names none (gfx.c) */
                                        LONG err = ped->ped_Render(0,
                                                ((LONG)pd->pd_Preferences.PrintDensity << 8) & SPECIAL_DENSITYMASK,
                                                0, PRS_PREINIT);
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

/* The last close of the unit. Called with the unit's pb_UnitLock held, so
 * that an open of the same unit waits until the job is finished and the
 * output device is free again, and then gets a new unit.
 */
static VOID pd_Close(struct PrinterData *pd, union printerIO *pio)
{
    struct PrinterBase *PrinterBase = pd->pd_PUnit->pu_PrinterBase;
    struct PrinterExtendedData *ped = &pd->pd_SegmentData->ps_PED;
    LONG unitnum = pd->pd_UnitNumber;

    /* not after a ped_Open that failed */
    if (pd->pd_PUnit->pu_DriverOpen) {
        ped->ped_Close(pio);
        pd->pd_PUnit->pu_DriverOpen = FALSE;
    }

    if (!(pd->pd_Flags & PDF_NOIO)) {
        /* whatever ped_Close wrote */
        WaitIOStd((struct IOStdReq *)&pd->pd_ior0);
        WaitIOStd((struct IOStdReq *)&pd->pd_ior1);
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
    PrinterBase->pb_Unit[unitnum] = NULL;
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

/* Waits until everything written has reached the output device, and
 * returns the first error it gave (0 if none).
 */
static LONG pd_Drain(struct PrinterData *pd)
{
    struct PrinterUnit *pu = pd->pd_PUnit;
    LONG ioerr;

    if (!(pd->pd_Flags & PDF_NOIO)) {
        pd_Collect(pd, (struct IOStdReq *)&pd->pd_ior0);
        pd_Collect(pd, (struct IOStdReq *)&pd->pd_ior1);
    }
    ioerr = pu->pu_IOError;
    pu->pu_IOError = 0;

    return ioerr;
}

static void pd_ReplyAborted(union printerIO *pio)
{
    pio->ios.io_Error = IOERR_ABORTED;
    ReplyMsg((struct Message *)pio);
}

/* The requests that print. After CMD_STOP they wait, in order, for CMD_START. */
static BOOL pd_IsPrintCommand(UWORD cmd)
{
    switch (cmd) {
    case CMD_RESET:
    case CMD_WRITE:
    case PRD_RAWWRITE:
    case PRD_PRTCOMMAND:
    case PRD_DUMPRPORT:
    case PRD_DUMPRPORTTAGS:
    case PRD_TPEXTDUMPRPORT:
        return TRUE;
    }
    return FALSE;
}

/* Prints one request and replies to it. */
static void pd_Print(struct PrinterData *pd, union printerIO *pio)
{
    struct PrinterUnit *pu = pd->pd_PUnit;
    LONG err = 0, ioerr, len;

    if (pio->ios.io_Flags & IOF_ABORTED) {
        pd_ReplyAborted(pio);
        return;
    }

    pu->pu_Current = pio;
    pu->pu_IOError = 0;

    switch (pio->ios.io_Command) {
    case CMD_RESET:
        err = Printer_Text_Command(pd, aRIN, 0, 0, 0, 0);
        break;
    case CMD_WRITE:
        len = pio->ios.io_Length;
        err = Printer_Text_Write(pd, pio->ios.io_Data, len);
        if (len == -1)
            len = strlen((char *)pio->ios.io_Data);
        pio->ios.io_Actual = len;
        break;
    case PRD_RAWWRITE:
        len = pio->ios.io_Length;
        if (len == -1)
            len = strlen((char *)pio->ios.io_Data);
        if (len > 0)
            err = pd_PWrite(pio->ios.io_Data, len);
        pio->ios.io_Actual = len;
        break;
    case PRD_PRTCOMMAND:
        err = Printer_Text_Command(pd, pio->iopc.io_PrtCommand,
                                   pio->iopc.io_Parm0, pio->iopc.io_Parm1,
                                   pio->iopc.io_Parm2, pio->iopc.io_Parm3);
        break;
    case PRD_DUMPRPORTTAGS:
        err = Printer_Gfx_DumpRPort((struct IODRPReq *)pio, ((struct IODRPTagsReq *)pio)->io_TagList);
        break;
    case PRD_DUMPRPORT:
    case PRD_TPEXTDUMPRPORT:
        err = Printer_Gfx_DumpRPort((struct IODRPReq *)pio, NULL);
        break;
    }

    /* The caller hears only once all of it has reached the output device:
     * its buffer is its own again, and an error the device gave is this
     * request's error. The device's own error numbers mean nothing to a
     * caller of printer.device, so those become PDERR_CANCEL; exec's
     * (IOERR_xxx, negative) are passed on.
     */
    ioerr = pd_Drain(pd);
    if (Printer_Aborted(pd) && (err != 0 || ioerr != 0)) {
        err = IOERR_ABORTED;
    } else if (ioerr != 0) {
        pd_ReportError(pd, pio, 0, ioerr);
        err = (ioerr < 0) ? ioerr : PDERR_CANCEL;
    } else if (err != 0 && err != PDERR_CANCEL && err != IOERR_ABORTED) {
        pd_ReportError(pd, pio, err, 0);
    }
    if (err != 0 && (pio->ios.io_Command == CMD_WRITE || pio->ios.io_Command == PRD_RAWWRITE))
        pio->ios.io_Actual = 0;

    pu->pu_Current = NULL;
    pio->ios.io_Error = err;
    D(bug("%s: Command = %d, Result = %d\n", __func__, pio->ios.io_Command, err));
    ReplyMsg((struct Message *)pio);
}

/* A driver task is created on an OpenDevice() call,
 * and is killed by a CloseDevice() call.
 */
static LONG pd_DriverTask(VOID)
{
    TASK_PRINTERDATA(pd);

    struct Process *me = (struct Process *)FindTask(NULL);
    struct PrinterExtendedData *ped = &pd->pd_SegmentData->ps_PED;
    struct PrinterMessage *msg = NULL;
    union printerIO *pio;
    UWORD cmd;
    LONG ret;

    /* Wait for startup message -
     * we use the DOS port because the
     * pd_Unit has not been created yet
     */
    D(bug("%s: Waiting for startup. pd=%p\n", __func__, pd));
    WaitPort(&me->pr_MsgPort);
    msg = (struct PrinterMessage *)GetMsg(&me->pr_MsgPort);

    D(bug("%s: Initializing driver, Unit Port %p\n", __func__, &pd->pd_Unit));
    ret = pd_Init(pd);

    /* We want to look like a TURBOPRINT driver
     * TPMATCHWORD is in the 3rd ULONG in pd_OldStk
     * TURBOPRINT was documented as using:
     *
     * (BOOL)TP_Installed = ( ((ULONG *)(PD->pd_OldStk))[2] == TPMATCHWORD)
     *
     * So the assumption is that this ULONG is in native endian format.
     */
    {
    typedef union { UBYTE b[4]; ULONG l; } BL;
    BL *p = (BL*)(pd->pd_OldStk + (2 * sizeof(ULONG)));
    p->l = TPMATCHWORD;
    }


    D(bug("%s: Replying with %d\n", __func__, ret));
    msg->mm_Version = ret;
    ReplyMsg(&msg->mm_Message);

    if (0 != ret)
        return ret;

    /* Wait for unit messages on the pd_Unit */
    for (;;) {
        struct PrinterUnit *pu = pd->pd_PUnit;
        struct Node *node, *next;
        LONG err = 0;

        /* Requests aborted while they waited for CMD_START */
        ForeachNodeSafe(&pu->pu_Held, node, next) {
            if (((struct IORequest *)node)->io_Flags & IOF_ABORTED) {
                Remove(node);
                pd_ReplyAborted((union printerIO *)node);
            }
        }

        pio = (union printerIO *)GetMsg(&pd->pd_Unit);
        if (pio == NULL) {
            D(bug("%s: Waiting for command on port %p\n", __func__, &pd->pd_Unit));
            Wait(1UL << pd->pd_Unit.mp_SigBit);
            continue;
        }
        cmd = pio->ios.io_Command;

        D(bug("%s: Command = %d\n", __func__, cmd));
        if (pd_IsPrintCommand(cmd)) {
            if (pu->pu_Stopped)
                AddTail((struct List *)&pu->pu_Held, (struct Node *)pio);
            else
                pd_Print(pd, pio);
            continue;
        }

        switch (cmd) {
        case CMD_OPENDEVICE:
            /* The job runs from the first open of the unit to its last
             * close: an open that joins it does not start the driver again.
             */
            if (!pu->pu_DriverOpen) {
                err = ped->ped_Open(pio);
                if (err == 0) {
                    pu->pu_DriverOpen = TRUE;
                    pu->pu_IOError = 0;
                    pd_SendJobInfo(pd);
                    Printer_Text_Write(pd, "\033#1", 3); /* aRIN */
                    /* a printer that is not ready fails the first write, not the open */
                    pd_Drain(pd);
                }
            }
            break;
        case PRD_SETJOBINFO:
        {
            struct TagItem *tags = (struct TagItem *)pio->ios.io_Data, *tag;

            while ((tag = LibNextTagItem(&tags))) {
                STRPTR dest = NULL;
                LONG size = 0;

                if (tag->ti_Tag == PJIA_Application) {
                    dest = pu->pu_JobApplication;
                    size = sizeof(pu->pu_JobApplication);
                } else if (tag->ti_Tag == PJIA_Document) {
                    dest = pu->pu_JobDocument;
                    size = sizeof(pu->pu_JobDocument);
                }
                if (dest != NULL) {
                    strncpy(dest, tag->ti_Data ? (const char *)tag->ti_Data : "", size - 1);
                    dest[size - 1] = 0;
                }
            }
            pd_SendJobInfo(pd);
            break;
        }
        case CMD_CLOSEDEVICE:
        {
            struct PrinterBase *PrinterBase = pu->pu_PrinterBase;
            LONG unitnum = pd->pd_UnitNumber;

            ObtainSemaphore(&PrinterBase->pb_UnitLock[unitnum]);
            /* Every open of a unit is an open of printer.device */
            Forbid();
            PrinterBase->pb_Device.dd_Library.lib_OpenCnt--;
            Permit();
            if (--pu->pu_Opens > 0) {
                ReleaseSemaphore(&PrinterBase->pb_UnitLock[unitnum]);
                err = IOERR_UNITBUSY;
                break;
            }

            /* The last close: nothing waits for a CMD_START any more */
            while ((node = RemHead((struct List *)&pu->pu_Held)))
                pd_ReplyAborted((union printerIO *)node);
            pd_Close(pd, pio);
            ReleaseSemaphore(&PrinterBase->pb_UnitLock[unitnum]);

            /* The unit is freed by the closer as soon as it has this reply */
            pio->ios.io_Error = 0;
            ReplyMsg((struct Message *)pio);
            D(bug("%s: Shutting down\n", __func__));
            return 0;
        }
        case CMD_FLUSH:
            /* What is on its way to the output device, and what waits for CMD_START */
            if (!(pd->pd_Flags & PDF_NOIO)) {
                AbortIO((struct IORequest *)&pd->pd_ior0);
                WaitIOStd((struct IOStdReq *)&pd->pd_ior0);
                AbortIO((struct IORequest *)&pd->pd_ior1);
                WaitIOStd((struct IOStdReq *)&pd->pd_ior1);
                pd->pd_ior0.pd_i0.io_Error = 0;
                pd->pd_ior1.pd_i1.io_Error = 0;
            }
            pu->pu_IOError = 0;
            while ((node = RemHead((struct List *)&pu->pu_Held)))
                pd_ReplyAborted((union printerIO *)node);
            break;
        case CMD_STOP:
            pu->pu_Stopped = TRUE;
            break;
        case CMD_START:
            pu->pu_Stopped = FALSE;
            /* An immediate command: answered before the queue is worked off */
            pio->ios.io_Error = 0;
            ReplyMsg((struct Message *)pio);
            while (!pu->pu_Stopped && (node = RemHead((struct List *)&pu->pu_Held)))
                pd_Print(pd, (union printerIO *)node);
            continue;
        case PRD_RESETPREFS:
        case PRD_LOADPREFS:
        case PRD_USEPREFS:
        case PRD_SAVEPREFS:
        case PRD_READPREFS:
        case PRD_WRITEPREFS:
        case PRD_EDITPREFS:
            err = pd_DoPreferences(pio, pio->ios.io_Command);
            break;
        case PRD_SETERRHOOK:
            pu->pu_ErrHook = ((struct IOPrtErrReq *)pio)->io_Hook;
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
    }
}

/* Synchronize old-style prefs with new style prefs
 */
static void pd_SyncPrefs(struct PrinterData *pd)
{
    struct Preferences *dprefs = &pd->pd_Preferences;
    struct PrinterPrefs *uprefs = &pd->pd_PUnit->pu_Prefs;
    STRPTR openname = pd->pd_PUnit->pu_OpenName;
    ULONG len;

    dprefs->PrinterType = pd->pd_PrinterType;

    strncpy(dprefs->PrinterFilename, uprefs->pp_Txt.pt_Driver,
            sizeof(dprefs->PrinterFilename));
    dprefs->PrinterFilename[sizeof(dprefs->PrinterFilename)-1] = 0;

    dprefs->PrintPitch = uprefs->pp_Txt.pt_Pitch << 10;
    dprefs->PrintQuality = uprefs->pp_Txt.pt_Quality << 8;
    dprefs->PrintSpacing = uprefs->pp_Txt.pt_Spacing << 9;
    dprefs->PrintLeftMargin = uprefs->pp_Txt.pt_LeftMargin;
    dprefs->PrintRightMargin = uprefs->pp_Txt.pt_RightMargin;
    dprefs->PrintImage = uprefs->pp_Gfx.pg_Image;
    dprefs->PrintAspect = uprefs->pp_Gfx.pg_Aspect;
    dprefs->PrintShade = uprefs->pp_Gfx.pg_Shade;
    dprefs->PrintThreshold = uprefs->pp_Gfx.pg_Threshold;
    dprefs->PaperSize = uprefs->pp_Txt.pt_PaperSize << 4;
    dprefs->PaperType = uprefs->pp_Txt.pt_PaperType << 7;
    dprefs->PaperLength = uprefs->pp_Txt.pt_PaperLength;

    if (uprefs->pp_Unit.pu_DeviceName[0] == 0) {
        if (uprefs->pp_Txt.pt_Port == PP_PARALLEL) {
            strcpy(openname, "parallel.device");
        } else if (uprefs->pp_Txt.pt_Port == PP_SERIAL) {
            strcpy(openname, "serial.device");
        } else {
            strcpy(openname, "printtofile.device");
        }
    } else {
        /* The name passed to OpenDevice(), as on AmigaOS. Older AROS
           preferences left out ".device". */
        CopyMem(uprefs->pp_Unit.pu_DeviceName, openname, DEVICENAMESIZE - 1);
        openname[DEVICENAMESIZE - 1] = 0;
        len = strlen(openname);
        if (len < 7 || strcmp(&openname[len - 7], ".device") != 0)
            strcat(openname, ".device");
    }

    /* Preferences.PrtDevName leaves out ".device" and has room for 15
       characters; the device is opened by its full name. */
    len = strlen(openname) - 7;
    if (len > sizeof(dprefs->PrtDevName) - 1)
        len = sizeof(dprefs->PrtDevName) - 1;
    CopyMem(openname, dprefs->PrtDevName, len);
    dprefs->PrtDevName[len] = 0;

    if (strcmp(openname, "parallel.device") == 0 ||
        strcmp(openname, "usbparallel.device") == 0) {
        pd->pd_ior0.pd_p0.IOPar.io_Message.mn_Length = sizeof(pd->pd_ior0.pd_p0);
        pd->pd_ior0.pd_p0.io_ParFlags = PARF_SHARED;
        pd->pd_ior1.pd_p1.IOPar.io_Message.mn_Length = sizeof(pd->pd_ior1.pd_p1);
        pd->pd_ior1.pd_p1.io_ParFlags = PARF_SHARED;
    } else if (strcmp(openname, "serial.device") == 0) {
        pd->pd_ior0.pd_s0.IOSer.io_Message.mn_Length = sizeof(pd->pd_ior0.pd_s0);
        pd->pd_ior0.pd_s0.io_SerFlags = SERF_SHARED;
        pd->pd_ior1.pd_s1.IOSer.io_Message.mn_Length = sizeof(pd->pd_ior1.pd_s1);
        pd->pd_ior1.pd_s1.io_SerFlags = SERF_SHARED;
    } else {
        pd->pd_ior0.pd_i0.io_Message.mn_Length = sizeof(pd->pd_ior0.pd_i0);
        pd->pd_ior1.pd_i1.io_Message.mn_Length = sizeof(pd->pd_ior1.pd_i1);
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
    struct PrinterPrefs prefs = { 0 };
    BPTR olddir, dir, driverseg;

    if (!Printer_LoadPrefs(PrinterBase, unitnum, &prefs) || prefs.pp_Txt.pt_Driver[0] == 0) {
        D(bug("%s: No valid prefs for printer.device %d\n", __func__, unitnum));
        return NULL;
    }
    if (prefs.pp_Txt.pt_PaperSize && (prefs.pp_Txt.pt_PaperSize & 0xF) == 0)
        prefs.pp_Txt.pt_PaperSize >>= 4;    /* saved by an older Prefs/Printer, already shifted */
    if (prefs.pp_DeviceUnit.pd_UnitName[0] == 0)
        pd_DefaultUnitName(prefs.pp_DeviceUnit.pd_UnitName, unitnum);

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
                    NEWLIST(&pu->pu_Held);

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
                            struct PrinterMessage startup, *reply;

                            D(bug("%s: Driver unit port %p\n", __func__, port));
                            startup.mm_Message.mn_ReplyPort=port;
                            startup.mm_Message.mn_Length = sizeof(startup);
                            startup.mm_Magic = AROS_MAKE_ID('p','r','u','n');
                            startup.mm_Version = 0;
                            PutMsg(&proc->pr_MsgPort, (struct Message *)&startup);
                            WaitPort(port);
                            D(bug("%s: Driver replied\n", __func__));
                            reply = (struct PrinterMessage *)GetMsg(port);
                            D(bug("%s: Driver reply = %p\n", __func__, reply));
                            DeleteMsgPort(port);
                            D(bug("%s: Driver port %p gone\n", __func__, port));
                            if (reply == &startup &&
                                reply->mm_Message.mn_Length == sizeof(*reply) &&
                                reply->mm_Magic == AROS_MAKE_ID('p','r','u','n') &&
                                reply->mm_Version == 0) {
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
