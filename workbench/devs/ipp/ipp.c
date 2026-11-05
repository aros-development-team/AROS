/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    ipp.device - printer output to a network printer: IPP (RFC 8010/8011) or a
    raw socket (port 9100). Unit n prints to the printer in ENV:IPP/Unit<n>,
    which FindPrinters writes.

    A job runs from the first open of a unit to the last close, and is streamed
    as it is written: the connection is made at the open (so an unreachable
    printer fails the open), each write is sent, and the close ends the job and
    reads the printer's answer. ENV:IPP/Status says how the last job went.
*/

#include <aros/debug.h>

#include <exec/errors.h>
#include <devices/printer.h>
#include <devices/newstyle.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <string.h>

#include LC_LIBDEFS_FILE

#include "ipp_intern.h"

static const UWORD commands[] = {
    CMD_WRITE, CMD_RESET, CMD_UPDATE, CMD_CLEAR, CMD_STOP, CMD_START, CMD_FLUSH, NSCMD_DEVICEQUERY, 0
};

AROS_LH1(void, BeginIO,
 AROS_LHA(struct IOStdReq *, io, A1),
          struct IPPBase *, IPPBase, 5, IPP)
{
    AROS_LIBFUNC_INIT

    struct IPPUnit *iu = (struct IPPUnit *)io->io_Unit;
    LONG len;

    io->io_Message.mn_Node.ln_Type = NT_MESSAGE;
    io->io_Error = 0;
    io->io_Actual = 0;

    switch (io->io_Command) {
    case CMD_WRITE:
        len = io->io_Length;
        if (len == -1)
            len = io->io_Data ? strlen((const char *)io->io_Data) : 0;
        ObtainSemaphore(&iu->iu_Lock);
        if (iu->iu_Failed)
            io->io_Error = PDERR_CANCEL;
        else if (len > 0) {
            if (IPP_Send(IPPBase, iu, io->io_Data, len))
                io->io_Actual = len;
            else {
                iu->iu_Failed = TRUE;
                IPP_SetStatus(IPPBase, iu, iu->iu_Error);
                io->io_Error = PDERR_CANCEL;
            }
        }
        ReleaseSemaphore(&iu->iu_Lock);
        break;
    case CMD_RESET:
    case CMD_UPDATE:
    case CMD_CLEAR:
    case CMD_STOP:
    case CMD_START:
    case CMD_FLUSH:
        break;
    case NSCMD_DEVICEQUERY:
        if (io->io_Data == NULL || io->io_Length < sizeof(struct NSDeviceQueryResult))
            io->io_Error = IOERR_BADLENGTH;
        else {
            struct NSDeviceQueryResult *q = io->io_Data;
            q->DevQueryFormat = 0;
            q->SizeAvailable = sizeof(*q);
            q->DeviceType = NSDEVTYPE_PRINTER;
            q->DeviceSubType = 0;
            q->SupportedCommands = (UWORD *)commands;
            io->io_Actual = sizeof(*q);
        }
        break;
    default:
        io->io_Error = IOERR_NOCMD;
        break;
    }

    if (!(io->io_Flags & IOF_QUICK))
        ReplyMsg(&io->io_Message);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AbortIO,
 AROS_LHA(struct IORequest *, io, A1),
          struct IPPBase *, IPPBase, 6, IPP)
{
    AROS_LIBFUNC_INIT
    return IOERR_NOCMD;
    AROS_LIBFUNC_EXIT
}

static int GM_UNIQUENAME(Init)(struct IPPBase *IPPBase)
{
    int i;

    for (i = 0; i < IPP_UNITS; i++) {
        InitSemaphore(&IPPBase->ib_Unit[i].iu_Lock);
        IPPBase->ib_Unit[i].iu_Number = i;
        IPPBase->ib_Unit[i].iu_Socket = -1;
    }
    IPPBase->ib_DOSBase = OpenLibrary("dos.library", 36);
    return IPPBase->ib_DOSBase != NULL;
}

static int GM_UNIQUENAME(Expunge)(struct IPPBase *IPPBase)
{
    CloseLibrary(IPPBase->ib_DOSBase);
    return TRUE;
}

static int GM_UNIQUENAME(Open)(struct IPPBase *IPPBase, struct IOStdReq *io, ULONG unitnum, ULONG flags)
{
    struct IPPUnit *iu;
    BOOL ok = TRUE;
    ULONG id;

    if (unitnum >= IPP_UNITS) {
        io->io_Error = IOERR_OPENFAIL;
        return FALSE;
    }
    iu = &IPPBase->ib_Unit[unitnum];
    ObtainSemaphore(&iu->iu_Lock);
    if (iu->iu_Opens == 0) {
        Forbid();
        id = ++IPPBase->ib_NextJob;
        Permit();
        iu->iu_Failed = FALSE;
        iu->iu_Bytes = 0;
        iu->iu_Error[0] = 0;
        if (!IPP_ReadConfig(IPPBase, unitnum, &iu->iu_Config)) {
            strcpy(iu->iu_Error, "no printer set for this unit (FindPrinters UNIT n)");
            ok = FALSE;
        } else if (!IPP_ParseURI(iu->iu_Config.ic_URI, &iu->iu_Target)) {
            strcpy(iu->iu_Error, "the printer address is not an ipp:// or socket:// URI");
            ok = FALSE;
        } else
            ok = IPP_BeginJob(IPPBase, iu, id);
        if (!ok) {
            D(bug("[ipp] unit %lu: %s\n", (unsigned long)unitnum, iu->iu_Error));
            IPP_SetStatus(IPPBase, iu, iu->iu_Error);
        }
    }
    if (ok)
        iu->iu_Opens++;
    ReleaseSemaphore(&iu->iu_Lock);

    if (!ok) {
        io->io_Error = IOERR_OPENFAIL;
        return FALSE;
    }
    io->io_Unit = (struct Unit *)iu;
    return TRUE;
}

static int GM_UNIQUENAME(Close)(struct IPPBase *IPPBase, struct IOStdReq *io)
{
    struct IPPUnit *iu = (struct IPPUnit *)io->io_Unit;
    char done[64] = "printed, ";

    io->io_Unit = NULL;
    ObtainSemaphore(&iu->iu_Lock);
    if (iu->iu_Opens > 0 && --iu->iu_Opens == 0) {
        if (!iu->iu_Failed && IPP_EndJob(IPPBase, iu)) {
            if (iu->iu_Started) {
                IPP_AppendNumber(done, iu->iu_Bytes);
                strcat(done, " bytes");
            } else
                strcpy(done, "nothing to print");
            IPP_SetStatus(IPPBase, iu, done);
        } else if (!iu->iu_Failed)
            IPP_SetStatus(IPPBase, iu, iu->iu_Error);
        IPP_Disconnect(IPPBase, iu);
    }
    ReleaseSemaphore(&iu->iu_Lock);
    return TRUE;
}

ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close), 0)

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge), 0)
