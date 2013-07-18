/* dev.c - usbscsi.device by Chris Hodges
*/

#include "debug.h"

#include "massstorage.class.h"

AROS_UFH3(DEVBASETYPEPTR, GM_UNIQUENAME(devInit),
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

    return(base);

    AROS_USERFUNC_EXIT
}

AROS_LH3(DEVBASETYPEPTR, devOpen,
         AROS_LHA(struct IORequest *, ioreq, A1),
         AROS_LHA(ULONG, unitnum, D0),
         AROS_LHA(ULONG, flags, D1),
         DEVBASETYPEPTR, base, 1, usbscsidev)
{
    AROS_LIBFUNC_INIT
    
    struct NepClassMS *ncm;

    KPRINTF(10, ("devOpen ioreq: 0x%08lx unit: %ld flags: 0x%08lx base: 0x%08lx\n",
                 ioreq, unitnum, flags, base));

    if(!base->np_UtilityBase)
    {
        if(!(base->np_UtilityBase = OpenLibrary("utility.library", 0)))
        {
            ioreq->io_Error = IOERR_OPENFAIL;
            return(NULL);
        }
    }

    ++base->np_Library.lib_OpenCnt;
    base->np_Library.lib_Flags &= ~LIBF_DELEXP;

    KPRINTF(10, ("devOpen: openCnt = %ld\n", base->np_Library.lib_OpenCnt));

    /* Damn f*cking programs which leave this field to zero! */
    if(ioreq->io_Message.mn_Length &&
       (ioreq->io_Message.mn_Length < sizeof(struct IOStdReq)))
    {
        KPRINTF(20, ("devOpen: invalid MN_LENGTH (%ld < %ld)!\n",
                ioreq->io_Message.mn_Length, sizeof(struct IOStdReq)));

        ioreq->io_Error = IOERR_BADLENGTH;
    } else {
        /* Default to open failure. */
        ioreq->io_Error = IOERR_OPENFAIL;

        ioreq->io_Unit = NULL;
        ncm = (struct NepClassMS *) base->np_ClsBase->nh_Units.lh_Head;
        while(ncm->ncm_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            if(ncm->ncm_UnitNo == unitnum)
            {
                if(!ncm->ncm_DenyRequests)
                {
                    ioreq->io_Unit = (struct Unit *) ncm;
                    break;
                }
            }
            ncm = (struct NepClassMS *) ncm->ncm_Unit.unit_MsgPort.mp_Node.ln_Succ;
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
         AROS_LHA(struct IORequest *, ioreq, A1),
         DEVBASETYPEPTR, base, 2, usbscsidev)
{
    AROS_LIBFUNC_INIT
    
    BPTR ret;
    struct NepClassMS *unit = (struct NepClassMS *) ioreq->io_Unit;

    KPRINTF(10, ("devClose ioreq: 0x%08lx base: 0x%08lx\n", ioreq, base));

    ret = BNULL;
    unit->ncm_Unit.unit_OpenCnt--;
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
         DEVBASETYPEPTR, base, 3,usbscsidev)
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
         DEVBASETYPEPTR, base, 4, usbscsidev)
{
    AROS_LIBFUNC_INIT
    return NULL;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, devBeginIO,
         AROS_LHA(struct IOStdReq *, ioreq, A1),
         DEVBASETYPEPTR, base, 5, usbscsidev)
{
    AROS_LIBFUNC_INIT
    
    struct NepClassMS *unit = (struct NepClassMS *) ioreq->io_Unit;
    WORD ret = IOERR_NOCMD;

    KPRINTF(5, ("devBeginIO ioreq: 0x%08lx base: 0x%08lx cmd: %lu\n", ioreq, base, ioreq->io_Command));

    ioreq->io_Message.mn_Node.ln_Type = NT_MESSAGE;
    ioreq->io_Error                   = 0;

    if(ioreq->io_Command < NSCMD_DEVICEQUERY)
    {
        switch (ioreq->io_Command)
        {
            case TD_ADDCHANGEINT:
                Forbid();
                AddTail(&unit->ncm_DCInts, (struct Node *) ioreq);
                Permit();
                ret = RC_DONTREPLY;
                break;

            case TD_REMCHANGEINT:
                Forbid();
                Remove((struct Node *) ioreq);
                Permit();
                ret = 0;
                ioreq->io_Flags &= ~IOF_QUICK;
                break;

            case TD_CHANGENUM:
                ioreq->io_Actual = unit->ncm_ChangeCount;
                ret = 0;
                break;

            case TD_CHANGESTATE:
                ioreq->io_Actual = unit->ncm_UnitReady ? 0 : ~0;
                ret = 0;
                break;

            case TD_PROTSTATUS:
                ioreq->io_Actual = unit->ncm_WriteProtect ? ~0 : 0;
                ret = 0;
                break;

            case CMD_START:
            case CMD_STOP:
            case CMD_RESET:
           	case CMD_FLUSH:
            case CMD_READ:
            case CMD_WRITE:
            case TD_SEEK:
            case TD_FORMAT:
            case TD_GETGEOMETRY:
            case TD_EJECT:
            case TD_READ64:
            case TD_WRITE64:
            case TD_SEEK64:
            case TD_FORMAT64:
            case HD_SCSICMD:
                if(unit->ncm_DenyRequests)
                {
                    ret = TDERR_DiskChanged;
                } else {
                    ioreq->io_Flags &= ~IOF_QUICK;
                    ret = RC_DONTREPLY;
                    PutMsg(&unit->ncm_Unit.unit_MsgPort, (struct Message *) ioreq);
                }
                break;

            /* NOPs */
            case CMD_CLEAR:
            case CMD_UPDATE:
            case TD_MOTOR:
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
                ret = GM_UNIQUENAME(cmdNSDeviceQuery)((struct IOStdReq *) ioreq, unit, base);
                break;

            case NSCMD_TD_READ64:
            case NSCMD_TD_WRITE64:
            case NSCMD_TD_FORMAT64:
            case NSCMD_TD_SEEK64:
                if(unit->ncm_DenyRequests)
                {
                    ret = IOERR_ABORTED;
                } else {
                    ioreq->io_Flags &= ~IOF_QUICK;
                    ret = RC_DONTREPLY;
                    PutMsg(&unit->ncm_Unit.unit_MsgPort, (struct Message *) ioreq);
                }
                break;

            default:
                ret = IOERR_NOCMD;
                break;
        }
    }

    if(ret != RC_DONTREPLY)
    {
        KPRINTF(1, ("GM_UNIQUENAME(TermIO)\n"));
        if (ret != RC_OK)
        {
            /* Set error codes  */
            ioreq->io_Error = ret & 0xff;
        }
        /* Terminate the iorequest */
        GM_UNIQUENAME(TermIO)(ioreq, base);
    }
    
    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, devAbortIO,
         AROS_LHA(struct IOStdReq *, ioreq, A1),
         DEVBASETYPEPTR, base, 6, usbscsidev)
{
    AROS_LIBFUNC_INIT

    struct NepClassMS *unit = (struct NepClassMS *) ioreq->io_Unit;
    struct IOStdReq *iocmp;

    KPRINTF(5, ("devAbortIO ioreq: 0x%08lx\n", ioreq));

    /* Is it pending? */
    if(ioreq->io_Message.mn_Node.ln_Type == NT_MESSAGE)
    {
        if(unit->ncm_XFerPending == ioreq)
        {
            unit->ncm_XFerPending = NULL;
            ioreq->io_Error = IOERR_ABORTED;
            ReplyMsg(&ioreq->io_Message);
            return(0);
        }
        iocmp = (struct IOStdReq *) unit->ncm_XFerQueue.lh_Head;
        while(iocmp->io_Message.mn_Node.ln_Succ)
        {
            if(iocmp == ioreq)
            {
                Remove((struct Node *) ioreq);
                ioreq->io_Error = IOERR_ABORTED;
                ReplyMsg(&ioreq->io_Message);
                return(0);
            }
            iocmp = (struct IOStdReq *) iocmp->io_Message.mn_Node.ln_Succ;
        }
    }
    return(-1);

    AROS_LIBFUNC_EXIT
}

/* NSD stuff */

static
const UWORD GM_UNIQUENAME(NSDSupported)[] =
{
    CMD_CLEAR, CMD_RESET,
    CMD_FLUSH, CMD_READ,
    CMD_WRITE, CMD_START,
    CMD_STOP, CMD_UPDATE,
    TD_SEEK,
    TD_MOTOR, TD_EJECT,
    TD_CHANGENUM, TD_PROTSTATUS,
    TD_CHANGESTATE,
    TD_FORMAT,
    TD_GETGEOMETRY,
    TD_ADDCHANGEINT, TD_REMCHANGEINT,
    TD_READ64, TD_WRITE64, TD_SEEK64,
    TD_FORMAT64,
    HD_SCSICMD,
    NSCMD_TD_READ64,
    NSCMD_TD_WRITE64,
    NSCMD_TD_FORMAT64,
    NSCMD_TD_SEEK64,
    NSCMD_DEVICEQUERY, 0
};

WORD GM_UNIQUENAME(cmdNSDeviceQuery)(struct IOStdReq *ioreq,
                      struct NepClassMS *unit,
                      struct NepMSDevBase *base)
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
         GM_UNIQUENAME(TermIO)((struct IOStdReq *) ioreq, base);

         /* Don't reply, we already did. */
         return RC_DONTREPLY;
    }

    ioreq->io_Actual         = query->SizeAvailable
                             = sizeof(struct my_NSDeviceQueryResult);
    query->DeviceType        = NSDEVTYPE_TRACKDISK;
    query->DeviceSubType     = 0;
    query->SupportedCommands = GM_UNIQUENAME(NSDSupported);

    /* Return success (note that this will NOT poke ios2_WireError).
    */
    return RC_OK;
}

void GM_UNIQUENAME(TermIO)(struct IOStdReq *ioreq,
            struct NepMSDevBase *base)
{
    ioreq->io_Message.mn_Node.ln_Type = NT_FREEMSG;

    /* If not quick I/O, reply the message */
    if(!(ioreq->io_Flags & IOF_QUICK))
    {
        ReplyMsg(&ioreq->io_Message);
    }
}
