/*
    Copyright (C) 2013-2023, The AROS Development Team. All rights reserved.
*/

/* Driver for the Synopsis DesignWare USB Host Controller
 * This implements the Host Controller Driver (HCD) in the USB spec.
 */

#define DEBUG 1

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/utility.h>
#include <proto/alib.h>
#include <proto/mbox.h>
#include <hardware/videocore.h>
#include <string.h>

#include "dwc2_intern.h"
#include "dwc2_regs.h"

// hmm... these aren't getting automatically defined for some reason
#ifndef VERSION_NUMBER
#define VERSION_NUMBER 1
#endif
#ifndef REVISION_NUMBER
#define REVISION_NUMBER 1
#endif

IPTR __arm_periiobase = 0;

static BOOL init_hardware(LIBBASETYPEPTR DWC2Base);
static struct Unit *open_unit(struct IOUsbHWReq *ioreq, LONG unitnr, LIBBASETYPEPTR DWC2Base);
static void terminate_io(struct IOUsbHWReq *ioreq);
static BOOL queue_control_transfer(struct IOUsbHWReq *ioreq, struct DWC2Unit *unit, LIBBASETYPEPTR DWC2Base);

static void flush_tx_fifo(void);

static WORD cmd_controlxfer(struct IOUsbHWReq *ioreq, struct DWC2Unit *unit, LIBBASETYPEPTR DWC2Base);
static WORD cmd_intxfer(struct IOUsbHWReq *ioreq, struct DWC2Unit *unit, LIBBASETYPEPTR DWC2Base);
static WORD cmd_querydevice(struct IOUsbHWReq *ioreq, struct DWC2Unit *unit, LIBBASETYPEPTR DWC2Base);
static WORD cmd_usboper(struct IOUsbHWReq *ioreq, struct DWC2Unit *unit, LIBBASETYPEPTR DWC2Base);
static WORD cmd_usbreset(struct IOUsbHWReq *ioreq, struct DWC2Unit *unit, LIBBASETYPEPTR DWC2Base);
static WORD cmd_usbresume(struct IOUsbHWReq *ioreq, struct DWC2Unit *unit, LIBBASETYPEPTR DWC2Base);
static WORD cmd_usbsuspend(struct IOUsbHWReq *ioreq, struct DWC2Unit *unit, LIBBASETYPEPTR DWC2Base);

//==============================================================================
// Exported library functions
//==============================================================================

// Library init function
// This is called when the driver is loaded. Return TRUE if the hardware is
// present and supported.
static int FNAME_DEV(Init)(LIBBASETYPEPTR DWC2Base)
{
    uint32_t version;
    uint32_t arch;

    bug("[DWC2] Synopsys DesignWare 2.0 USB Host Controller Driver\n");

    KernelBase = OpenResource("kernel.resource");
    __arm_periiobase = KrnGetSystemAttr(KATTR_PeripheralBase);

    // Check if the DWC2 USB controller is present
    version = rd32le(GSNPSID);
    if ((version & 0xFFFFF000) != 0x4F542000)
    {
        bug("[DWC2] Device not present (got %08lx)\n", version);
        return FALSE;
    }
    bug("[DWC2] Core Release: %c%c%x.%x%x%x\n",
        (version >> 24) & 0xFF, (version >> 16) & 0xFF, (version >> 12) & 0xFF,
        (version >> 8) & 0xF, (version >> 4) & 0xF, version & 0xF);
    arch = (rd32le(GHWCFG2) & GHWCFG2_ARCHITECTURE_MASK) >> GHWCFG2_ARCHITECTURE_SHIFT;
    bug("[DWC2] Architecture: %d - %s\n",
        arch,
        arch == GHWCFG2_EXT_DMA_ARCH ? "External DMA" :
        arch == GHWCFG2_INT_DMA_ARCH ? "Internal DMA" :
        "Slave Only");
    if (arch != GHWCFG2_INT_DMA_ARCH)
    {
        bug("[DWC2] Sorry, this driver only supports the Internal DMA architecture\n");
        return FALSE;
    }
    return TRUE;
}

ADD2INITLIB(FNAME_DEV(Init), 0)

// Library expunge function
// This is called when the driver is unloaded.
static int FNAME_DEV(Expunge)(LIBBASETYPEPTR DWC2Base)
{
    D(bug("[DWC2] %s\n", __PRETTY_FUNCTION__));
    return TRUE;
}

ADD2EXPUNGELIB(FNAME_DEV(Expunge), 0)

// Device open function
// This is called when the driver is opened using OpenDevice.
// Initialise the hardware and return TRUE if successful.
static int FNAME_DEV(Open)(LIBBASETYPEPTR DWC2Base, struct IOUsbHWReq *ioreq, ULONG unit, ULONG flags)
{
    D(bug("[DWC2] %s\n", __PRETTY_FUNCTION__));

    if (ioreq->iouh_Req.io_Message.mn_Length < sizeof(struct IOUsbHWReq))
    {
        ioreq->iouh_Req.io_Error = IOERR_BADLENGTH;
        DWC2_BUG("invalid mn_Length %i, need at least %i", ioreq->iouh_Req.io_Message.mn_Length, sizeof(struct IOUsbHWReq));
        return FALSE;
    }
    ioreq->iouh_Req.io_Unit = open_unit(ioreq, unit, DWC2Base);
    if (ioreq->iouh_Req.io_Unit == NULL)
    {
        ioreq->iouh_Req.io_Error = IOERR_OPENFAIL;
        DWC2_BUG("Could not open unit");
        return FALSE;
    }

    return TRUE;
}

ADD2OPENDEV(FNAME_DEV(Open), 0)

// Device close function
static int FNAME_DEV(Close)(LIBBASETYPEPTR DWC2Base, struct IOUsbHWReq *ioreq)
{
    D(bug("[DWC2] %s\n", __PRETTY_FUNCTION__));
    // TODO: free allocated resources
    return TRUE;
}

ADD2CLOSEDEV(FNAME_DEV(Close), 0)

// BeginIO function
AROS_LH1(
    void, FNAME_DEV(BeginIO),
    AROS_LHA(struct IOUsbHWReq *, ioreq, A1),
    LIBBASETYPEPTR, DWC2Base, 5, dwc2)
{
    AROS_LIBFUNC_INIT

    WORD ret = RC_OK;
    struct DWC2Unit *unit = (struct DWC2Unit *)ioreq->iouh_Req.io_Unit;

    //D(bug("[DWC2] %s\n", __PRETTY_FUNCTION__));
    DWC2_BUG("IOReq @ 0x%08lx, DWC2Base @ 0x%08lx [cmd:%lu]",
        ioreq, DWC2Base, ioreq->iouh_Req.io_Command);

    ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    ioreq->iouh_Req.io_Error                   = UHIOERR_NO_ERROR;

    switch (ioreq->iouh_Req.io_Command)
    {
    case UHCMD_CONTROLXFER:
        ret = cmd_controlxfer(ioreq, unit, DWC2Base);
        break;
    case UHCMD_INTXFER:
        ret = cmd_intxfer(ioreq, unit, DWC2Base);
        break;
    case UHCMD_QUERYDEVICE:
        ret = cmd_querydevice(ioreq, unit, DWC2Base);
        break;
    case UHCMD_USBOPER:
        ret = cmd_usboper(ioreq, unit, DWC2Base);
        break;
    case UHCMD_USBRESET:
        ret = cmd_usbreset(ioreq, unit, DWC2Base);
        break;
    case UHCMD_USBRESUME:
        ret = cmd_usbresume(ioreq, unit, DWC2Base);
        break;
    case UHCMD_USBSUSPEND:
        ret = cmd_usbsuspend(ioreq, unit, DWC2Base);
        break;
    default:
        DWC2_BUG("unhandled command %lu", ioreq->iouh_Req.io_Command);
        ret = IOERR_NOCMD;
        hang();
        break;
    }

    if (ret != RC_DONTREPLY)
    {
        if (ret != RC_OK)
            ioreq->iouh_Req.io_Error = ret & 0xFF;
        terminate_io(ioreq);
    }

    AROS_LIBFUNC_EXIT
}

// AbortIO function
AROS_LH1(
    LONG, FNAME_DEV(AbortIO),
    AROS_LHA(struct IOUsbHWReq *, ioreq, A1),
    LIBBASETYPEPTR, DWC2Base, 6, dwc2)
{
    AROS_LIBFUNC_INIT

    D(bug("[DWC2] %s\n", __PRETTY_FUNCTION__));
    // TODO: implement
    return -1;

    AROS_LIBFUNC_EXIT
}

//==============================================================================
// IO commands
//==============================================================================

static WORD cmd_controlxfer(struct IOUsbHWReq *ioreq, struct DWC2Unit *unit, LIBBASETYPEPTR DWC2Base)
{
    DWC2_BUG("UHCMD_CONTROLXFER: devaddr %i", ioreq->iouh_DevAddr);

    if (ioreq->iouh_DevAddr == unit->hu_RootHubAddr)
        return dwc2_roothub_cmd_controlxfer(ioreq, unit, DWC2Base);

    if (!queue_control_transfer(ioreq, unit, DWC2Base))
        return UHIOERR_HOSTERROR;
    return RC_DONTREPLY;
}

static WORD cmd_intxfer(struct IOUsbHWReq *ioreq, struct DWC2Unit *unit, LIBBASETYPEPTR DWC2Base)
{
    DWC2_BUG("UHCMD_INTXFER: devaddr %i", ioreq->iouh_DevAddr);
    // TODO: implement
    DWC2_BUG("interrupt transfers not implemented yet. coming soon...");
    hang();
    return RC_DONTREPLY;
}

static WORD cmd_querydevice(struct IOUsbHWReq *ioreq, struct DWC2Unit *unit, LIBBASETYPEPTR DWC2Base)
{
    struct TagItem *tagList = (struct TagItem *)ioreq->iouh_Data;
    struct TagItem *tag;
    ULONG count = 0;

    DWC2_BUG("UHCMD_QUERYDEVICE");
    while ((tag = NextTagItem(&tagList)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case UHA_State:
            // TODO: handle actual state
            *(UWORD *)tag->ti_Data = UHSF_OPERATIONAL;
            count++;
            break;
        case UHA_Manufacturer:
            *(STRPTR *)tag->ti_Data = "Synopsys";
            count++;
            break;
        case UHA_ProductName:
            *(STRPTR *)tag->ti_Data = "DWC2 USB 2.0 OTG Controller";
            count++;
            break;
        case UHA_Version:
            *(UWORD *)tag->ti_Data = VERSION_NUMBER;
            count++;
            break;
        case UHA_Revision:
            *(UWORD *)tag->ti_Data = REVISION_NUMBER;
            count++;
            break;
        case UHA_Description:
            *(STRPTR *)tag->ti_Data = "Synopsys/DesignWare USB 2.0 OTG Controller";
            count++;
            break;
        case UHA_Copyright:
            *(STRPTR *)tag->ti_Data = "\xA9""2013-2023 The AROS Dev Team";
            count++;
            break;
        case UHA_DriverVersion:
            *(UWORD *)tag->ti_Data = 0x100;
            count++;
            break;
        }
    }
    ioreq->iouh_Actual = count;
    return RC_OK;
}

static WORD cmd_usboper(struct IOUsbHWReq *ioreq, struct DWC2Unit *unit, LIBBASETYPEPTR DWC2Base)
{
    DWC2_BUG("UHCMD_USBOPER");
    return RC_OK;  // TODO: handle actual state
}

static WORD cmd_usbreset(struct IOUsbHWReq *ioreq, struct DWC2Unit *unit, LIBBASETYPEPTR DWC2Base)
{
    DWC2_BUG("UHCMD_USBRESET");
    if (ioreq->iouh_State & UHSF_OPERATIONAL)  // TODO: handle actual state
        return RC_OK;
    else
        return UHIOERR_USBOFFLINE;
}

static WORD cmd_usbresume(struct IOUsbHWReq *ioreq, struct DWC2Unit *unit, LIBBASETYPEPTR DWC2Base)
{
    DWC2_BUG("UHCMD_USBRESUME");
    return RC_OK;  // TODO: handle actual state
}

static WORD cmd_usbsuspend(struct IOUsbHWReq *ioreq, struct DWC2Unit *unit, LIBBASETYPEPTR DWC2Base)
{
    DWC2_BUG("UHCMD_USBSUSPEND");
    return UHIOERR_HOSTERROR;  // not supported yet
}

//==============================================================================

void dwc2_delay(unsigned int milliseconds)
{
    struct MsgPort *port = CreateMsgPort();
    struct timerequest *req = (struct timerequest *)CreateIORequest(port, sizeof(*req));

    req->tr_node.io_Command = TR_ADDREQUEST;
    req->tr_time.tv_secs = milliseconds / 1000;
    req->tr_time.tv_micro = (milliseconds % 1000) * 1000;
    DoIO((struct IORequest *)req);
    CloseDevice((struct IORequest *)req);
    DeleteIORequest((struct IORequest *)req);
    DeleteMsgPort(port);
}

static void dump_channel_regs(int channel)
{
    bug("Global Regs:\n"
        "  GOTGINT:  %08lX  GRSTCTL:  %08lX  GINTSTS:  %08lX  GINTMSK:  %08lX\n"
        "  GAHBCFG:  %08lX  GUSBCFG:  %08lX  HCFG:     %08lX  HPRT0:    %08lX\n"
        "  HAINT:    %08lX  HAINTMSK: %08lX  GHWCFG2:  %08lX  PCGCCTL:  %08lX\n"
        "  GNPTXSTS: %08lX\n",
        rd32le(GOTGINT), rd32le(GRSTCTL), rd32le(GINTSTS), rd32le(GINTMSK),
        rd32le(GAHBCFG), rd32le(GUSBCFG), rd32le(HCFG), rd32le(HPRT0),
        rd32le(HAINT), rd32le(HAINTMSK), rd32le(GHWCFG2), rd32le(PCGCCTL),
        rd32le(GNPTXSTS));
    bug("Channel %i Regs:\n"
        "  HCCHAR:   %08lX  HCSPLT:   %08lX\n"
        "  HCINT:    %08lX  HCINTMSK: %08lX\n"
        "  HCTSIZ:   %08lX  HCDMA:    %08lX\n",
        channel,
        rd32le(HCCHAR(channel)),
        rd32le(HCSPLT(channel)),
        rd32le(HCINT(channel)), rd32le(HCINTMSK(channel)),
        rd32le(HCTSIZ(channel)),
        rd32le(HCDMA(channel)));
}

static void terminate_io(struct IOUsbHWReq *ioreq)
{
    ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_FREEMSG;
    if (!(ioreq->iouh_Req.io_Flags & IOF_QUICK))
    {
        ReplyMsg(&ioreq->iouh_Req.io_Message);
    }
}

// Finds a channel that is not currently being used
static int alloc_channel(struct DWC2Unit *unit)
{
    int i;

    for (i = 0; i < unit->hu_NumChannels; i++)
    {
        if (unit->hu_Channels[i].hc_Request == NULL)
        {
            memset(&unit->hu_Channels[i], 0, sizeof(unit->hu_Channels[i]));
            return i;
        }
    }
    return -1;
}

// Marks a channel as free
static void free_channel(struct DWC2Unit *unit, int channel)
{
    memset(&unit->hu_Channels[channel], 0, sizeof(unit->hu_Channels[channel]));
}

// Starts transmitting data on the USB, either to or from a device.
static void transmit_data(
    struct DWC2Unit *unit,
    int channel,
    struct IOUsbHWReq *ioreq,
    int pid /* packet ID: TSIZ_SC_MC_PID_* */,
    void *buffer /* input or output buffer */,
    size_t size /* total size of data */,
    size_t packetSize /* size of each packet */,
    int endpointNum /* 0-15 */,
    int endpointDir /* 0: out, 1: in */,
    enum HCCHAR_EPTYPE endpointType)
{
    uint32_t regval;
    int packetCount;
    int multiErrorCount;
    BOOL isLowSpeed = (ioreq->iouh_Flags & UHFF_LOWSPEED) ? TRUE : FALSE;
    static const char *const pidNames[] = {"DATA0", "DATA2", "DATA1"};
    const char *pidName = "(invalid)";

    packetCount = (size + packetSize - 1) / packetSize;
    // Always send at least 1 packet, even if it's zero length
    if (packetCount == 0)
        packetCount = 1;

    // TODO: properly handle large sizes
    if (packetSize >= (1 << unit->hu_PacketSizeWidth))
    {
        DWC2_BUG("packet size %li is too big to fit in %i bits", packetSize, unit->hu_PacketSizeWidth);
        hang();
    }
    if (size >= (1 << unit->hu_TransferSizeWidth))
    {
        DWC2_BUG("transfer size %li is too big to fit in %i bits", size, unit->hu_TransferSizeWidth);
        hang();
    }

    if (pid == 3)
        pidName = (endpointType == HCCHAR_EPTYPE_CONTROL) ? "SETUP" : "MDATA";
    else if (pid < ARRAY_LEN(pidNames))
        pidName = pidNames[pid];
    DWC2_BUG("%s, PID %s, size: %li (%i %li-byte packets)",
        endpointDir ? "IN" : "OUT",
        pidName,
        size,
        packetCount,
        packetSize);
    if (ioreq->iouh_Flags & UHFF_SPLITTRANS)
        DWC2_BUG("split transaction (hub %i, port %i)", ioreq->iouh_SplitHubAddr, ioreq->iouh_SplitHubPort);

    if (endpointDir == 0)
        CacheClearE(buffer, size, CACRF_InvalidateD);  // invalidate cache for input
    else
        CacheClearE(buffer, size, CACRF_ClearD);  // flush cache to main memory

    // program the HCSIZ register
    regval = ((pid << TSIZ_SC_MC_PID_SHIFT) & TSIZ_SC_MC_PID_MASK)
           | ((packetCount << TSIZ_PKTCNT_SHIFT) & TSIZ_PKTCNT_MASK)
           | ((size << TSIZ_XFERSIZE_SHIFT) & TSIZ_XFERSIZE_MASK);
    wr32le(HCTSIZ(channel), regval);

    // program the HCSPLT register
    if (ioreq->iouh_Flags & UHFF_SPLITTRANS)
    {
        regval = HCSPLT_SPLTENA
               | (HCSPLT_XACTPOS_ALL << HCSPLT_XACTPOS_SHIFT)
               | ((ioreq->iouh_SplitHubAddr << HCSPLT_HUBADDR_SHIFT) & HCSPLT_HUBADDR_MASK)
               | ((ioreq->iouh_SplitHubPort << HCSPLT_PRTADDR_SHIFT) & HCSPLT_PRTADDR_MASK);
        multiErrorCount = 1;
    }
    else
    {
        regval = 0;
        multiErrorCount = 1;
    }
    wr32le(HCSPLT(channel), regval);

    // program the HCDMA register
    wr32le(HCDMA(channel), 0xc0000000 | (uint32_t)buffer);

    // program the HCCHAR register (starts the channel)
    regval = ((ioreq->iouh_DevAddr << HCCHAR_DEVADDR_SHIFT) & HCCHAR_DEVADDR_MASK)
           | ((endpointNum << HCCHAR_EPNUM_SHIFT) & HCCHAR_EPNUM_MASK)
           | (endpointDir ? HCCHAR_EPDIR : 0)
           | ((endpointType << HCCHAR_EPTYPE_SHIFT) & HCCHAR_EPTYPE_MASK)
           | (isLowSpeed ? HCCHAR_LSPDDEV : 0)
           | ((packetSize << HCCHAR_MPS_SHIFT) & HCCHAR_MPS_MASK)
           | ((multiErrorCount << HCCHAR_MULTICNT_SHIFT) & HCCHAR_MULTICNT_MASK)
           | HCCHAR_CHENA;
    wr32le(HCCHAR(channel), regval);
}

static void enable_channel_interrupts(int channel)
{
    uint32_t regval;

    // unmask host channel interrupts
    regval = rd32le(GINTMSK);
    regval |= GINTSTS_HCHINT;
    wr32le(GINTMSK, regval);
    // unmask interrupt for this channel
    regval = rd32le(HAINTMSK);
    regval |= 1 << channel;
    wr32le(HAINTMSK, regval);
    // unmask all of this channel's interrupts
    wr32le(HCINTMSK(channel), ~HCINTMSK_RESERVED14_31);
}

static void disable_channel_interrupts(int channel)
{
    uint32_t regval;

    // mask all of this channel's interrupts
    wr32le(HCINTMSK(channel), 0);
    // mask interrupt for this channel
    regval = rd32le(HAINTMSK);
    regval &= ~(1 << channel);
    wr32le(HAINTMSK, regval);
    // clear any pending interrupts
    wr32le(HCINT(channel), 0x7FF);
}

// Starts transferring data on channel
static void resume_channel(int channel)
{
    uint32_t regval;

    // enable the channel to start the transaction
    regval = rd32le(HCCHAR(channel));
    regval |= HCCHAR_CHENA;
    wr32le(HCCHAR(channel), regval);
}

// Stops transferring data on channel and disables its interrupts
static void stop_channel(int channel)
{
    uint32_t regval;

    // disable the channel
    regval = rd32le(HCCHAR(channel));
    regval &= ~HCCHAR_CHENA;  // disable channel
    wr32le(HCCHAR(channel), regval);

    disable_channel_interrupts(channel);
}

// Starts sending request on a channel
static void submit_request(struct DWC2Unit *unit, int channel, struct IOUsbHWReq *ioreq, LIBBASETYPEPTR DWC2Base)
{
    uint32_t haintmask;
    uint32_t chanintmask;
    void *buffer = NULL;
    size_t size;
    uint32_t reg;

    unit->hu_Channels[channel].hc_Request = ioreq;
    ioreq->iouh_Actual = 0;

    if (ioreq->iouh_Flags & UHFF_SPLITTRANS)
    {
        DWC2_BUG("split transactions not supported yet. coming soon...");
        hang();
    }

    if (ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER)
    {
        DWC2_BUG("Starting control transfer %s device %i, endpoint %i with size of %i bytes",
            (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? "from" : "to",
            ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length);

        stop_channel(channel);

        // flush FIFO
        wr32le(GRSTCTL, GRSTCTL_TXFFLSH | GRSTCTL_TXFNUM(0x10));
        while (rd32le(GRSTCTL) & GRSTCTL_TXFFLSH)
            ;

        // Setup packet is always 8 bytes
        size = 8;
        buffer = &ioreq->iouh_SetupData;

        // Don't include the setup packet in the amount transferred
        unit->hu_Channels[channel].hc_BytesTransferred = 0;

        CacheClearE(buffer, size, CACRF_ClearD);  // flush setup packet cache to main memory
        if (ioreq->iouh_SetupData.bmRequestType & URTF_IN)
            CacheClearE(ioreq->iouh_Data, ioreq->iouh_Length, CACRF_InvalidateD);  // invalidate input buffer
        else
            CacheClearE(ioreq->iouh_Data, ioreq->iouh_Length, CACRF_ClearD);  // flush data packet cache to main memory

        enable_channel_interrupts(channel);

        transmit_data(
            unit,                    // unit
            channel,                 // channel
            ioreq,
            TSIZ_SC_MC_PID_SETUP,    // pid
            buffer,                  // buffer
            size,                    // size
            8,                       // packetSize
            ioreq->iouh_Endpoint,    // endpointNum
            0,                       // endpointDir
            HCCHAR_EPTYPE_CONTROL);  // endpointType
    }
}

// called upon successful completion of a transaction
static void advance_channel(int channel, struct DWC2Unit *unit)
{
    struct IOUsbHWReq *ioreq = unit->hu_Channels[channel].hc_Request;
    size_t bytesTransferred = unit->hu_Channels[channel].hc_BytesTransferred;
    size_t totalPacketsTransferred = (ioreq->iouh_Actual + ioreq->iouh_MaxPktSize - 1) / ioreq->iouh_MaxPktSize;
    BOOL odd = (totalPacketsTransferred & 1) != 0;
    int pid;

    // Data PIDs alternate DATA0, DATA1, DATA0, DATA1, etc.
    if (!odd)  // TODO: I guess the DATA0 PID in the SETUP transaction counts?
        pid = TSIZ_SC_MC_PID_DATA1;
    else
        pid = TSIZ_SC_MC_PID_DATA0;

    ioreq->iouh_Actual += bytesTransferred;

    DWC2_BUG("just transferred %i bytes. total: %i of %i (%i packets transferred)",
        bytesTransferred, ioreq->iouh_Actual, ioreq->iouh_Length, totalPacketsTransferred);

    if (ioreq->iouh_Actual >= ioreq->iouh_Length)
    {
        // We're done with the transfer!
        // For Control transfers, we must acknowledge it with a zero-length packet
        if (ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER)
        {
            if (!unit->hu_Channels[channel].hc_AckSent)
            {
                unit->hu_Channels[channel].hc_AckSent = TRUE;
                unit->hu_Channels[channel].hc_BytesTransferred = 0;
                DWC2_BUG("channel %i %s zero length ACK packet",
                    channel,
                    (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? "sending" : "receiving");
                transmit_data(
                        unit,
                        channel,
                        ioreq,
                        pid,
                        ioreq->iouh_Data,        // buffer (dummy)
                        0,                       // size
                        ioreq->iouh_MaxPktSize,  // packetSize
                        ioreq->iouh_Endpoint,    // endpointNum
                        (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? 0 : 1, // endpointDir
                        HCCHAR_EPTYPE_CONTROL);  // endpointType
                /*
                if (ioreq->iouh_SetupData.bmRequestType & URTF_IN)
                {
                    DWC2_BUG("channel %i sending ACK!", channel);
                    transmit_data(
                        unit,
                        channel,
                        ioreq,
                        pid,    // pid
                        ioreq->iouh_Data,        // buffer (dummy)
                        0,                       // size
                        ioreq->iouh_MaxPktSize,  // packetSize
                        ioreq->iouh_Endpoint,    // endpointNum
                        0,                       // endpointDir
                        HCCHAR_EPTYPE_CONTROL);  // endpointType
                }
                else
                {
                    DWC2_BUG("channel %i receiving ACK!", channel);
                    transmit_data(
                        unit,
                        channel,
                        ioreq,
                        pid,    // pid
                        ioreq->iouh_Data,        // buffer (dummy)
                        0,                       // size
                        ioreq->iouh_MaxPktSize,  // packetSize
                        ioreq->iouh_Endpoint,    // endpointNum
                        1,                       // endpointDir
                        HCCHAR_EPTYPE_CONTROL);  // endpointType
                }
                */
                return;
            }
        }

        DWC2_BUG("channel %i has finished!", channel);
        stop_channel(channel);
        free_channel(unit, channel);
        ReplyMsg(&ioreq->iouh_Req.io_Message);
        return;
    }

    if (ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER)
    {
        size_t size = ioreq->iouh_Length - ioreq->iouh_Actual;
        size_t numPackets = (size + ioreq->iouh_MaxPktSize - 1) / ioreq->iouh_MaxPktSize;
        void *buffer = (void *)((uint8_t *)ioreq->iouh_Data + ioreq->iouh_Actual);
        BOOL in = (ioreq->iouh_SetupData.bmRequestType & URTF_IN) != 0;

        DWC2_BUG("continuing control transfer");
        unit->hu_Channels[channel].hc_BytesTransferred = size;
        transmit_data(
            unit,                    // unit
            channel,                 // channel
            ioreq,                   // ioreq
            pid,                     // pid
            buffer,                  // buffer
            size,                    // size
            ioreq->iouh_MaxPktSize,  // packetSize
            ioreq->iouh_Endpoint,    // endpointNum
            (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? 1 : 0,  // endpointDir
            HCCHAR_EPTYPE_CONTROL);  // endpointType
    }
}

static BOOL queue_control_transfer(struct IOUsbHWReq *ioreq, struct DWC2Unit *unit, LIBBASETYPEPTR DWC2Base)
{
    int ret;

    Disable();  // Don't interrupt me, please!

    int channel = alloc_channel(unit);
    if (channel == -1)
    {
        // TODO: queue up commands in this situation
        DWC2_BUG("No available channels!");
        ret = FALSE;
    }
    else
    {
        DWC2_BUG("starting transfer on channel %i", channel);
        submit_request(unit, channel, ioreq, DWC2Base);
        ret = TRUE;
    }

    Enable();
    return ret;
}

//==============================================================================

static struct Unit *open_unit(struct IOUsbHWReq *ioreq, LONG unitnr, LIBBASETYPEPTR DWC2Base)
{
    if (unitnr != 0)
    {
        DWC2_BUG("Sorry, I can only support one instance of this driver!");
        return NULL;
    }
    if (!init_hardware(DWC2Base))
    {
        DWC2_BUG("Hardware initialisation failed");
        return NULL;
    }

    return &DWC2Base->hd_Unit->hu_Unit;
}

//==============================================================================
// Interrupt management
//==============================================================================

static void enable_global_interrupts(void)
{
    DWC2_BUG("Enabling USB Interrupts (Globally)");

    uint32_t ahbcfg = rd32le(GAHBCFG);
    ahbcfg |= GAHBCFG_GLBL_INTR_EN;
    wr32le(GAHBCFG, ahbcfg);
}

static void disable_global_interrupts(void)
{
    DWC2_BUG("Disabling USB Interrupts (Globally)");

    uint32_t ahbcfg = rd32le(GAHBCFG);
    ahbcfg &= ~GAHBCFG_GLBL_INTR_EN;
    wr32le(GAHBCFG, ahbcfg);
}

static void enable_common_interrupts(void)
{
    wr32le(GINTSTS, ~0);  // clear any currently pending interrupts
}

static void enable_host_interrupts(void)
{
    uint32_t intmask;

    wr32le(GINTMSK, 0);  // disable all interrupts
    enable_common_interrupts();
    intmask = rd32le(GINTMSK);
    intmask |= GINTSTS_HCHINT;  // host channel interrupt
    intmask |= GINTSTS_SOF;  // start of frame interrupts
    wr32le(GINTMSK, intmask);
}

// Handles the Start of Frame interrupt
// TODO: we will use this as a timing source for interrupt transfers
static void handle_sof_interrupt(void)
{
    //DWC2_BUG("SOF");
    wr32le(GINTSTS, GINTSTS_SOF);  // acknowledge the interrupt
}

// Handles a host channel interrupt
static void handle_channel_interrupt(int channel, struct DWC2Unit *unit)
{
    uint32_t chanintr = rd32le(HCINT(channel));
    struct IOUsbHWReq *request = unit->hu_Channels[channel].hc_Request;

    // TODO: Properly handle errors
    if (chanintr & HCINTMSK_DATATGLERR)
    {
        DWC2_BUG("channel %i: data toggle error", channel);
        dump_channel_regs(channel);
        hang();
    }
    if (chanintr & HCINTMSK_FRMOVRUN)
    {
        DWC2_BUG("channel %i: frame overrun", channel);
        dump_channel_regs(channel);
        hang();
    }
    if (chanintr & HCINTMSK_BBLERR)
    {
        wr32le(HCINT(channel), HCINTMSK_BBLERR);
        DWC2_BUG("channel %i: babble", channel);
        stop_channel(channel);
        free_channel(unit, channel);
        request->iouh_Req.io_Error = UHIOERR_BABBLE;
        ReplyMsg(&request->iouh_Req.io_Message);
        return;
    }
    if (chanintr & HCINTMSK_XACTERR)
    {
        DWC2_BUG("channel %i: transaction error", channel);
        dump_channel_regs(channel);
        hang();
    }
    if (chanintr & HCINTMSK_NYET)
    {
        DWC2_BUG("channel %i: NYET received", channel);
        dump_channel_regs(channel);
        hang();
    }
    // ACK handled below
    if (chanintr & HCINTMSK_NAK)
    {
        wr32le(HCINT(channel), HCINTMSK_NAK);
        DWC2_BUG("channel %i: nak", channel);
        stop_channel(channel);
        free_channel(unit, channel);
        request->iouh_Req.io_Error = UHIOERR_NAK;
        ReplyMsg(&request->iouh_Req.io_Message);
        return;
    }
    if (chanintr & HCINTMSK_STALL)
    {
        wr32le(HCINT(channel), HCINTMSK_STALL);
        DWC2_BUG("channel %i: stalled", channel);
        stop_channel(channel);
        free_channel(unit, channel);
        request->iouh_Req.io_Error = UHIOERR_STALL;
        ReplyMsg(&request->iouh_Req.io_Message);
        return;
    }
    if (chanintr & HCINTMSK_AHBERR)
    {
        DWC2_BUG("channel %i: AHB error", channel);
        dump_channel_regs(channel);
        hang();
    }

    // Assume a xfercompl with no other errors is the same as an ACK
    // The real Pi 3 sends an ACK, while QEMU does not.
    if (chanintr & HCINTMSK_XFERCOMPL)
    {
        wr32le(HCINT(channel), HCINTMSK_XFERCOMPL|HCINTMSK_ACK|HCINTMSK_CHHLTD);
        DWC2_BUG("channel %i: transfer complete%s", channel, (chanintr & HCINTMSK_ACK) ? " (ACK received)" : "");
        advance_channel(channel, unit);
        return;
    }

    if (chanintr & HCINTMSK_ACK)
    {
        DWC2_BUG("got ACK without transfer completed somehow");
        hang();
    }

    if (chanintr & HCINTMSK_CHHLTD)
    {
        wr32le(HCINT(channel), HCINTMSK_CHHLTD);
        DWC2_BUG("channel %i: halted, not completed", channel);
        dump_channel_regs(channel);
        hang();
    }

    DWC2_BUG("Channel %i interrupt", channel);
    dump_channel_regs(channel);
    hang();
}

// Main USB controller interrupt handler
static void irq_handler(struct DWC2Unit *unit, struct ExecBase *SysBase)
{
    uint32_t intr = rd32le(GINTSTS);

    if (intr & GINTSTS_SOF)  // start of frame interrupt
        handle_sof_interrupt();
    else if (intr & GINTSTS_HCHINT)  // host channel interrupt
    {
        int chan;
        uint32_t haint = rd32le(HAINT);

        for (chan = 0; chan < 16; chan++)
        {
            if (haint & (1 << chan))
            {
                handle_channel_interrupt(chan, unit);
            }
        }
    }
    else
    {
        DWC2_BUG("got other IRQ. GINTSTS: %08lX", intr);
        hang();
    }
}

//==============================================================================

static BOOL device_reset(void)  // TODO: wait timeout
{
    uint32_t reset;

    // wait for AHB master IDLE state
    while (!(rd32le(GRSTCTL) & GRSTCTL_AHBIDLE))
        ;

    // core soft reset
    reset = rd32le(GRSTCTL);
    reset |= GRSTCTL_CSFTRST;
    wr32le(GRSTCTL, reset);

    // wait for soft reset to finish
    while (rd32le(GRSTCTL) & GRSTCTL_CSFTRST)
        ;

    dwc2_delay(100);

    return TRUE;
}

static void flush_tx_fifo(void)
{
    wr32le(GRSTCTL, GRSTCTL_TXFFLSH | GRSTCTL_TXFNUM(0x10));
    while (rd32le(GRSTCTL) & GRSTCTL_TXFFLSH)
        ;
}

static void flush_rx_fifo(void)
{
    wr32le(GRSTCTL, GRSTCTL_RXFFLSH);
    while (rd32le(GRSTCTL) & GRSTCTL_RXFFLSH)
        ;
}

static BOOL init_core(struct DWC2Unit *unit)
{
    uint32_t usbcfg;
    uint32_t hwcfg2;
    uint32_t hwcfg3;
    uint32_t ahbcfg;

    usbcfg = rd32le(GUSBCFG);
#if !ENABLE_HIGH_SPEED
    DWC2_BUG("disabling high speed mode");
    usbcfg |= GUSBCFG_PHYSEL;
#endif
    usbcfg &= ~(GUSBCFG_ULPI_EXT_VBUS_DRV|GUSBCFG_TERMSELDLPULSE);
    wr32le(GUSBCFG, usbcfg);

    if (!device_reset())
    {
        DWC2_BUG("Device reset failed");
        return FALSE;
    }

    usbcfg = rd32le(GUSBCFG);
    usbcfg &= ~GUSBCFG_ULPI_UTMI_SEL;  // select UTMI+
    usbcfg &= ~GUSBCFG_PHYIF16;  // UTMI width is 8 (not 16)
    wr32le(GUSBCFG, usbcfg);

    // Internal DMA mode only
    hwcfg2 = rd32le(GHWCFG2);
    usbcfg = rd32le(GUSBCFG);
    if (((hwcfg2 & GHWCFG2_HS_PHY_TYPE_MASK) >> GHWCFG2_HS_PHY_TYPE_SHIFT) == GHWCFG2_HS_PHY_TYPE_ULPI
     && ((hwcfg2 & GHWCFG2_FS_PHY_TYPE_MASK) >> GHWCFG2_FS_PHY_TYPE_SHIFT) == GHWCFG2_FS_PHY_TYPE_DEDICATED)
        usbcfg |= GUSBCFG_ULPI_FS_LS|GUSBCFG_ULPI_CLK_SUSP_M;
    else
        usbcfg &= ~(GUSBCFG_ULPI_FS_LS|GUSBCFG_ULPI_CLK_SUSP_M);
    wr32le(GUSBCFG, usbcfg);
    unit->hu_NumChannels = 1 + ((hwcfg2 & GHWCFG2_NUM_HOST_CHAN_MASK) >> GHWCFG2_NUM_HOST_CHAN_SHIFT);

    hwcfg3 = rd32le(GHWCFG3);
    unit->hu_PacketSizeWidth   =  4 + ((hwcfg3 & GHWCFG3_PACKET_SIZE_CNTR_WIDTH_MASK) >> GHWCFG3_PACKET_SIZE_CNTR_WIDTH_SHIFT);
    unit->hu_TransferSizeWidth = 11 + ((hwcfg3 & GHWCFG3_XFER_SIZE_CNTR_WIDTH_MASK) >> GHWCFG3_XFER_SIZE_CNTR_WIDTH_SHIFT);

    DWC2_BUG("number of channels: %i, pkt size width: %i bits, trans size width %i bits",
        unit->hu_NumChannels, unit->hu_PacketSizeWidth, unit->hu_TransferSizeWidth);

    ahbcfg = rd32le(GAHBCFG);
    ahbcfg |= GAHBCFG_DMA_EN;
    ahbcfg |= (1 << 4);  // wait axi writes (BCM2835 only)?
    ahbcfg &= ~(3 << 1);
    wr32le(GAHBCFG, ahbcfg);

    // Host Negotiation Protocol (HNP) and Session Request Protocol (SRP) not used
    usbcfg = rd32le(GUSBCFG);
    usbcfg &= ~(GUSBCFG_HNPCAP|GUSBCFG_SRPCAP);
    wr32le(GUSBCFG, usbcfg);

    return TRUE;
}

static BOOL init_host(void)
{
    uint32_t hostcfg;
    uint32_t hwcfg2;
    uint32_t hostport;

    // Restart the PHY clock
    wr32le(PCGCCTL, 0);

    hostcfg = rd32le(HCFG);
    hostcfg &= HCFG_FSLSPCLKSEL_MASK;
    hwcfg2 = rd32le(GHWCFG2);
    if (((hwcfg2 & GHWCFG2_HS_PHY_TYPE_MASK) >> GHWCFG2_HS_PHY_TYPE_SHIFT) == GHWCFG2_HS_PHY_TYPE_ULPI
     && ((hwcfg2 & GHWCFG2_FS_PHY_TYPE_MASK) >> GHWCFG2_FS_PHY_TYPE_SHIFT) == GHWCFG2_FS_PHY_TYPE_DEDICATED)
        hostcfg |= HCFG_FSLSPCLKSEL_48_MHZ << HCFG_FSLSPCLKSEL_SHIFT;
    else
        hostcfg |= HCFG_FSLSPCLKSEL_30_60_MHZ << HCFG_FSLSPCLKSEL_SHIFT;
//#if !ENABLE_HIGH_SPEED
#if 1
    hostcfg |= HCFG_FSLSSUPP;
#endif
    wr32le(HCFG, hostcfg);

    flush_tx_fifo();
    flush_rx_fifo();

    dwc2_power_on_port();

    enable_host_interrupts();

    return TRUE;
}

// Turns on the USB controller using the mailbox interface.
// This should be already powered on, but for some reason, I actually have to do
// this for host channel interrupts to work.
static void power_on_usb_controller(void)
{
    ULONG *PwrOnMsg, *pwron = NULL;
    void *MBoxBase = OpenResource("mbox.resource");

    pwron = AllocVec(9*sizeof(ULONG), MEMF_CLEAR);
    PwrOnMsg = (ULONG*)(((IPTR)pwron + 15) & ~15);

    DWC2_BUG("pwron=%p, PwrOnMsg=%p", pwron, PwrOnMsg);

    PwrOnMsg[0] = AROS_LE2LONG(8 * sizeof(ULONG));
    PwrOnMsg[1] = AROS_LE2LONG(VCTAG_REQ);
    PwrOnMsg[2] = AROS_LE2LONG(VCTAG_SETPOWER);
    PwrOnMsg[3] = AROS_LE2LONG(8);
    PwrOnMsg[4] = AROS_LE2LONG(0);
    PwrOnMsg[5] = AROS_LE2LONG(VCPOWER_USBHCD);
    PwrOnMsg[6] = AROS_LE2LONG(VCPOWER_STATE_ON | VCPOWER_STATE_WAIT);
    PwrOnMsg[7] = 0;

    MBoxWrite((void*)VCMB_BASE, VCMB_PROPCHAN, PwrOnMsg);
    if (MBoxRead((void*)VCMB_BASE, VCMB_PROPCHAN) == PwrOnMsg)
    {
        DWC2_BUG("Power on state: %08x", AROS_LE2LONG(PwrOnMsg[6]));
        if ((AROS_LE2LONG(PwrOnMsg[6]) & 1) == 0)
        {
            DWC2_BUG("Failed to power on USB controller");
        }

        if ((AROS_LE2LONG(PwrOnMsg[6]) & 2) != 0)
        {
            DWC2_BUG("USB HCD does not exist\n");
            for (int i=0; i < 8; i++)
                DWC2_BUG("%08x\n", PwrOnMsg[i]);
        }
    }
    FreeVec(pwron);
}

BOOL dwc2_reset_root_port(void)  // TODO: wait timeout
{
    uint32_t hostport;

    while (!(rd32le(HPRT0) & HPRT0_CONNSTS))
        ;

    dwc2_delay(100);

    // reset host port
    hostport = rd32le(HPRT0);
    hostport &= ~HPRT0_WRITE_CLEAR_BITS;
    hostport |= HPRT0_RST;
    wr32le(HPRT0, hostport);

    dwc2_delay(50);

    // come out of reset
    hostport = rd32le(HPRT0);
    hostport &= ~HPRT0_WRITE_CLEAR_BITS;
    hostport &= ~HPRT0_RST;
    wr32le(HPRT0, hostport);

    dwc2_delay(20);

    return TRUE;
}

void dwc2_power_on_port(void)
{
    uint32_t hostport = rd32le(HPRT0);
    hostport &= ~HPRT0_WRITE_CLEAR_BITS;
    if (!(hostport & HPRT0_PWR))
    {
        hostport |= HPRT0_PWR;
        wr32le(HPRT0, hostport);
    }
}

void dwc2_power_off_port(void)
{
    uint32_t hostport = rd32le(HPRT0);
    hostport &= ~HPRT0_WRITE_CLEAR_BITS;
    if (hostport & HPRT0_PWR)
    {
        hostport &= ~HPRT0_PWR;
        wr32le(HPRT0, hostport);
    }
}

static BOOL init_hardware(LIBBASETYPEPTR DWC2Base)
{
    // Create resources
    DWC2Base->hd_MsgPort = CreateMsgPort();
    if (DWC2Base->hd_MsgPort == NULL)
    {
        DWC2_BUG("Failed to create message port");
        return FALSE;
    }
    DWC2Base->hd_TimerReq = CreateIORequest(DWC2Base->hd_MsgPort, sizeof(*DWC2Base->hd_TimerReq));
    if (DWC2Base->hd_TimerReq == NULL)
    {
        DWC2_BUG("Failed to allocate timer IORequest");
        return FALSE;
    }
    if (OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)DWC2Base->hd_TimerReq, 0) != 0)
    {
        DWC2_BUG("Failed to open timer.device");
        return FALSE;
    }
    DWC2Base->hd_TimerReq->tr_node.io_Message.mn_Node.ln_Name = "DWC2 Timer";
    DWC2Base->hd_TimerReq->tr_node.io_Command = TR_ADDREQUEST;
    DWC2_BUG("opened timer.device");
    disable_global_interrupts();
    DWC2Base->hd_UtilityBase = OpenLibrary("utility.library", 39);
    if (DWC2Base->hd_UtilityBase == NULL)
    {
        DWC2_BUG("Failed to open utility.library");
        return FALSE;
    }
    DWC2Base->hd_MemPool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR | MEMF_SEM_PROTECTED, 16384, 4096);
    if (DWC2Base->hd_MemPool == NULL)
    {
        DWC2_BUG("Failed to create memory pool");
        return FALSE;
    }
    DWC2Base->hd_Unit = AllocPooled(DWC2Base->hd_MemPool, sizeof(*DWC2Base->hd_Unit));
    if (DWC2Base->hd_Unit == NULL)
    {
        DWC2_BUG("Failed to allocate unit");
        return FALSE;
    }

    power_on_usb_controller();

    // Install IRQ handler
    KrnAddIRQHandler(IRQ_VC_USB, irq_handler, DWC2Base->hd_Unit, SysBase);

    if (!init_core(DWC2Base->hd_Unit))
    {
        DWC2_BUG("Failed to initialise core");
        return FALSE;
    }

    enable_global_interrupts();

    if (!init_host())
    {
        DWC2_BUG("Failed to initialise host");
        return FALSE;
    }

    if (!dwc2_reset_root_port())
    {
        DWC2_BUG("Failed to reset root port");
        return FALSE;
    }

    DWC2_BUG("initialised hardware");

    return TRUE;
}
