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
#include <clib/alib_protos.h>

#include <string.h>

#include LC_LIBDEFS_FILE

#include "ipp_intern.h"

static const UWORD commands[] = {
    CMD_WRITE, CMD_RESET, CMD_UPDATE, CMD_CLEAR, CMD_STOP, CMD_START, CMD_FLUSH,
    PRD_GETDESTINATION, PRD_SETDESTINATION, NSCMD_DEVICEQUERY, 0
};

/* What PRD_GETDESTINATION and PRD_SETDESTINATION carry for a unit */
static const Tag destTags[] = { PDA_URI, PDA_Name, PDA_UUID, PDA_Format, TAG_DONE };

static BOOL isSettings(struct IPPBase *IPPBase, struct Unit *unit)
{
    return (APTR)unit >= (APTR)&IPPBase->ib_Settings[0] && (APTR)unit < (APTR)&IPPBase->ib_Settings[IPP_UNITS];
}

static ULONG unitNumber(struct IPPBase *IPPBase, struct Unit *unit)
{
    return isSettings(IPPBase, unit) ? ((struct IPPSettingsUnit *)unit)->is_Number
                                     : ((struct IPPUnit *)unit)->iu_Number;
}

static void copyValue(STRPTR to, const char *from)
{
    strncpy((char *)to, from, PDA_VALUESIZE - 1);
    to[PDA_VALUESIZE - 1] = 0;
}

static LONG getDestination(struct IPPBase *IPPBase, struct IOStdReq *io)
{
    struct TagItem *tags = io->io_Data, *tag;
    struct IPPConfig *ic;

    if (tags == NULL)
        return IOERR_BADADDRESS;
    /* not on the stack: the device runs on its opener's */
    if (!(ic = AllocVec(sizeof(*ic), MEMF_CLEAR)))
        return PDERR_BUFFERMEMORY;
    IPP_ReadConfig(IPPBase, unitNumber(IPPBase, io->io_Unit), ic);
    while ((tag = LibNextTagItem(&tags)) != NULL) {
        if (tag->ti_Data == 0)
            continue;
        switch (tag->ti_Tag) {
        case PDA_Tags:
            *(const Tag **)tag->ti_Data = destTags;
            break;
        case PDA_URI:
            copyValue((STRPTR)tag->ti_Data, ic->ic_URI);
            break;
        case PDA_Name:
            copyValue((STRPTR)tag->ti_Data, ic->ic_Name);
            break;
        case PDA_UUID:
            copyValue((STRPTR)tag->ti_Data, ic->ic_UUID);
            break;
        case PDA_Format:
            copyValue((STRPTR)tag->ti_Data, ic->ic_HasFormat ? ic->ic_Format : "");
            break;
        }
    }
    FreeVec(ic);
    return 0;
}

static LONG setDestination(struct IPPBase *IPPBase, struct IOStdReq *io)
{
    struct TagItem *tags = io->io_Data, *tag;
    const char *uri = NULL, *name = NULL, *uuid = NULL, *format = NULL;
    struct IPPKey keys[4];
    struct IPPTarget *t;
    LONG nkeys = 0;
    BOOL save = FALSE;

    if (tags == NULL)
        return IOERR_BADADDRESS;
    while ((tag = LibNextTagItem(&tags)) != NULL) {
        const char *v = (const char *)tag->ti_Data;

        switch (tag->ti_Tag) {
        case PDA_URI:
        case PDA_Name:
        case PDA_UUID:
        case PDA_Format:
            if (v == NULL)
                v = "";
            if (strlen(v) >= PDA_VALUESIZE || strchr(v, '\n'))
                return IOERR_BADADDRESS;
            if (tag->ti_Tag == PDA_URI)
                uri = v;
            else if (tag->ti_Tag == PDA_Name)
                name = v;
            else if (tag->ti_Tag == PDA_UUID)
                uuid = v;
            else
                format = v;
            break;
        case PDA_Save:
            save = tag->ti_Data != 0;
            break;
        }
    }
    if (uri) {
        /* a printer address must be one ipp.device can use */
        if (uri[0]) {
            if (!(t = AllocVec(sizeof(*t), MEMF_CLEAR)))
                return PDERR_BUFFERMEMORY;
            if (!IPP_ParseURI(uri, t)) {
                FreeVec(t);
                return IOERR_BADADDRESS;
            }
            FreeVec(t);
        }
        /* another printer: the old one's name and ID go with its address */
        if (!uuid)
            uuid = "";
        if (!name)
            name = "";
        keys[nkeys].ik_Key = "URI=";
        keys[nkeys++].ik_Value = uri;
    }
    if (name) {
        keys[nkeys].ik_Key = "NAME=";
        keys[nkeys++].ik_Value = name;
    }
    if (uuid) {
        keys[nkeys].ik_Key = "UUID=";
        keys[nkeys++].ik_Value = uuid;
    }
    if (format) {
        keys[nkeys].ik_Key = "FORMAT=";
        keys[nkeys++].ik_Value = format;
    }
    if (nkeys == 0)
        return 0;
    if (!IPP_UpdateConfig(IPPBase, unitNumber(IPPBase, io->io_Unit), keys, nkeys,
                          save ? IPP_SAVE : IPP_NOSAVE))
        return IOERR_OPENFAIL;
    return 0;
}

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

    switch (isSettings(IPPBase, io->io_Unit) && io->io_Command != PRD_GETDESTINATION
            && io->io_Command != PRD_SETDESTINATION && io->io_Command != NSCMD_DEVICEQUERY
            ? CMD_INVALID : io->io_Command) {
    case PRD_GETDESTINATION:
        io->io_Error = getDestination(IPPBase, io);
        break;
    case PRD_SETDESTINATION:
        io->io_Error = setDestination(IPPBase, io);
        break;
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
    InitSemaphore(&IPPBase->ib_ResolveLock);
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
    if (flags & PRDOF_SETTINGS) {
        IPPBase->ib_Settings[unitnum].is_Number = unitnum;
        io->io_Unit = (struct Unit *)&IPPBase->ib_Settings[unitnum];
        return TRUE;
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
        } else if (!(ok = IPP_BeginJob(IPPBase, iu, id)) && IPP_Resolve(IPPBase, iu)
                   && IPP_ParseURI(iu->iu_Config.ic_URI, &iu->iu_Target))
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

    if (isSettings(IPPBase, io->io_Unit)) {
        io->io_Unit = NULL;
        return TRUE;
    }
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
