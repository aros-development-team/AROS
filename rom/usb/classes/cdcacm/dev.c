/* dev.c - usbmodem.device by Chris Hodges
*/

#include "debug.h"

#include "cdcacm.class.h"

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
         AROS_LHA(struct IOExtSer *, ioreq, A1),
         AROS_LHA(ULONG, unit, D0),
         AROS_LHA(ULONG, flags, D1),
         DEVBASETYPEPTR, base, 1, dev)
{
    AROS_LIBFUNC_INIT

    struct NepClassSerial *ncp;

    KPRINTF(10, ("devOpen ioreq: 0x%08lx unit: %ld flags: 0x%08lx base: 0x%08lx\n",
                 ioreq, unit, flags, base));

    ++base->np_Library.lib_OpenCnt;
    base->np_Library.lib_Flags &= ~LIBF_DELEXP;

    KPRINTF(10, ("devOpen: openCnt = %ld\n", base->np_Library.lib_OpenCnt));
    /* Damn f*cking programs which leave this field to zero! */
    if(ioreq->IOSer.io_Message.mn_Length && (ioreq->IOSer.io_Message.mn_Length < sizeof(struct IOExtSer)))
    {
        KPRINTF(20, ("devOpen: invalid MN_LENGTH (%ld < %ld)!\n",
                ioreq->IOSer.io_Message.mn_Length, sizeof(struct IOExtSer)));

        ioreq->IOSer.io_Error = IOERR_BADLENGTH;
    } else {
        /* Default to open failure. */
        ioreq->IOSer.io_Error = IOERR_OPENFAIL;

        ioreq->IOSer.io_Unit = NULL;
        ncp = (struct NepClassSerial *) base->np_ClsBase->nh_Units.lh_Head;
        while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            if(ncp->ncp_UnitNo == unit && ncp->ncp_Task )
            {
                if(ncp->ncp_Unit.unit_OpenCnt)
                {
                    ioreq->IOSer.io_Error = IOERR_UNITBUSY;
                } else {
                    ioreq->IOSer.io_Unit = (struct Unit *) ncp;
                }
                break;
            }
            ncp = (struct NepClassSerial *) ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
        }

        if(!ioreq->IOSer.io_Unit)
        {
            ioreq->IOSer.io_Error = IOERR_OPENFAIL;
            KPRINTF(20, ("devOpen: could not open unit!\n"));
        } else {
            /* Opended ok! */
            ioreq->IOSer.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
            ioreq->IOSer.io_Error                   = 0;
            ioreq->IOSer.io_Unit->unit_OpenCnt++;

            ioreq->io_CtlChar = SER_DEFAULT_CTLCHAR;
            ioreq->io_RBufLen = DEFREADBUFLEN;
            ioreq->io_ExtFlags = 0;
            ioreq->io_Baud = 9600;
            ioreq->io_BrkTime = 250000;
            ioreq->io_TermArray.TermArray0 = 0;
            ioreq->io_TermArray.TermArray1 = 0;
            ioreq->io_ReadLen = 8;
            ioreq->io_WriteLen = 8;
            ioreq->io_StopBits = 1;
            if(ioreq->io_SerFlags & SERF_RAD_BOOGIE)
            {
                ioreq->io_SerFlags |= SERF_XDISABLED;
                ioreq->io_SerFlags &= ~SERF_PARTY_ON|SERF_PARTY_ODD;
            }

            /* Allow queuing */
            ncp->ncp_DenyRequests = FALSE;

            /* initial settings */
            ioreq->IOSer.io_Command = SDCMD_SETPARAMS;
            ioreq->IOSer.io_Flags &= ~IOF_QUICK;
            PutMsg(&ncp->ncp_Unit.unit_MsgPort, (struct Message *) ioreq);
            while(!GetMsg(&ncp->ncp_Unit.unit_MsgPort))
            {
                WaitPort(&ncp->ncp_Unit.unit_MsgPort);
            }
            return base;
        }
    }

    ioreq->IOSer.io_Unit   = (APTR) -1;
    ioreq->IOSer.io_Device = (APTR) -1;
    base->np_Library.lib_OpenCnt--;

    return(NULL);
    
    AROS_LIBFUNC_EXIT
}


AROS_LH1(BPTR, devClose,
         AROS_LHA(struct IOExtSer *, ioreq, A1),
         DEVBASETYPEPTR, base, 2, dev)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    struct NepClassSerial *ncp = (struct NepClassSerial *) ioreq->IOSer.io_Unit;

    KPRINTF(10, ("devClose ioreq: 0x%08lx base: 0x%08lx\n", ioreq, base));

    ret = BNULL;

    /* Don't allow queuing */
    ncp->ncp_DenyRequests = TRUE;

    ncp->ncp_Unit.unit_OpenCnt--;
    ioreq->IOSer.io_Unit   = (APTR) -1;
    ioreq->IOSer.io_Device = (APTR) -1;

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
         AROS_LHA(struct IOExtSer *, ioreq, A1),
         DEVBASETYPEPTR, base, 5, dev)
{
    AROS_LIBFUNC_INIT
    
    struct NepClassSerial *ncp = (struct NepClassSerial *) ioreq->IOSer.io_Unit;
    WORD ret = IOERR_NOCMD;

    KPRINTF(1, ("devBeginIO ioreq: 0x%08lx base: 0x%08lx cmd: %lu len: %ld\n", ioreq, base, ioreq->IOSer.io_Command, ioreq->IOSer.io_Length));

    ioreq->IOSer.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    ioreq->IOSer.io_Error                   = 0;

    if(ioreq->IOSer.io_Command < NSCMD_DEVICEQUERY)
    {
        switch (ioreq->IOSer.io_Command)
        {
            case CMD_READ:
                if(ncp->ncp_EPInStream && (!ncp->ncp_DenyRequests))
                {
                    ioreq->IOSer.io_Flags &= ~IOF_QUICK;
                    Forbid();
                    AddTail(&ncp->ncp_ReadQueue, &ioreq->IOSer.io_Message.mn_Node);
                    if(ncp->ncp_Task)
                    {
                        Signal(ncp->ncp_Task, 1UL<<ncp->ncp_TaskMsgPort->mp_SigBit);
                    }
                    Permit();
                    ret = RC_DONTREPLY;
                } else {
                    ret = IOERR_ABORTED;
                }
                break;

            case CMD_WRITE:
                if(!ncp->ncp_DenyRequests)
                {
                    if(ioreq->IOSer.io_Length == -1)
                    {
                        ioreq->IOSer.io_Length = strlen(ioreq->IOSer.io_Data);
                    }
                    ioreq->IOSer.io_Flags &= ~IOF_QUICK;
                    Forbid();
                    AddTail(&ncp->ncp_WriteQueue, &ioreq->IOSer.io_Message.mn_Node);
                    if(ncp->ncp_Task)
                    {
                        Signal(ncp->ncp_Task, 1UL<<ncp->ncp_TaskMsgPort->mp_SigBit);
                    }
                    Permit();
                    ret = RC_DONTREPLY;
                } else {
                    ret = IOERR_ABORTED;
                }
                break;

            case CMD_START:
                ncp->ncp_DevSuspend = FALSE;
                ret = RC_OK;
                break;

            case CMD_STOP:
                ncp->ncp_DevSuspend = TRUE;
                ret = RC_OK;
                break;

            case CMD_CLEAR:
            case CMD_RESET:
            case CMD_FLUSH:
            case SDCMD_QUERY:
            case SDCMD_SETPARAMS:
            case SDCMD_BREAK:
                if(!ncp->ncp_DenyRequests)
                {
                    ioreq->IOSer.io_Flags &= ~IOF_QUICK;
                    ret = RC_DONTREPLY;
                    PutMsg(&ncp->ncp_Unit.unit_MsgPort, (struct Message *) ioreq);
                } else {
                    ioreq->IOSer.io_Actual = 0;
                    ret = IOERR_ABORTED;
                }
                break;

            default:
                ret = IOERR_NOCMD;
                break;
        }
    } else {
        switch(ioreq->IOSer.io_Command)
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
            ioreq->IOSer.io_Error = ret & 0xff;
        }
        /* Terminate the iorequest
        */
        TermIO(ioreq, base);
    }
    
    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, devAbortIO,
         AROS_LHA(struct IOExtSer *, ioreq, A1),
         DEVBASETYPEPTR, base, 6, dev)
{
    AROS_LIBFUNC_INIT

    struct NepClassSerial *ncp = (struct NepClassSerial *) ioreq->IOSer.io_Unit;

    struct IOExtSer *iocmp;

    KPRINTF(5, ("devAbortIO ioreq: 0x%08lx\n", ioreq));

    /* Is it pending?
    */
    Forbid();
    if(ioreq->IOSer.io_Message.mn_Node.ln_Type == NT_MESSAGE)
    {
        /* check if it's the writing pipe */
        if(ioreq == ncp->ncp_WritePending)
        {
            if(ncp->ncp_AbortSignal >= 0)
            {
                /* prod the subtask */
                Signal(ncp->ncp_Task, 1UL<<ncp->ncp_AbortSignal);
            }
            Permit();
            return(0);
        }
        iocmp = (struct IOExtSer *) ncp->ncp_ReadQueue.lh_Head;
        while(iocmp->IOSer.io_Message.mn_Node.ln_Succ)
        {
            if(iocmp == ioreq)
            {
                Remove((struct Node *) ioreq);
                ioreq->IOSer.io_Error = IOERR_ABORTED;
                ReplyMsg(&ioreq->IOSer.io_Message);
                Permit();
                return(0);
            }
            iocmp = (struct IOExtSer *) iocmp->IOSer.io_Message.mn_Node.ln_Succ;
        }
        iocmp = (struct IOExtSer *) ncp->ncp_WriteQueue.lh_Head;
        while(iocmp->IOSer.io_Message.mn_Node.ln_Succ)
        {
            if(iocmp == ioreq)
            {
                Remove((struct Node *)ioreq);
                ioreq->IOSer.io_Error = IOERR_ABORTED;
                ReplyMsg(&ioreq->IOSer.io_Message);
                Permit();
                return(0);
            }
            iocmp = (struct IOExtSer *) iocmp->IOSer.io_Message.mn_Node.ln_Succ;
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
    CMD_CLEAR, CMD_RESET,
    CMD_FLUSH, CMD_READ,
    CMD_WRITE, CMD_START,
    CMD_STOP,  SDCMD_QUERY,
    SDCMD_SETPARAMS,
    SDCMD_BREAK,
    NSCMD_DEVICEQUERY, 0
};

WORD cmdNSDeviceQuery(struct IOStdReq *ioreq,
                      struct NepClassSerial *ncp,
                      struct NepSerDevBase *base)
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
         TermIO((struct IOExtSer *) ioreq, base);

         /* Don't reply, we already did.
         */
         return RC_DONTREPLY;
    }

    ioreq->io_Actual         = query->SizeAvailable
                             = sizeof(struct my_NSDeviceQueryResult);
    query->DeviceType        = NSDEVTYPE_SERIAL;
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

void TermIO(struct IOExtSer *ioreq,
            struct NepSerDevBase *base)
{
    ioreq->IOSer.io_Message.mn_Node.ln_Type = NT_FREEMSG;

    /* If not quick I/O, reply the message
    */
    if(!(ioreq->IOSer.io_Flags & IOF_QUICK))
    {
        ReplyMsg(&ioreq->IOSer.io_Message);
    }
}
