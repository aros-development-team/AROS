/* dev.c - usbraw.device by Chris Hodges
*/

#include "debug.h"

#include "rawwrap.class.h"

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

    /* Store segment */
    base->np_SegList = seglist;
    
    if((base->np_UtilityBase = OpenLibrary("utility.library", 0)))
    {
        KPRINTF(10, ("devInit: Ok\n"));
        KPRINTF(10, ("devInit: openCnt = %ld\n", base->np_Library.lib_OpenCnt));
        return(base);
    }
    else
    {
        return(NULL);
    }
    return(base);

    AROS_USERFUNC_EXIT
}

#undef UtilityBase
#define	UtilityBase	base->np_UtilityBase

AROS_LH3(DEVBASETYPEPTR, devOpen,
         AROS_LHA(struct IOStdReq *, ioreq, A1),
         AROS_LHA(ULONG, unit, D0),
         AROS_LHA(ULONG, flags, D1),
         DEVBASETYPEPTR, base, 1, dev)
{
    AROS_LIBFUNC_INIT

    struct NepClassRawWrap *ncp;

    KPRINTF(10, ("devOpen ioreq: 0x%08lx unit: %ld flags: 0x%08lx base: 0x%08lx\n",
                 ioreq, unit, flags, base));

    ++base->np_Library.lib_OpenCnt;
    base->np_Library.lib_Flags &= ~LIBF_DELEXP;

    KPRINTF(10, ("devOpen: openCnt = %ld\n", base->np_Library.lib_OpenCnt));
    /* Damn f*cking programs which leave this field to zero! */
    if(ioreq->io_Message.mn_Length && (ioreq->io_Message.mn_Length < sizeof(struct IOStdReq)))
    {
        KPRINTF(20, ("devOpen: invalid MN_LENGTH (%ld < %ld)!\n",
                ioreq->io_Message.mn_Length, sizeof(struct IOStdReq)));

        ioreq->io_Error = IOERR_BADLENGTH;
    } else {
        /* Default to open failure. */
        ioreq->io_Error = IOERR_OPENFAIL;

        ioreq->io_Unit = NULL;
        ncp = (struct NepClassRawWrap *) base->np_ClsBase->nh_Units.lh_Head;
        while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            if(ncp->ncp_UnitNo == unit)
            {
                if(ncp->ncp_Unit.unit_OpenCnt && ncp->ncp_CDC->cdc_UnitExclusive)
                {
                    ioreq->io_Error = IOERR_UNITBUSY;
                } else {
                    ioreq->io_Unit = (struct Unit *) ncp;
                }
                break;
            }
            ncp = (struct NepClassRawWrap *) ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
        }

        if(!ioreq->io_Unit)
        {
            ioreq->io_Error = IOERR_OPENFAIL;
            KPRINTF(20, ("devOpen: could not open unit!\n"));
        } else {
            /* Opended ok! */
            ioreq->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
            ioreq->io_Error                   = 0;
            ioreq->io_Unit->unit_OpenCnt++;

            /* Allow queuing */
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
    struct NepClassRawWrap *ncp = (struct NepClassRawWrap *) ioreq->io_Unit;

    KPRINTF(10, ("devClose ioreq: 0x%08lx base: 0x%08lx\n", ioreq, base));

    ret = BNULL;
    /* Try to flush the last buffer */
    Forbid();
    if(ncp->ncp_Unit.unit_MsgPort.mp_SigTask)
    {
        Signal(ncp->ncp_Unit.unit_MsgPort.mp_SigTask, 1L<<(ncp->ncp_Unit.unit_MsgPort.mp_SigBit));
    }
    Permit();

    /* Allow queuing */
    ncp->ncp_DenyRequests = FALSE;

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

        KPRINTF(5, ("devExpunge: removing device node 0x%08lx\n",
                    &base->np_Library.lib_Node));
        Remove(&base->np_Library.lib_Node);

        KPRINTF(5, ("devExpunge: FreeMem()...\n"));
        FreeMem((char *) base - base->np_Library.lib_NegSize,
                (ULONG) (base->np_Library.lib_NegSize + base->np_Library.lib_PosSize));

        KPRINTF(5, ("devExpunge: Unloading done! " DEVNAME " expunged!\n\n"));

        return(ret);
    }
    else
    {
        KPRINTF(5, ("devExpunge: Could not expunge, LIBF_DELEXP set!\n"));
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

    struct NepClassRawWrap *ncp = (struct NepClassRawWrap *) ioreq->io_Unit;
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
                if(!ncp->ncp_DenyRequests)
                {
                    ioreq->io_Flags &= ~IOF_QUICK;
                    ret = RC_DONTREPLY;
                    PutMsg(&ncp->ncp_Unit.unit_MsgPort, (struct Message *) ioreq);
                } else {
                    ret = IOERR_ABORTED;
                }
                break;

            case SDCMD_QUERY:
                ioreq->io_Actual = ncp->ncp_RBufFull;
            case SDCMD_BREAK:
            case SDCMD_SETPARAMS:
                ret = 0;
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
        KPRINTF(1, ("TermIO\n"));
        if (ret != RC_OK)
        {
            /* Set error codes
            */
            ioreq->io_Error = ret & 0xff;
        }
        /* Terminate the iorequest
        */
        TermIO(ioreq, base);
    }
    
    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, devAbortIO,
         AROS_LHA(struct IOStdReq *, ioreq, A1),
         DEVBASETYPEPTR, base, 6, dev)
{
    AROS_LIBFUNC_INIT

    struct NepClassRawWrap *ncp = (struct NepClassRawWrap *) ioreq->io_Unit;

    struct IOStdReq *iocmp;

    KPRINTF(5, ("devAbortIO ioreq: 0x%08lx\n", ioreq));

    /* Is it pending?
    */
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

/* NSD stuff
*/

static
const UWORD NSDSupported[] =
{
    CMD_RESET, CMD_CLEAR,
    CMD_FLUSH, CMD_READ,
    CMD_WRITE,
    NSCMD_DEVICEQUERY, 0
};

WORD cmdNSDeviceQuery(struct IOStdReq *ioreq,
                      struct NepClassRawWrap *ncp,
                      struct NepRawDevBase *base)
{
    struct my_NSDeviceQueryResult *query;

    query = (struct my_NSDeviceQueryResult *) ioreq->io_Data;

    KPRINTF(10, ("NSCMD_DEVICEQUERY ioreq: 0x%08lx query: 0x%08lx\n", ioreq, query));

    /* NULL ptr?
       Enough data?
       Valid request?
    */
    if((!query) ||
       (ioreq->io_Length < sizeof(struct my_NSDeviceQueryResult)) ||
       (query->DevQueryFormat != 0) ||
       (query->SizeAvailable != 0))
    {
        /* Return error. This is special handling, since iorequest is only
           guaranteed to be sizeof(struct IOStdReq). If we'd let our
           devBeginIO dispatcher return the error, it would trash some
           memory past end of the iorequest (ios2_WireError field).
         */
         ioreq->io_Error = IOERR_NOCMD;
         TermIO((struct IOStdReq *) ioreq, base);

         /* Don't reply, we already did.
         */
         return RC_DONTREPLY;
    }

    ioreq->io_Actual         = query->SizeAvailable
                             = sizeof(struct my_NSDeviceQueryResult);
    query->DeviceType        = NSDEVTYPE_UNKNOWN;
    query->DeviceSubType     = 0;
    query->SupportedCommands = NSDSupported;

    /* Return success (note that this will NOT poke ios2_WireError).
    */
    return RC_OK;
}

/*
 *===========================================================
 * TermIO(ioreq, base)
 *===========================================================
 *
 * Return completed ioreq to sender.
 *
 */

void TermIO(struct IOStdReq *ioreq,
            struct NepRawDevBase *base)
{
    ioreq->io_Message.mn_Node.ln_Type = NT_FREEMSG;

    /* If not quick I/O, reply the message
    */
    if(!(ioreq->io_Flags & IOF_QUICK))
    {
        ReplyMsg(&ioreq->io_Message);
    }
}
