/* dev.c - usbethwrap.device by Chris Hodges
*/

#include "debug.h"

#include "ethwrap.class.h"

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
         AROS_LHA(struct IOSana2Req *, ioreq, A1),
         AROS_LHA(ULONG, unit, D0),
         AROS_LHA(ULONG, flags, D1),
         DEVBASETYPEPTR, base, 1, dev)
{
    AROS_LIBFUNC_INIT

    struct NepClassEth *ncp;
    struct TagItem *taglist;

    KPRINTF(10, ("devOpen ioreq: 0x%08lx unit: %ld flags: 0x%08lx base: 0x%08lx\n",
                 ioreq, unit, flags, base));

    ++base->np_Library.lib_OpenCnt;
    base->np_Library.lib_Flags &= ~LIBF_DELEXP;

    KPRINTF(10, ("devOpen: openCnt = %ld\n", base->np_Library.lib_OpenCnt));
    /* Damn f*cking programs which leave this field to zero! */
    if(ioreq->ios2_Req.io_Message.mn_Length && (ioreq->ios2_Req.io_Message.mn_Length < sizeof(struct IOSana2Req)))
    {
        KPRINTF(20, ("devOpen: invalid MN_LENGTH (%ld < %ld)!\n",
                ioreq->ios2_Req.io_Message.mn_Length, sizeof(struct IOSana2Req)));

        ioreq->ios2_Req.io_Error = IOERR_BADLENGTH;
    } else {
        /* Default to open failure. */
        ioreq->ios2_Req.io_Error = IOERR_OPENFAIL;

        ioreq->ios2_Req.io_Unit = NULL;
        ncp = (struct NepClassEth *) base->np_ClsBase->nh_Units.lh_Head;
        while(ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            if(ncp->ncp_UnitNo == unit)
            {
                if(ncp->ncp_OpenFlags & SANA2OPF_MINE)
                {
                    ioreq->ios2_Req.io_Error = IOERR_UNITBUSY;
                } else {
                    ioreq->ios2_Req.io_Unit = (struct Unit *) ncp;
                }
                break;
            }
            ncp = (struct NepClassEth *) ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
        }

        if(!ioreq->ios2_Req.io_Unit)
        {
            ioreq->ios2_Req.io_Error = IOERR_OPENFAIL;
            KPRINTF(20, ("devOpen: could not open unit!\n"));
        } else {
            ncp->ncp_OpenFlags |= flags;

            /* Got taglist? (don't fail if not available!) */
            taglist = (struct TagItem *) ioreq->ios2_BufferManagement;
            if(taglist)
            {
                struct BufMan *bufman;

                bufman = (struct BufMan *) AllocVec(sizeof(struct BufMan), MEMF_CLEAR|MEMF_PUBLIC);
                if(bufman)
                {
                    /* Get copyfrom routines */
                    bufman->bm_CopyFromBuf = (APTR) GetTagData(S2_CopyFromBuff, (IPTR) NULL, taglist);
                    bufman->bm_CopyToBuf   = (APTR) GetTagData(S2_CopyToBuff, (IPTR) NULL, taglist);

                    /* This is new SANA-II V2 addition */
                    bufman->bm_PacketFilter = (APTR) GetTagData(S2_PacketFilter, (IPTR) NULL, taglist);

                    /* These are new SANA-II V3 addition */
                    bufman->bm_DMACopyFromBuf32 = (APTR) GetTagData(S2_DMACopyFromBuff32, (IPTR) NULL, taglist);
                    bufman->bm_DMACopyToBuf32   = (APTR) GetTagData(S2_DMACopyToBuff32, (IPTR) NULL, taglist);

                    /* Init the list for CMD_READ requests */
                    NewList((struct List *) &bufman->bm_RXQueue);

                    /* Add the new bufman to global bufmanlist */
                    KPRINTF(5, ("Open_Unit: added bufman at 0x%08lx\n", bufman));
                    Forbid();
                    AddHead((struct List *) &ncp->ncp_BufManList, (struct Node *) bufman);
                    Permit();

                    ioreq->ios2_BufferManagement = bufman;
                }
                KPRINTF(5, ("Open_Unit:\n"
                            "copyfrombuf: 0x%08lx copytobuf: 0x%08lx packetfilter: 0x%08lx\n"
                            "dmacopyfrombuf32: 0x%08lx dmacopytobuf32: 0x%08lx\n",
                            bufman->bm_CopyFromBuf, bufman->bm_CopyToBuf, bufman->bm_PacketFilter,
                            bufman->bm_DMACopyFromBuf32, bufman->bm_DMACopyToBuf32));
            }
            /* Opended ok! */
            ioreq->ios2_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
            ioreq->ios2_Req.io_Error                   = 0;
            ioreq->ios2_Req.io_Unit->unit_OpenCnt++;

            /* Allow queuing */
            //ncp->ncp_DenyRequests = FALSE;

            return base;
        }
    }

    ioreq->ios2_Req.io_Unit   = (APTR) -1;
    ioreq->ios2_Req.io_Device = (APTR) -1;
    base->np_Library.lib_OpenCnt--;

    return(NULL);
    
    AROS_LIBFUNC_EXIT
}


AROS_LH1(BPTR, devClose,
         AROS_LHA(struct IOSana2Req *, ioreq, A1),
         DEVBASETYPEPTR, base, 2, dev)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    struct NepClassEth *ncp = (struct NepClassEth *) ioreq->ios2_Req.io_Unit;
    struct BufMan *bufman;

    KPRINTF(10, ("devClose ioreq: 0x%08lx base: 0x%08lx\n", ioreq, base));

    ret = BNULL;
    /* Allow queuing */
    //ncp->ncp_DenyRequests = TRUE;

    ncp->ncp_Unit.unit_OpenCnt--;
    if(ncp->ncp_Unit.unit_OpenCnt == 1)
    {
        ncp->ncp_OpenFlags = 0; // clear all flags, if all units are closed
    }

    ioreq->ios2_Req.io_Unit   = (APTR) -1;
    ioreq->ios2_Req.io_Device = (APTR) -1;

    bufman = ioreq->ios2_BufferManagement;
    if(bufman)
    {
        KPRINTF(5, ("Close_Unit: remove bufman at 0x%08lx\n", bufman));
        ioreq->ios2_BufferManagement = NULL;

        Forbid();
        Remove((struct Node *) bufman);
        Permit();
        FreeVec(bufman);
    }

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
    

/*
 *======================================================================
 * cmdRead(ioreq, base)
 *======================================================================
 *
 * This is the device CMD_READ routine.
 *
 * First it check if nic is in proper state and if user passed arguments
 * are valid. If everything is ok, the request is linked to queue of
 * pending read requests.
 *
 * The request is replied only when packet matching it's ios2_PacketType
 * arrive, or if user abort the request with AbortIO().
 *
 */

WORD cmdRead(struct NepClassEth *ncp, struct IOSana2Req *ioreq)
{
    struct BufMan *bufman;
    KPRINTF(1, ("CMD_READ ioreq: 0x%08lx type: %lu\n", ioreq, ioreq->ios2_PacketType));

    /* Configured? */
    if(!(ncp->ncp_StateFlags & DDF_CONFIGURED))
    {
        return deverror(S2ERR_BAD_STATE, S2WERR_NOT_CONFIGURED);
    }

    /* Online? */
    if(!(ncp->ncp_StateFlags & DDF_ONLINE))
    {
        return deverror(S2ERR_OUTOFSERVICE, S2WERR_UNIT_OFFLINE);
    }

    /* Valid bufman? Got copytobuf routine? */
    bufman = ioreq->ios2_BufferManagement;
    if((!bufman) || (!bufman->bm_CopyToBuf))
    {
        return deverror(S2ERR_BAD_ARGUMENT, S2WERR_NULL_POINTER);
    }

    /* Must be queued */
    ioreq->ios2_Req.io_Flags &= ~IOF_QUICK;

    /* Add to this ioreq's bufman rxqueue */
    Forbid();
    AddTail((struct List *) &bufman->bm_RXQueue, (struct Node *) ioreq);
    Permit();
    /* Don't reply the ioreq, it's pending
    */
    return RC_DONTREPLY;
}


/*
 *======================================================================
 * cmdWrite(ioreq, base)
 *======================================================================
 *
 * This is the device CMD_WRITE routine.
 *
 * First it check if nic is in proper state and if user passed arguments
 * are valid. If everything is ok, the request is linked to queue of
 * pending write requests.
 *
 * The request is replied as soon as the write has been processed or if
 * user abort the request with AbortIO().
 *
 */

WORD cmdWrite(struct NepClassEth *ncp, struct IOSana2Req *ioreq)
{
    struct BufMan *bufman;
    int size;

    KPRINTF(1, ("CMD_WRITE ioreq: 0x%08lx type: %lu len: %lu\n",
                ioreq, ioreq->ios2_PacketType, ioreq->ios2_DataLength));

    /* Configured? */
    if(!(ncp->ncp_StateFlags & DDF_CONFIGURED))
    {
        return deverror(S2ERR_BAD_STATE, S2WERR_NOT_CONFIGURED);
    }

    /* Online? */
    if(!(ncp->ncp_StateFlags & DDF_ONLINE))
    {
        return deverror(S2ERR_OUTOFSERVICE, S2WERR_UNIT_OFFLINE);
    }

    /* Valid bufman? got copyfrombuf routine? */
    bufman = ioreq->ios2_BufferManagement;
    if((!bufman) || (!bufman->bm_CopyFromBuf))
    {
        return deverror(S2ERR_BAD_ARGUMENT, S2WERR_NULL_POINTER);
    }

    /* Check packet size */
    size = (ioreq->ios2_Req.io_Flags & SANA2IOF_RAW) ? RAWPKT_SIZE : ETHERPKT_SIZE;
    if(ioreq->ios2_DataLength > size)
    {
        /* Trigger any tx or generic error events */
        nDoEvent(ncp, S2EVENT_ERROR|S2EVENT_TX);
        return deverror(S2ERR_MTU_EXCEEDED, S2WERR_GENERIC_ERROR);
    }

    /* Must be queued */
    ioreq->ios2_Req.io_Flags &= ~IOF_QUICK;
    Forbid();
    AddTail(&ncp->ncp_WriteQueue, &ioreq->ios2_Req.io_Message.mn_Node);
    if(ncp->ncp_Unit.unit_MsgPort.mp_SigTask)
    {
        Signal(ncp->ncp_Unit.unit_MsgPort.mp_SigTask, 1L<<(ncp->ncp_Unit.unit_MsgPort.mp_SigBit));
    }
    Permit();
    /* Don't reply the ioreq, it's pending */
    return RC_DONTREPLY;
}


/*
 *======================================================================
 * cmdFlush(ioreq, base)
 *======================================================================
 *
 * This is the device CMD_FLUSH routine.
 *
 * This routine abort all pending read, write and event requests.
 *
 */

WORD cmdFlush(struct NepClassEth *ncp, struct IOSana2Req *ioreq)
{
    struct BufMan *bufman;

    KPRINTF(1, ("CMD_FLUSH ioreq: 0x%08lx\n", ioreq));

    bufman = ioreq->ios2_BufferManagement;
    if(bufman)
    {
      Forbid();
      AbortRW(ncp, bufman, deverror(IOERR_ABORTED, S2WERR_GENERIC_ERROR));
      AbortList(ncp, &ncp->ncp_EventList, bufman, deverror(IOERR_ABORTED, S2WERR_GENERIC_ERROR));
      Permit();
    }

    /* Return success */
    return RC_OK;
}


/*
 *======================================================================
 * cmdDeviceQuery(ioreq, base)
 *======================================================================
 *
 * This is the device S2_DEVICEQUERY routine.
 *
 * This routine return various parameters for this network interface.
 *
 */

static
const struct Sana2DeviceQuery DeviceQueryBlock[] =
{
    {
        sizeof(struct Sana2DeviceQuery), sizeof(struct Sana2DeviceQuery),
        0, 0,
        ETHER_ADDR_SIZE * 8,
        ETHERPKT_SIZE,
        10000000,
        S2WireType_Ethernet
    }
};


WORD cmdDeviceQuery(struct NepClassEth *ncp, struct IOSana2Req *ioreq)
{
    struct Sana2DeviceQuery *query;

    KPRINTF(1, ("S2_DEVICEQUERY ioreq: 0x%08lx\n", ioreq));

    /* NULL ptr? */
    query = (struct Sana2DeviceQuery *) ioreq->ios2_StatData;
    if(!query)
    {
        return deverror(S2ERR_BAD_ARGUMENT, S2WERR_NULL_POINTER);
    }

    /* Enough space to store the info ? */
    if(query->SizeAvailable < sizeof(struct Sana2DeviceQuery))
    {
        KPRINTF(1, ("S2_DEVICEQUERY: too small buffer supplied! (%lu / %lu)\n",
                    query->SizeAvailable, sizeof(struct Sana2DeviceQuery)));

        query->SizeSupplied = 0;
        return deverror(S2ERR_BAD_ARGUMENT, S2WERR_BAD_STATDATA);
    }

    CopyMem(&((struct Sana2DeviceQuery *) DeviceQueryBlock)->SizeSupplied, &query->SizeSupplied, sizeof(struct Sana2DeviceQuery) - sizeof(ULONG));

    /* Return success */
    return RC_OK;
}

/*
 *======================================================================
 * cmdGetStationAddress(ioreq, base)
 *======================================================================
 *
 * This is the device S2_GETSTATIONADDRESS routine.
 *
 * This routine return current and default interface hardware addresses.
 *
 */

WORD cmdGetStationAddress(struct NepClassEth *ncp, struct IOSana2Req *ioreq)
{
    UWORD cnt;

    KPRINTF(1, ("S2_GETSTATIONADDRESS ioreq: 0x%08lx\n", ioreq));

    /* Source address = current station address
       Dest address = default station address
    */
    for(cnt = 0; cnt < ETHER_ADDR_SIZE; cnt++)
    {
        ioreq->ios2_SrcAddr[cnt] = ncp->ncp_MacAddress[cnt];
        ioreq->ios2_DstAddr[cnt] = ncp->ncp_ROMAddress[cnt];
    }

    /* Clear the rest */
    for(; cnt < SANA2_MAX_ADDR_BYTES; cnt++)
    {
      ioreq->ios2_SrcAddr[cnt] = 0;
      ioreq->ios2_DstAddr[cnt] = 0;
    }

    /* Return success */
    return(RC_OK);
}

WORD cmdConfigInterface(struct NepClassEth *ncp, struct IOSana2Req *ioreq)
{
    UWORD cnt;

    KPRINTF(1, ("S2_CONFIGINTERFACE ioreq: 0x%08lx\n", ioreq));

    /* This stuff must be atomic */
    Forbid();
    if(ncp->ncp_StateFlags & DDF_CONFIGURED)
    {
        Permit();
        /* Error, already configured */
        return deverror(S2ERR_BAD_STATE, S2WERR_IS_CONFIGURED);
    }

    /* Check for valid address */
    if(ioreq->ios2_SrcAddr[0] & 0x80)
    {
        Permit();
        return deverror(S2ERR_BAD_ADDRESS, S2WERR_SRC_ADDRESS);
    }

    /* Copy the address */
    for(cnt = 0; cnt < ETHER_ADDR_SIZE; cnt++)
    {
        ncp->ncp_MacAddress[cnt] = ioreq->ios2_SrcAddr[cnt];
    }

    ncp->ncp_StateFlags |= DDF_CONFIGURED;
    if(ncp->ncp_Unit.unit_MsgPort.mp_SigTask)
    {
        Signal(ncp->ncp_Unit.unit_MsgPort.mp_SigTask, 1L<<(ncp->ncp_Unit.unit_MsgPort.mp_SigBit));
    }
    Permit();
    return(RC_OK);
}

WORD cmdAddMulticastAddress(struct NepClassEth *ncp, struct IOSana2Req *ioreq)

{
    KPRINTF(1, ("S2_ADDMULTICASTADDRESS 0x%08lx%04lx\n",
                ((ULONG *) ioreq->ios2_SrcAddr)[0], ((UWORD *) ioreq->ios2_SrcAddr)[2]));

    return AddMCastRange(ncp, ioreq, ioreq->ios2_SrcAddr, ioreq->ios2_SrcAddr);
}

WORD cmdDelMulticastAddress(struct NepClassEth *ncp, struct IOSana2Req *ioreq)
{
    KPRINTF(1, ("S2_DELMULTICASTADDRESS 0x%08lx%04lx\n",
                ((ULONG *) ioreq->ios2_SrcAddr)[0], ((UWORD *) ioreq->ios2_SrcAddr)[2]));

    return DelMCastRange(ncp, ioreq, ioreq->ios2_SrcAddr, ioreq->ios2_SrcAddr);
}

WORD cmdAddMulticastAddresses(struct NepClassEth *ncp, struct IOSana2Req *ioreq)
{
    KPRINTF(1, ("S2_ADDMULTICASTADDRESSES 0x%08lx%04lx - 0x%08lx%04lx\n",
                ((ULONG *) ioreq->ios2_SrcAddr)[0], ((UWORD *) ioreq->ios2_SrcAddr)[2],
                ((ULONG *) ioreq->ios2_DstAddr)[0], ((UWORD *) ioreq->ios2_DstAddr)[2]));

    return AddMCastRange(ncp, ioreq, ioreq->ios2_SrcAddr, ioreq->ios2_DstAddr);
}

WORD cmdDelMulticastAddresses(struct NepClassEth *ncp, struct IOSana2Req *ioreq)
{
    KPRINTF(1, ("S2_DELMULTICASTADDRESSES 0x%08lx%04lx - 0x%08lx%04lx\n",
                ((ULONG *) ioreq->ios2_SrcAddr)[0], ((UWORD *) ioreq->ios2_SrcAddr)[2],
                ((ULONG *) ioreq->ios2_DstAddr)[0], ((UWORD *) ioreq->ios2_DstAddr)[2]));

    return DelMCastRange(ncp, ioreq, ioreq->ios2_SrcAddr, ioreq->ios2_DstAddr);
}

#define mcmp(a,b) (memcmp(a,b,ETHER_ADDR_SIZE)==0)

WORD AddMCastRange(struct NepClassEth *ncp, struct IOSana2Req *ioreq, UBYTE *lower, UBYTE *upper)
{
    struct MulticastAddressRange *mar;
    WORD cnt;

    /* Valid multicast addresses? */
    if(((lower[0] & 1) == 0) || ((upper[0] & 1) == 0))
    {
        return deverror(S2ERR_BAD_ADDRESS, S2WERR_BAD_MULTICAST);
    }

    /* Lock access to list */
    Forbid();

    /* Find matching node */
    mar = (struct MulticastAddressRange *) ncp->ncp_Multicasts.lh_Head;
    while(mar->mar_Node.ln_Succ)
    {
        if(mcmp(lower, mar->mar_LowerAddr) &&
           mcmp(upper, mar->mar_UpperAddr))
        {
            break;
        }
        mar = (struct MulticastAddressRange *) mar->mar_Node.ln_Succ;
    }

    if(!mar->mar_Node.ln_Succ)
    {
        /* Add new node */
        mar = AllocVec(sizeof(struct MulticastAddressRange), MEMF_CLEAR|MEMF_PUBLIC);
        if(!mar)
        {
            Permit();
            return deverror(S2ERR_NO_RESOURCES, S2WERR_GENERIC_ERROR);
        }
        for(cnt = 0; cnt < ETHER_ADDR_SIZE; cnt++)
        {
            mar->mar_LowerAddr[cnt] = *lower++;
            mar->mar_UpperAddr[cnt] = *upper++;
        }
        mar->mar_UseCount = 1;
        AddTail(&ncp->ncp_Multicasts, (struct Node *) mar);

        Permit();

        return RC_OK;
    }
    mar->mar_UseCount++;
    Permit();

    /* Return success */
    return RC_OK;
}

WORD DelMCastRange(struct NepClassEth *ncp, struct IOSana2Req *ioreq, UBYTE *lower, UBYTE *upper)
{
    struct MulticastAddressRange *mar;

    /* Lock access to list */
    Forbid();

    /* Find matching node */
    mar = (struct MulticastAddressRange *) ncp->ncp_Multicasts.lh_Head;
    while(mar->mar_Node.ln_Succ)
    {
        if(mcmp(lower, mar->mar_LowerAddr) &&
           mcmp(upper, mar->mar_UpperAddr))
        {
            if(!--mar->mar_UseCount)
            {
                Remove((struct Node *) mar);
                Permit();
                FreeVec(mar);

                return RC_OK;
            }
            Permit();
            return RC_OK;
        }
        mar = (struct MulticastAddressRange *) mar->mar_Node.ln_Succ;
    }
    Permit();

    /* Return with failure */
    return deverror(S2ERR_BAD_STATE, S2WERR_BAD_MULTICAST);
}

WORD cmdMulticast(struct NepClassEth *ncp, struct IOSana2Req *ioreq)
{
    KPRINTF(1, ("S2_MULTICAST ioreq: 0x%08lx to: 0x%08lx%04lx\n",
                ioreq,
                ((ULONG *) ioreq->ios2_DstAddr)[0], ((UWORD *) ioreq->ios2_DstAddr)[2]));

    /* Is dest a multicast address? */
    if(ioreq->ios2_DstAddr[0] & 1)
    {
        return cmdWrite(ncp, ioreq);
    }

    /* Trigger any tx or generic error events */
    nDoEvent(ncp, S2EVENT_ERROR|S2EVENT_TX);

    /* Wasn't a multicast addr */
    return deverror(S2ERR_BAD_ADDRESS, S2WERR_BAD_MULTICAST);
}

WORD cmdBroadcast(struct NepClassEth *ncp, struct IOSana2Req *ioreq)
{
    UWORD cnt;
    KPRINTF(1, ("S2_BROADCAST ioreq: 0x%08lx\n", ioreq));

    /* Dest address = broadcast */
    for(cnt = 0; cnt < ETHER_ADDR_SIZE; cnt++)
    {
        ioreq->ios2_DstAddr[cnt] = 0xff;
    }
    return cmdWrite(ncp, ioreq);
}


WORD cmdTrackType(struct NepClassEth *ncp, struct IOSana2Req *ioreq)
{
    struct PacketTypeStats *pts;

    KPRINTF(1, ("S2_TRACKTYPE ioreq: 0x%08lx type: %lu\n",
                ioreq, ioreq->ios2_PacketType));

    Forbid();
    /* Find matching node */
    pts = (struct PacketTypeStats *) ncp->ncp_TrackList.lh_Head;
    while(pts->pts_Node.ln_Succ)
    {
        /* Our packet type? */
        if(pts->pts_PacketType == ioreq->ios2_PacketType)
        {
            Permit();
            return deverror(S2ERR_BAD_STATE, S2WERR_ALREADY_TRACKED);
        }
        pts = (struct PacketTypeStats *) pts->pts_Node.ln_Succ;
    }

    /* Alloc tracking node */
    pts = (struct PacketTypeStats *) AllocVec(sizeof(struct PacketTypeStats), MEMF_CLEAR|MEMF_PUBLIC);
    if(!pts)
    {
        return deverror(S2ERR_NO_RESOURCES, S2WERR_NOT_TRACKED);
    }
    /* Set up tracking node */
    pts->pts_PacketType = ioreq->ios2_PacketType;

    /* Add the new node to tracklist */
    AddTail((struct List *) &ncp->ncp_TrackList, (struct Node *) pts);
    // caching
    if(pts->pts_PacketType == 2048)
    {
        ncp->ncp_TypeStats2048 = &pts->pts_Stats;
    }
    else if(pts->pts_PacketType == 2054)
    {
        ncp->ncp_TypeStats2054 = &pts->pts_Stats;
    }
    Permit();

    /* Return success */
    return RC_OK;
}


WORD cmdUntrackType(struct NepClassEth *ncp, struct IOSana2Req *ioreq)
{
    struct PacketTypeStats *pts;
    KPRINTF(1, ("S2_UNTRACKTYPE ioreq: 0x%08lx type: %lu\n",
                ioreq, ioreq->ios2_PacketType));

    Forbid();
    /* Find matching node */
    pts = (struct PacketTypeStats *) ncp->ncp_TrackList.lh_Head;
    while(pts->pts_Node.ln_Succ)
    {
        /* Our packet type? */
        if(pts->pts_PacketType == ioreq->ios2_PacketType)
        {
            Remove(&pts->pts_Node);
            // caching
            if(pts->pts_PacketType == 2048)
            {
                ncp->ncp_TypeStats2048 = NULL;
            }
            else if(pts->pts_PacketType == 2054)
            {
                ncp->ncp_TypeStats2054 = NULL;
            }
            Permit();
            FreeVec(pts);
            return RC_OK;

        }
        pts = (struct PacketTypeStats *) pts->pts_Node.ln_Succ;
    }
    Permit();
    /* Return failure */
    return deverror(S2ERR_BAD_STATE, S2WERR_NOT_TRACKED);
}


WORD cmdGetTypeStats(struct NepClassEth *ncp, struct IOSana2Req *ioreq)
{
    struct Sana2PacketTypeStats *tostats;
    struct Sana2PacketTypeStats *fromstats;

    KPRINTF(1, ("S2_GETTYPESTATS ioreq: 0x%08lx type: %lu\n",
                ioreq, ioreq->ios2_PacketType));

    /* NULL ptr? */
    tostats = (struct Sana2PacketTypeStats *) ioreq->ios2_StatData;
    if(!tostats)
    {
        return deverror(S2ERR_BAD_ARGUMENT, S2WERR_NULL_POINTER);
    }

    Forbid();
    fromstats = FindPacketTypeStats(ncp, ioreq->ios2_PacketType);
    if(fromstats)
    {
        /* Copy the stats */
        CopyMem(fromstats, tostats, sizeof(struct Sana2PacketTypeStats));
        Permit();

        /* Return success */
        return RC_OK;
    }
    Permit();

    /* Return error */
    return deverror(S2ERR_BAD_STATE, S2WERR_NOT_TRACKED);
}


WORD cmdGetSpecialStats(struct NepClassEth *ncp, struct IOSana2Req *ioreq)
{
    struct Sana2SpecialStatHeader *stats;
    struct Sana2SpecialStatRecord *record;
    ULONG maxcount;

    KPRINTF(1, ("S2_GETSPECIALSTATS ioreq: 0x%08lx\n", ioreq));

    /* NULL ptr? */
    stats = (struct Sana2SpecialStatHeader *) ioreq->ios2_StatData;
    if(!stats)
    {
        return deverror(S2ERR_BAD_ARGUMENT, S2WERR_NULL_POINTER);
    }

    stats->RecordCountSupplied = 0;
    maxcount = stats->RecordCountMax;
    record = (struct Sana2SpecialStatRecord *) (stats + 1);

    if(maxcount--)
    {
        stats->RecordCountSupplied++;

        record->Type   = S2SS_ETHERNET_BADMULTICAST;
        record->Count  = ncp->ncp_BadMulticasts;
        record->String = "bad multicasts";
        record++;

        if(maxcount--)
        {
            stats->RecordCountSupplied++;

            record->Type   = S2SS_ETHERNET_RETRIES;
            record->Count  = ncp->ncp_Retries;
            record->String = "retries";
            //record++;
        }
    }

    /* Return success */
    return RC_OK;
}


WORD cmdGetGlobalStats(struct NepClassEth *ncp, struct IOSana2Req *ioreq)
{
    struct Sana2DeviceStats *stats;

    KPRINTF(1, ("S2_GETGLOBALSTATS ioreq: 0x%08lx\n", ioreq));

    /* NULL ptr? */
    stats = (struct Sana2DeviceStats *) ioreq->ios2_StatData;
    if(!stats)
    {
        return deverror(S2ERR_BAD_ARGUMENT, S2WERR_NULL_POINTER);
    }

    CopyMem(&ncp->ncp_DeviceStats, stats, sizeof(struct Sana2DeviceStats));

    /* Return success */
    return RC_OK;
}


#define KNOWN_EVENTS (S2EVENT_ERROR |   \
                      S2EVENT_TX |      \
                      S2EVENT_RX |      \
                      S2EVENT_ONLINE |  \
                      S2EVENT_OFFLINE | \
                      S2EVENT_BUFF |    \
                      S2EVENT_HARDWARE)

WORD cmdOnEvent(struct NepClassEth *ncp, struct IOSana2Req *ioreq)
{
    KPRINTF(1, ("S2_ONEVENT ioreq: 0x%08lx mask: 0x%08lx\n", ioreq,
                ioreq->ios2_WireError));

    /* Do we know the requested events? */
    if(ioreq->ios2_WireError & (~KNOWN_EVENTS))
    {
        return deverror(S2ERR_NOT_SUPPORTED, S2WERR_BAD_EVENT);
    }

    /* If online return S2EVENT_ONLINE immediately
       If offline return S2EVENT_OFFLINE immediately. */
    if(ncp->ncp_StateFlags & DDF_ONLINE)
    {
        if(ioreq->ios2_WireError & S2EVENT_ONLINE)
        {
            ioreq->ios2_WireError = S2EVENT_ONLINE;

            /* Return success */
            return RC_OK;
        }
    } else {
        if(ioreq->ios2_WireError & S2EVENT_OFFLINE)
        {
            ioreq->ios2_WireError = S2EVENT_OFFLINE;

            /* Return success */
            return RC_OK;
        }
    }

    /* Must be queued */
    ioreq->ios2_Req.io_Flags &= ~IOF_QUICK;

    /* Queue the ioreq */
    Forbid();
    AddTail((struct List *) &ncp->ncp_EventList, (struct Node *) ioreq);
    Permit();

    /* Don't reply the ioreq, it's pending */
    return RC_DONTREPLY;
}


WORD cmdReadOrphan(struct NepClassEth *ncp, struct IOSana2Req *ioreq)
{
    struct BufMan *bufman;

    KPRINTF(1, ("S2_READORPHAN ioreq: 0x%08lx type: %lu\n", ioreq,
                ioreq->ios2_PacketType));

    /* Configured? */
    if(!(ncp->ncp_StateFlags & DDF_CONFIGURED))
    {
        return deverror(S2ERR_BAD_STATE, S2WERR_NOT_CONFIGURED);
    }

    /* Online? */
    if(!(ncp->ncp_StateFlags & DDF_ONLINE))
    {
        return deverror(S2ERR_OUTOFSERVICE, S2WERR_UNIT_OFFLINE);
    }

    /* Valid bufman? got copytobuf routine? */
    bufman = ioreq->ios2_BufferManagement;
    if((!bufman) || (!bufman->bm_CopyToBuf))
    {
        return deverror(S2ERR_BAD_ARGUMENT, S2WERR_NULL_POINTER);
    }

    /* Must be queued */
    ioreq->ios2_Req.io_Flags &= ~IOF_QUICK;

    /* Queue the ioreq */
    Forbid();
    AddTail((struct List *) &ncp->ncp_OrphanQueue, (struct Node *) ioreq);
    Permit();

    /* Don't reply the ioreq, it's pending */
    return RC_DONTREPLY;
}


WORD cmdOnline(struct NepClassEth *ncp, struct IOSana2Req *ioreq)
{
    KPRINTF(1, ("S2_ONLINE ioreq: 0x%08lx\n", ioreq));

    Forbid();
    /* Already online? */
    if(!(ncp->ncp_StateFlags & DDF_ONLINE))
    {
        /* Configured? */
        if(!(ncp->ncp_StateFlags & DDF_CONFIGURED))
        {
            Permit();
            return deverror(S2ERR_BAD_STATE, S2WERR_NOT_CONFIGURED);
        }

        /* Mark no longer offline */
        ncp->ncp_StateFlags &= ~DDF_OFFLINE;

        /* Mark being online */
        ncp->ncp_StateFlags |= DDF_ONLINE;
        if(ncp->ncp_Unit.unit_MsgPort.mp_SigTask)
        {
            Signal(ncp->ncp_Unit.unit_MsgPort.mp_SigTask, 1L<<(ncp->ncp_Unit.unit_MsgPort.mp_SigBit));
        }
        Permit();

        /* Trigger any ONLINE events */
        nDoEvent(ncp, S2EVENT_ONLINE);

        return RC_OK;
    }

    Permit();
    /* Return success */
    return RC_OK;
}


WORD cmdOffline(struct NepClassEth *ncp, struct IOSana2Req *ioreq)

{
    KPRINTF(1, ("S2_OFFLINE ioreq: 0x%08lx\n", ioreq));

    Forbid();
    /* Mark being offline */
    ncp->ncp_StateFlags |= DDF_OFFLINE;

    /* Already offline? */
    if(ncp->ncp_StateFlags & DDF_ONLINE)
    {
        /* Mark no longer online */
        ncp->ncp_StateFlags &= ~DDF_ONLINE;
        if(ncp->ncp_Unit.unit_MsgPort.mp_SigTask)
        {
            Signal(ncp->ncp_Unit.unit_MsgPort.mp_SigTask, 1L<<(ncp->ncp_Unit.unit_MsgPort.mp_SigBit));
        }
        Permit();

        /* *DO NOT* abort pending read & write requests.
           Miami dies a horrible death if this is enabled!
        */
        if(ioreq->ios2_BufferManagement)
        {
            AbortRW(ncp, ioreq->ios2_BufferManagement, deverror(S2ERR_OUTOFSERVICE, S2WERR_UNIT_OFFLINE));
        }

        /* Trigger any OFFLINE events */
        nDoEvent(ncp, S2EVENT_OFFLINE);

        /* Don't reply the ioreq, it's pending */
        return RC_OK;
    }
    Permit();

    /* Return success */
    return RC_OK;
}


AROS_LH1(void, devBeginIO,
         AROS_LHA(struct IOSana2Req *, ioreq, A1),
         DEVBASETYPEPTR, base, 5, dev)
{
    AROS_LIBFUNC_INIT
    
    struct NepClassEth *ncp = (struct NepClassEth *) ioreq->ios2_Req.io_Unit;
    WORD ret = IOERR_NOCMD;

    KPRINTF(1, ("devBeginIO ioreq: 0x%08lx base: 0x%08lx cmd: %lu\n", ioreq, base, ioreq->ios2_Req.io_Command));

    ioreq->ios2_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    ioreq->ios2_Req.io_Error                   = 0;

    if(ioreq->ios2_Req.io_Command < NSCMD_DEVICEQUERY)
    {
        switch(ioreq->ios2_Req.io_Command)
        {
            case CMD_READ:
                ret = cmdRead(ncp, ioreq);
                break;

            case CMD_FLUSH:
                ret = cmdFlush(ncp, ioreq);
                break;

            case CMD_WRITE:
                ret = cmdWrite(ncp, ioreq);
                break;

            case S2_GETSTATIONADDRESS:
                ret = cmdGetStationAddress(ncp, ioreq);
                break;

            case S2_DEVICEQUERY:
                ret = cmdDeviceQuery(ncp, ioreq);
                break;

            case S2_CONFIGINTERFACE:
                ret = cmdConfigInterface(ncp, ioreq);
                break;

            case S2_ADDMULTICASTADDRESS:
                ret = cmdAddMulticastAddress(ncp, ioreq);
                break;

            case S2_DELMULTICASTADDRESS:
                ret = cmdDelMulticastAddress(ncp, ioreq);
                break;

            case S2_MULTICAST:
                ret = cmdMulticast(ncp, ioreq);
                break;

            case S2_BROADCAST:
                ret = cmdBroadcast(ncp, ioreq);
                break;

            case S2_TRACKTYPE:
                ret = cmdTrackType(ncp, ioreq);
                break;

            case S2_UNTRACKTYPE:
                ret = cmdUntrackType(ncp, ioreq);
                break;

            case S2_GETTYPESTATS:
                ret = cmdGetTypeStats(ncp, ioreq);
                break;

            case S2_GETSPECIALSTATS:
                ret = cmdGetSpecialStats(ncp, ioreq);
                break;

            case S2_GETGLOBALSTATS:
                ret = cmdGetGlobalStats(ncp, ioreq);
                break;

            case S2_ONEVENT:
                ret = cmdOnEvent(ncp, ioreq);
                break;

            case S2_READORPHAN:
                ret = cmdReadOrphan(ncp, ioreq);
                break;

            case S2_ONLINE:
                ret = cmdOnline(ncp, ioreq);
                break;

            case S2_OFFLINE:
                ret = cmdOffline(ncp, ioreq);
                break;

            default:
                ret = IOERR_NOCMD;
                break;
        }
    } else {
        switch(ioreq->ios2_Req.io_Command)
        {
            case NSCMD_DEVICEQUERY:
                ret = cmdNSDeviceQuery(ncp, (struct IOStdReq *) ioreq);
                break;

            case S2_ADDMULTICASTADDRESSES:
                ret = cmdAddMulticastAddresses(ncp, ioreq);
                break;

            case S2_DELMULTICASTADDRESSES:
                ret = cmdDelMulticastAddresses(ncp, ioreq);
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
            /* Set error codes */
            ioreq->ios2_Req.io_Error = ret & 0xff;
        }
        /* Terminate the iorequest */
        TermIO(ncp, ioreq);
    }
    
    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, devAbortIO,
         AROS_LHA(struct IOSana2Req *, ioreq, A1),
         DEVBASETYPEPTR, base, 6, dev)
{
    AROS_LIBFUNC_INIT

    struct NepClassEth *ncp = (struct NepClassEth *) ioreq->ios2_Req.io_Unit;
    struct BufMan *worknode, *nextnode;

    KPRINTF(5, ("devAbortIO ioreq: 0x%08lx\n", ioreq));

    /* Is it pending? */
    Forbid();
    if(ioreq->ios2_Req.io_Message.mn_Node.ln_Type == NT_MESSAGE)
    {

        switch (ioreq->ios2_Req.io_Command)
        {
            case CMD_READ:
                /* Check all nodes in dd_bufmanlist until we succeed to abort or run out of nodes. */
                worknode = (struct BufMan *) ncp->ncp_BufManList.lh_Head;
                while((nextnode = (struct BufMan *) (((struct Node *) worknode)->ln_Succ)))
                {
                    if(AbortReq(ncp, &worknode->bm_RXQueue, ioreq) == S2ERR_NO_ERROR)
                    {
                        /* Success, break out */
                        break;
                    }
                    worknode = nextnode;
                }
                break;

            case S2_READORPHAN:
                AbortReq(ncp, &ncp->ncp_OrphanQueue, ioreq);
                break;

            case CMD_WRITE:
            case S2_MULTICAST:
            case S2_BROADCAST:
                AbortReq(ncp, &ncp->ncp_WriteQueue, ioreq);
                break;

            // these commands may dispatch their ioreq to the USB task; catch from the message port
            case S2_ONLINE:
            case S2_OFFLINE:
            case S2_CONFIGINTERFACE:
            case S2_ADDMULTICASTADDRESS:
            case S2_DELMULTICASTADDRESS:
            case S2_ADDMULTICASTADDRESSES:
            case S2_DELMULTICASTADDRESSES:
                AbortReq(ncp, &ncp->ncp_Unit.unit_MsgPort.mp_MsgList, ioreq);
                break;

            case S2_ONEVENT:
                AbortReq(ncp, &ncp->ncp_EventList, ioreq);
                break;

            default:
                KPRINTF(1, ("devAbortIO: not READ, READORPHAN, WRITE, MULTICAST, BROADCAST or ONEVENT\n"));
                break;
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
    CMD_FLUSH, CMD_READ, CMD_WRITE,
    S2_DEVICEQUERY,
    S2_GETSTATIONADDRESS,
    S2_CONFIGINTERFACE,
    S2_ADDMULTICASTADDRESS, S2_DELMULTICASTADDRESS,
    S2_MULTICAST, S2_BROADCAST,
    S2_TRACKTYPE, S2_UNTRACKTYPE,
    S2_GETTYPESTATS, S2_GETSPECIALSTATS, S2_GETGLOBALSTATS,
    S2_ONEVENT,
    S2_READORPHAN,
    S2_ONLINE, S2_OFFLINE,
    NSCMD_DEVICEQUERY,
    S2_ADDMULTICASTADDRESSES, S2_DELMULTICASTADDRESSES,
    0
};

WORD cmdNSDeviceQuery(struct NepClassEth *ncp, struct IOStdReq *ioreq)
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
         TermIO(ncp, (struct IOSana2Req *) ioreq);

         /* Don't reply, we already did.
         */
         return RC_DONTREPLY;
    }

    ioreq->io_Actual         = query->SizeAvailable
                             = sizeof(struct my_NSDeviceQueryResult);
    query->DeviceType        = NSDEVTYPE_SANA2;
    query->DeviceSubType     = 0;
    query->SupportedCommands = NSDSupported;

    /* Return success (note that this will NOT poke ios2_WireError).
    */
    return RC_OK;
}

/*
 *======================================================================
 * AbortReq(list, ioreq, base)
 *======================================================================
 *
 * Locate an IO request in a linked list and abort it if found.
 *
 */

LONG AbortReq(struct NepClassEth *ncp, struct List *list, struct IOSana2Req *ioreq)
{
    struct Node *node;

    for(node = list->lh_Head; node->ln_Succ; node = node->ln_Succ)
    {
        if(node == (struct Node *) ioreq)
        {
            //KPRINTF(1, ("AbortReq: aborted 0x%08lx\n", ioreq));

            Remove(&ioreq->ios2_Req.io_Message.mn_Node);
            ioreq->ios2_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
            ioreq->ios2_Req.io_Error = IOERR_ABORTED;
            ReplyMsg(&ioreq->ios2_Req.io_Message);

            return S2ERR_NO_ERROR;
        }
    }

    return S2ERR_BAD_STATE;
}



/*
 *===========================================================
 * TermIO(ioreq, base)
 *===========================================================
 *
 * Return completed ioreq to sender.
 *
 */

void TermIO(struct NepClassEth *ncp, struct IOSana2Req *ioreq)
{
    ioreq->ios2_Req.io_Message.mn_Node.ln_Type = NT_FREEMSG;

    /* If not quick I/O, reply the message
    */
    if(!(ioreq->ios2_Req.io_Flags & IOF_QUICK))
    {
        ReplyMsg(&ioreq->ios2_Req.io_Message);
    }
}

/*
 *======================================================================
 * AbortList(list, bufman, error, base)
 *======================================================================
 *
 * Aborts all pending ioreqs in given list with io_Error & ios2_WireError
 * set.
 *
 * error is deverror() macro packed error number.
 *
 */

void AbortList(struct NepClassEth *ncp, struct List *list, struct BufMan *bufman, WORD error)
{
    struct IOSana2Req *ioreq, *nextioreq;

    if(bufman)
    {
        for(ioreq = (struct IOSana2Req *) list->lh_Head;
            (nextioreq = (struct IOSana2Req *) ioreq->ios2_Req.io_Message.mn_Node.ln_Succ);
            ioreq = nextioreq)
        {
            if(ioreq->ios2_BufferManagement == bufman)
            {
                Remove(&ioreq->ios2_Req.io_Message.mn_Node);
                /*ioreq->ios2_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;*/
                ioreq->ios2_Req.io_Error = error & 0xff;
                ioreq->ios2_WireError = error >> 8;
                ReplyMsg(&ioreq->ios2_Req.io_Message);
            }
        }
    }
    else
    {
        while((ioreq = (struct IOSana2Req *) RemHead(list)))
        {
            /* ioreq->ios2_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;*/
            ioreq->ios2_Req.io_Error = error & 0xff;
            ioreq->ios2_WireError = error >> 8;
            ReplyMsg(&ioreq->ios2_Req.io_Message);
        }
    }
}


/*
 *======================================================================
 * AbortRW(bufman, error, base)
 *======================================================================
 *
 * Aborts all pending reads and writes with io_Error & ios2_WireError
 * set.
 *
 * error is deverror() macro packed error number.
 *
 */

void AbortRW(struct NepClassEth *ncp, struct BufMan *bufman, WORD error)
{
    AbortList(ncp, &bufman->bm_RXQueue, NULL, error);
    AbortList(ncp, &ncp->ncp_OrphanQueue, bufman, error);
    AbortList(ncp, &ncp->ncp_WriteQueue, bufman, error);
}

struct Sana2PacketTypeStats * FindPacketTypeStats(struct NepClassEth *ncp, ULONG packettype)
{
    struct PacketTypeStats *pts;

    switch(packettype)
    {
        case 2048:
            return ncp->ncp_TypeStats2048;
            break;

        case 2054:
            return ncp->ncp_TypeStats2054;
            break;

        default:
            /* Find matching node - slowly */
            pts = (struct PacketTypeStats *) ncp->ncp_TrackList.lh_Head;
            while(pts->pts_Node.ln_Succ)
            {
                /* Our packet type? */
                if(pts->pts_PacketType == packettype)
                {
                     return &pts->pts_Stats;
                }
                pts = (struct PacketTypeStats *) pts->pts_Node.ln_Succ;
            }
            break;
    }

    return NULL;
}

