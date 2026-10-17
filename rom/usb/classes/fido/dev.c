/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: fido.device - the exec device side of fido.class (after
          usbraw.device by Chris Hodges)
*/

#include "debug.h"
#include "fido.class.h"

AROS_UFH3(DEVBASETYPEPTR, devInit,
          AROS_UFHA(DEVBASETYPEPTR, base, D0),
          AROS_UFHA(BPTR, seglist, A0),
          AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    KPRINTF(10, ("devInit base: 0x%08lx seglist: 0x%08lx SysBase: 0x%08lx\n",
                 base, seglist, SysBase));

    base->np_Library.lib_Node.ln_Type = NT_DEVICE;
    base->np_Library.lib_Node.ln_Name = DEVNAME;
    base->np_Library.lib_Flags        = LIBF_SUMUSED|LIBF_CHANGED;
    base->np_Library.lib_Version      = VERSION_NUMBER;
    base->np_Library.lib_Revision     = REVISION_NUMBER;
    base->np_Library.lib_IdString     = VERSION_STRING;

    base->np_SegList = seglist;

    if((base->np_UtilityBase = OpenLibrary("utility.library", 0)))
    {
        return(base);
    }
    return(NULL);

    AROS_USERFUNC_EXIT
}

#undef UtilityBase
#define UtilityBase base->np_UtilityBase

AROS_LH3(DEVBASETYPEPTR, devOpen,
         AROS_LHA(struct IOStdReq *, ioreq, A1),
         AROS_LHA(ULONG, unit, D0),
         AROS_LHA(ULONG, flags, D1),
         DEVBASETYPEPTR, base, 1, dev)
{
    AROS_LIBFUNC_INIT

    struct NepClassFido *ncp;

    KPRINTF(10, ("devOpen ioreq: 0x%08lx unit: %ld flags: 0x%08lx base: 0x%08lx\n",
                 ioreq, unit, flags, base));

    ++base->np_Library.lib_OpenCnt;
    base->np_Library.lib_Flags &= ~LIBF_DELEXP;

    if(ioreq->io_Message.mn_Length && (ioreq->io_Message.mn_Length < sizeof(struct IOStdReq)))
    {
        KPRINTF(20, ("devOpen: invalid MN_LENGTH (%ld < %ld)!\n",
                ioreq->io_Message.mn_Length, sizeof(struct IOStdReq)));
        ioreq->io_Error = IOERR_BADLENGTH;
    } else {
        ioreq->io_Error = IOERR_OPENFAIL;
        ioreq->io_Unit = NULL;
        ncp = (struct NepClassFido *) base->np_ClsBase->nh_Units.lh_Head;
        while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            if(ncp->ncp_UnitNo == unit)
            {
                ioreq->io_Unit = (struct Unit *) ncp;
                break;
            }
            ncp = (struct NepClassFido *) ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
        }
        if(!ioreq->io_Unit)
        {
            ioreq->io_Error = IOERR_OPENFAIL;
            KPRINTF(20, ("devOpen: could not open unit!\n"));
        } else {
            ioreq->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
            ioreq->io_Error                   = 0;
            ioreq->io_Unit->unit_OpenCnt++;
            ncp->ncp_DenyRequests = FALSE;
            return base;
        }
    }

    ioreq->io_Unit   = (APTR) -1;
    ioreq->io_Device = (APTR) -1;
    base->np_Library.lib_OpenCnt--;

    return(NULL);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(BPTR, devClose,
         AROS_LHA(struct IOStdReq *, ioreq, A1),
         DEVBASETYPEPTR, base, 2, dev)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    struct NepClassFido *ncp = (struct NepClassFido *) ioreq->io_Unit;

    KPRINTF(10, ("devClose ioreq: 0x%08lx base: 0x%08lx\n", ioreq, base));

    ret = BNULL;

    ncp->ncp_Unit.unit_OpenCnt--;

    ioreq->io_Unit   = (APTR) -1;
    ioreq->io_Device = (APTR) -1;

    if(--base->np_Library.lib_OpenCnt == 0)
    {
        if(base->np_Library.lib_Flags & LIBF_DELEXP)
        {
            KPRINTF(5, ("devClose: calling expunge...\n"));
            ret = AROS_LC1(BPTR, devExpunge,
                           AROS_LCA(DEVBASETYPEPTR, base, D0),
                           DEVBASETYPEPTR, base, 3, dev);
        }
    }

    KPRINTF(5, ("devClose: lib_OpenCnt = %ld\n", base->np_Library.lib_OpenCnt));

    return(ret);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(BPTR, devExpunge,
         AROS_LHA(DEVBASETYPEPTR, extralh, D0),
         DEVBASETYPEPTR, base, 3, dev)
{
    AROS_LIBFUNC_INIT

    BPTR ret;

    KPRINTF(10, ("devExpunge base: 0x%08lx\n", base));

    ret = BNULL;

    if(base->np_Library.lib_OpenCnt == 0)
    {
        KPRINTF(5, ("devExpunge: Unloading...\n"));

        CloseLibrary(base->np_UtilityBase);

        ret = base->np_SegList;

        Remove(&base->np_Library.lib_Node);
        FreeMem((char *) base - base->np_Library.lib_NegSize,
                (ULONG) (base->np_Library.lib_NegSize + base->np_Library.lib_PosSize));
        return(ret);
    }
    else
    {
        base->np_Library.lib_Flags |= LIBF_DELEXP;
    }
    return(BNULL);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(DEVBASETYPEPTR, devReserved,
         DEVBASETYPEPTR, base, 4, dev)
{
    AROS_LIBFUNC_INIT
    return NULL;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, devBeginIO,
         AROS_LHA(struct IOStdReq *, ioreq, A1),
         DEVBASETYPEPTR, base, 5, dev)
{
    AROS_LIBFUNC_INIT

    struct NepClassFido *ncp = (struct NepClassFido *) ioreq->io_Unit;
    WORD ret = IOERR_NOCMD;

    KPRINTF(1, ("devBeginIO ioreq: 0x%08lx base: 0x%08lx cmd: %lu\n", ioreq, base, ioreq->io_Command));

    ioreq->io_Message.mn_Node.ln_Type = NT_MESSAGE;
    ioreq->io_Error                   = 0;

    if(ioreq->io_Command < NSCMD_DEVICEQUERY)
    {
        switch (ioreq->io_Command)
        {
            case CMD_FLUSH:
            case CMD_READ:
            case CMD_WRITE:
            case CMD_RESET:
            case CMD_CLEAR:
                /* the unit task does these */
                if(!ncp->ncp_DenyRequests)
                {
                    ioreq->io_Flags &= ~IOF_QUICK;
                    ret = RC_DONTREPLY;
                    PutMsg(&ncp->ncp_Unit.unit_MsgPort, (struct Message *) ioreq);
                } else {
                    ret = IOERR_ABORTED;
                }
                break;

            case FIDOCMD_UNITINFO:
                if(ioreq->io_Data && ioreq->io_Length >= sizeof(struct FidoUnitInfo))
                {
                    struct FidoUnitInfo *info = (struct FidoUnitInfo *) ioreq->io_Data;

                    memset(info, 0, sizeof(*info));
                    info->VendorID = ncp->ncp_UnitVendorID;
                    info->ProductID = ncp->ncp_UnitProdID;
                    info->ReportSize = ncp->ncp_ReportSize;
                    info->Flags = ncp->ncp_EPOut ? 0 : FIDOUF_NOOUTENDPOINT;
                    if(ncp->ncp_ProductName)
                    {
                        strncpy(info->Product, ncp->ncp_ProductName, sizeof(info->Product) - 1);
                    }
                    ioreq->io_Actual = sizeof(*info);
                    ret = RC_OK;
                } else {
                    ret = IOERR_BADLENGTH;
                }
                break;

            default:
                ret = IOERR_NOCMD;
                break;
        }
    } else {
        switch(ioreq->io_Command)
        {
            case NSCMD_DEVICEQUERY:
                ret = cmdNSDeviceQuery((struct IOStdReq *) ioreq, ncp, base);
                break;

            default:
                ret = IOERR_NOCMD;
                break;
        }
    }

    if(ret != RC_DONTREPLY)
    {
        if (ret != RC_OK)
        {
            ioreq->io_Error = ret & 0xff;
        }
        TermIO(ioreq, base);
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, devAbortIO,
         AROS_LHA(struct IOStdReq *, ioreq, A1),
         DEVBASETYPEPTR, base, 6, dev)
{
    AROS_LIBFUNC_INIT

    struct NepClassFido *ncp = (struct NepClassFido *) ioreq->io_Unit;
    struct IOStdReq *iocmp;

    KPRINTF(5, ("devAbortIO ioreq: 0x%08lx\n", ioreq));

    Forbid();
    if(ioreq->io_Message.mn_Node.ln_Type == NT_MESSAGE)
    {
        if(ncp->ncp_ReadPending == ioreq)
        {
            ncp->ncp_AbortRead = TRUE;
            Signal(ncp->ncp_Task, 1L<<ncp->ncp_Unit.unit_MsgPort.mp_SigBit);
            Permit();
            return(0);
        }
        if(ncp->ncp_WritePending == ioreq)
        {
            ncp->ncp_AbortWrite = TRUE;
            Signal(ncp->ncp_Task, 1L<<ncp->ncp_Unit.unit_MsgPort.mp_SigBit);
            Permit();
            return(0);
        }
        iocmp = (struct IOStdReq *) ncp->ncp_ReadQueue.lh_Head;
        while(iocmp->io_Message.mn_Node.ln_Succ)
        {
            if(iocmp == ioreq)
            {
                Remove((struct Node *) ioreq);
                ioreq->io_Error = IOERR_ABORTED;
                ReplyMsg(&ioreq->io_Message);
                Permit();
                return(0);
            }
            iocmp = (struct IOStdReq *) iocmp->io_Message.mn_Node.ln_Succ;
        }
        iocmp = (struct IOStdReq *) ncp->ncp_WriteQueue.lh_Head;
        while(iocmp->io_Message.mn_Node.ln_Succ)
        {
            if(iocmp == ioreq)
            {
                Remove((struct Node *) ioreq);
                ioreq->io_Error = IOERR_ABORTED;
                ReplyMsg(&ioreq->io_Message);
                Permit();
                return(0);
            }
            iocmp = (struct IOStdReq *) iocmp->io_Message.mn_Node.ln_Succ;
        }
    }
    Permit();

    return(-1);

    AROS_LIBFUNC_EXIT
}

/* NSD stuff */

static
const UWORD NSDSupported[] =
{
    CMD_RESET, CMD_CLEAR,
    CMD_FLUSH, CMD_READ,
    CMD_WRITE, FIDOCMD_UNITINFO,
    NSCMD_DEVICEQUERY, 0
};

WORD cmdNSDeviceQuery(struct IOStdReq *ioreq,
                      struct NepClassFido *ncp,
                      struct NepFidoDevBase *base)
{
    struct my_NSDeviceQueryResult *query;

    query = (struct my_NSDeviceQueryResult *) ioreq->io_Data;

    KPRINTF(10, ("NSCMD_DEVICEQUERY ioreq: 0x%08lx query: 0x%08lx\n", ioreq, query));

    if((!query) ||
       (ioreq->io_Length < sizeof(struct my_NSDeviceQueryResult)) ||
       (query->DevQueryFormat != 0) ||
       (query->SizeAvailable != 0))
    {
         ioreq->io_Error = IOERR_NOCMD;
         TermIO((struct IOStdReq *) ioreq, base);
         return RC_DONTREPLY;
    }

    ioreq->io_Actual         = query->SizeAvailable
                             = sizeof(struct my_NSDeviceQueryResult);
    query->DeviceType        = NSDEVTYPE_UNKNOWN;
    query->DeviceSubType     = 0;
    query->SupportedCommands = NSDSupported;

    return RC_OK;
}

void TermIO(struct IOStdReq *ioreq,
            struct NepFidoDevBase *base)
{
    ioreq->io_Message.mn_Node.ln_Type = NT_FREEMSG;

    /* If not quick I/O, reply the message */
    if(!(ioreq->io_Flags & IOF_QUICK))
    {
        ReplyMsg(&ioreq->io_Message);
    }
}
