/* uhwcmd.c - pciusb.device by Chris Hodges
*/

#include <devices/usb_hub.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/timer.h>

#include <strings.h>

#include LC_LIBDEFS_FILE

#include "uhwcmd.h"
#include "ohci/ohciproto.h"
#include "uhci/uhciproto.h"
#include "ehci/ehciproto.h"

static inline void pciusbFreePTDChipset(struct PCIController *hc, struct PTDNode *ptd)
{
    if(!ptd || !ptd->ptd_Chipset)
        return;

    switch(hc->hc_HCIType) {
    case HCITYPE_EHCI:
        FreeMem(ptd->ptd_Chipset, sizeof(struct EhciPTDPrivate));
        break;
    case HCITYPE_UHCI:
        FreeMem(ptd->ptd_Chipset, sizeof(struct UhciPTDPrivate));
        break;
    default:
        FreeMem(ptd->ptd_Chipset, sizeof(struct OhciPTDPrivate));
        break;
    }
    ptd->ptd_Chipset = NULL;
}

static inline BOOL uhwIsRootHubIOReq(const struct IOUsbHWReq *ioreq,
                                     const struct PCIUnit *unit)
{
    return (ioreq->iouh_DevAddr == unit->hu_RootHubAddr);
}

#define NewList NEWLIST

/* we cannot use AROS_WORD2LE in struct initializer */
#if AROS_BIG_ENDIAN
#define WORD2LE(w) (UWORD)(((w) >> 8) & 0x00FF) | (((w) << 8) & 0xFF00)
#else
#define WORD2LE(w) (w)
#endif

#if defined(AROS_USE_LOGRES)
#ifdef LogHandle
#undef LogHandle
#endif
#ifdef LogResBase
#undef LogResBase
#endif
#define LogHandle (base->hd_LogRHandle)
#define LogResBase (base->hd_LogResBase)
#endif

/* Root hub device */
const struct UsbStdDevDesc RHDevDesc = {
    sizeof(struct UsbStdDevDesc),
    UDT_DEVICE,
    WORD2LE(0x0110),                            // bcdUSB
    HUB_CLASSCODE,                              // bDeviceClass
    0,                                          // bDeviceSubClass
    0,                                          // bDeviceProtocol
    8,                                          // bMaxPacketSize0
    WORD2LE(0x0000),                            // idVendor
    WORD2LE(0x0000),                            // idProduct
    WORD2LE(0x0100),                            // bcdDevice
    1,                                          // iManufacturer
    2,                                          // iProduct
    0,                                          // iSerialNumber
    1                                           // bNumConfigurations
};

/* Root hub config */
const struct UsbStdCfgDesc RHCfgDesc = {
    sizeof(struct UsbStdCfgDesc),
    UDT_CONFIGURATION,
    WORD2LE(sizeof(struct UsbStdCfgDesc) + sizeof(struct UsbStdIfDesc) + sizeof(struct UsbStdEPDesc)),
    // wTotalLength
    1,                                              // bNumInterfaces
    1,                                              // bConfigurationValue
    3,                                              // iConfiguration
    USCAF_ONE|USCAF_SELF_POWERED,                   // bmAttributes
    0                                               // MaxPower
};

/* Root hub interface */
const struct UsbStdIfDesc  RHIfDesc  = {
    sizeof(struct UsbStdIfDesc),
    UDT_INTERFACE,
    0,                                              // bInterfaceNumber
    0,                                              // bAlternateSetting
    1,                                              // bNumEndpoints
    HUB_CLASSCODE,                                  // bInterfaceClass
    0,                                              // bInterfaceSubClass
    0,                                              // bInterfaceProtocol
    4                                               // iInterface
};

/* Root hub endpoint */
const struct UsbStdEPDesc  RHEPDesc  = {
    sizeof(struct UsbStdEPDesc),
    UDT_ENDPOINT,
    URTF_IN|1,                                      // bEndpointAddress
    USEAF_INTERRUPT,                                // bmAttributes
    WORD2LE(8),                                     // wMaxPacketSize
    255                                             // bInterval
};

/* Root hub descriptors */
const struct UsbHubDesc    RHHubDesc = {
    sizeof(struct UsbHubDesc),                      // 0 Number of bytes in this descriptor, including this byte
    UDT_HUB,                                        // 1 Descriptor Type, value: 29H for hub descriptor
    0,                                              // 2 Number of downstream facing ports that this hub supports
    0,                                              // 3 wHubCharacteristics
    0,                                              // 5 bPwrOn2PwrGood
    1,                                              // 6 bHubContrCurrent
    1,                                              // 7 DeviceRemovable (size is variable)
    0                                               // x PortPwrCtrlMask (size is variable)
};

static const char strStandardConfig[] = "Standard Config";
static const char strHubInterface[] = "Hub interface";
const CONST_STRPTR RHStrings[] = { "Chris Hodges", "PCI Root Hub Unit x", strStandardConfig, strHubInterface };

/* /// "SureCause()" */
void SureCause(struct PCIDevice *base, struct Interrupt *interrupt)
{
#if !defined(__AROS__)
    /* this is a workaround for the original Cause() function missing tailed calls */
    Disable();

    if((interrupt->is_Node.ln_Type == NT_SOFTINT) || (interrupt->is_Node.ln_Type == NT_USER)) {
        // signal tailed call
        interrupt->is_Node.ln_Type = NT_USER;
    } else {
        do {
            interrupt->is_Node.ln_Type = NT_SOFTINT;
            Forbid(); // make sure code is not interrupted by other tasks
            Enable();
            AROS_INTC1(interrupt->is_Code, interrupt->is_Data);
            Disable();
            Permit();
        } while(interrupt->is_Node.ln_Type != NT_SOFTINT);
        interrupt->is_Node.ln_Type = NT_INTERRUPT;
    }
    Enable();
#else
    Cause(interrupt);
#endif
}
/* \\\ */

/* /// "uhwOpenTimer()" */
BOOL uhwOpenTimer(struct PCIUnit *unit, struct PCIDevice *base)
{
    if((unit->hu_MsgPort = CreateMsgPort())) {
        if((unit->hu_TimerReq = (struct timerequest *) CreateIORequest(unit->hu_MsgPort, sizeof(struct timerequest)))) {
            if(!OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *) unit->hu_TimerReq, 0)) {
                unit->hu_TimerReq->tr_node.io_Message.mn_Node.ln_Name = "PCI hardware";
                unit->hu_TimerReq->tr_node.io_Command = TR_ADDREQUEST;
                KPRINTF(1, "opened timer device\n");
                return(TRUE);
            }
            DeleteIORequest((struct IORequest *) unit->hu_TimerReq);
            unit->hu_TimerReq = NULL;
        }
        DeleteMsgPort(unit->hu_MsgPort);
        unit->hu_MsgPort = NULL;
    }
    KPRINTF(5, "failed to open timer.device\n");
    return(FALSE);
}
/* \\\ */

/* /// "uhwDelayMS()" */
void uhwDelayMS(ULONG milli, struct timerequest *timerreq)
{
    timerreq->tr_time.tv_secs  = 0;
    timerreq->tr_time.tv_micro = milli * 1000;
    DoIO((struct IORequest *) timerreq);
}
/* \\\ */

/* /// "uhwDelayMicro()" */
void uhwDelayMicro(ULONG micro, struct timerequest *timerreq)
{
    timerreq->tr_time.tv_secs  = 0;
    timerreq->tr_time.tv_micro = micro;
    DoIO((struct IORequest *) timerreq);
}
/* \\\ */

/* /// "uhwCloseTimer()" */
void uhwCloseTimer(struct PCIUnit *unit, struct PCIDevice *base)
{
    if(unit->hu_MsgPort) {
        if(unit->hu_TimerReq) {
            KPRINTF(1, "closing timer.device\n");
            CloseDevice((APTR) unit->hu_TimerReq);
            DeleteIORequest((struct IORequest *) unit->hu_TimerReq);
            unit->hu_TimerReq = NULL;
        }
        DeleteMsgPort(unit->hu_MsgPort);
        unit->hu_MsgPort = NULL;
    }
}
/* \\\ */

/* /// "Open_Unit()" */
struct Unit * Open_Unit(struct IOUsbHWReq *ioreq,
                        LONG unitnr,
                        struct PCIDevice *base)
{
    struct PCIUnit *unit = NULL;

    pciusbDebug("UHW", DEBUGCOLOR_SET "%s(0x%p, %u)" DEBUGCOLOR_RESET "\n", __func__, ioreq, unitnr);

    if(!base->hd_ScanDone) {
        base->hd_ScanDone = TRUE;
        if(!pciInit(base)) {
            return NULL;
        }
    }
    unit = (struct PCIUnit *) base->hd_Units.lh_Head;
    while(((struct Node *) unit)->ln_Succ) {
        if((unit->hu_UnitNo & ~PCIUSBUNIT_MASK) == unitnr) {
            break;
        }
        unit = (struct PCIUnit *) ((struct Node *) unit)->ln_Succ;
    }
    if(!((struct Node *) unit)->ln_Succ) {
        KPRINTF(20, "Unit %ld does not exist!\n", unitnr);
        return NULL;
    }
    if(unit->hu_UnitNo & PCIUSBUNIT_MASK) {
        ioreq->iouh_Req.io_Error = IOERR_UNITBUSY;
        KPRINTF(5, "Unit %ld already open!\n", unitnr);
        return NULL;
    }

    if(uhwOpenTimer(unit, base)) {

        if(pciAllocUnit(unit)) { // hardware self test
            unit->hu_UnitNo |= PCIUSBUNIT_MASK;                                                 // Mark the unit as allocated/opened

            unit->hu_NakTimeoutInt.is_Node.ln_Type = NT_INTERRUPT;
            unit->hu_NakTimeoutInt.is_Node.ln_Name = "PCIUSB NakTimeout";
            unit->hu_NakTimeoutInt.is_Node.ln_Pri  = -16;
            unit->hu_NakTimeoutInt.is_Data = unit;
            unit->hu_NakTimeoutInt.is_Code = (VOID_FUNC)uhwNakTimeoutInt;

            CopyMem(unit->hu_TimerReq, &unit->hu_NakTimeoutReq, sizeof(struct timerequest));
            memset( &unit->hu_NakTimeoutMsgPort, 0, sizeof( unit->hu_NakTimeoutMsgPort ) );
            unit->hu_NakTimeoutMsgPort.mp_Node.ln_Type = NT_MSGPORT;
            unit->hu_NakTimeoutMsgPort.mp_Flags = PA_SOFTINT;
            unit->hu_NakTimeoutMsgPort.mp_SigTask = &unit->hu_NakTimeoutInt;
            NewList(&unit->hu_NakTimeoutMsgPort.mp_MsgList);
            unit->hu_NakTimeoutReq.tr_node.io_Message.mn_ReplyPort = &unit->hu_NakTimeoutMsgPort;
            Cause(&unit->hu_NakTimeoutInt);

            return(&unit->hu_Unit);
        } else {
            ioreq->iouh_Req.io_Error = IOERR_SELFTEST;
            KPRINTF(20, "Hardware allocation failure!\n");
        }
        uhwCloseTimer(unit, base);
    }
    return(NULL);
}
/* \\\ */

struct RTIsoNode *pciusbAllocStdIsoNode(struct PCIController *hc, struct IOUsbHWReq *ioreq)
{
#if defined(AROS_USE_LOGRES)
    struct PCIDevice *base = hc->hc_Device;
#endif

    struct RTIsoNode *rtn;
    UWORD ptdcount;

    pciusbDebug("UHW", DEBUGCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET "\n", __func__, hc, ioreq);

    if (!ioreq)
        return NULL;

    rtn = AllocMem(sizeof(*rtn), MEMF_CLEAR);
    if (!rtn)
        return NULL;

    ptdcount = hc->hc_IsoPTDCount ? hc->hc_IsoPTDCount : 2;
    if (ptdcount < 2)
        ptdcount = 2;

    rtn->rtn_PTDCount = ptdcount;
    rtn->rtn_PTDs = AllocMem(sizeof(*rtn->rtn_PTDs) * ptdcount, MEMF_CLEAR);
    if (!rtn->rtn_PTDs) {
        FreeMem(rtn, sizeof(*rtn));
        return NULL;
    }

    rtn->rtn_StdReq = ioreq;
    CopyMem(ioreq, &rtn->rtn_IOReq, sizeof(*ioreq));
    rtn->rtn_BufferReq.ubr_Buffer = ioreq->iouh_Data;
    rtn->rtn_BufferReq.ubr_Length = ioreq->iouh_Length;
    rtn->rtn_BufferReq.ubr_Frame = ioreq->iouh_Frame;
    rtn->rtn_BufferReq.ubr_Flags = 0;
    rtn->rtn_NextFrame = ioreq->iouh_Frame;

    return rtn;
}

void pciusbFreeStdIsoNode(struct PCIController *hc, struct RTIsoNode *rtn)
{
#if defined(AROS_USE_LOGRES)
    struct PCIDevice *base = hc->hc_Device;
#else
    (void)hc;
#endif

    pciusbDebug("UHW", DEBUGCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET "\n", __func__, hc, rtn);

    if (!rtn)
        return;

    if (rtn->rtn_PTDs) {
        FreeMem(rtn->rtn_PTDs, sizeof(*rtn->rtn_PTDs) * rtn->rtn_PTDCount);
        rtn->rtn_PTDs = NULL;
    }

    FreeMem(rtn, sizeof(*rtn));
}

/* /// "Close_Unit()" */
void Close_Unit(struct PCIDevice *base,
                struct PCIUnit *unit,
                struct IOUsbHWReq *ioreq)
{
    /* Disable all interrupts */
    unit->hu_NakTimeoutMsgPort.mp_Flags = PA_IGNORE;
#if !defined(__AROS__)
    unit->hu_NakTimeoutInt.is_Node.ln_Type = NT_SOFTINT;
#endif
    AbortIO((APTR) &unit->hu_NakTimeoutReq);

    pciFreeUnit(unit);

    uhwCloseTimer(unit, base);
    unit->hu_UnitNo &= ~PCIUSBUNIT_MASK;                                             // Mark as de-allocated
}
/* \\\ */

/* /// "uhwGetUsbState()" */
UWORD uhwGetUsbState(struct IOUsbHWReq *ioreq,
                     struct PCIUnit *unit,
                     struct PCIDevice *base)
{
    return(ioreq->iouh_State = UHSF_OPERATIONAL);
}
/* \\\ */

/* /// "cmdReset()" */
/*
 *======================================================================
 * cmdReset(ioreq, unit, base)
 *======================================================================
 *
 * This is the device CMD_RESET routine.
 *
 * Resets the whole USB hardware. Goes into USBOperational mode right
 * after. Must NOT be called from an interrupt.
 *
 */

WORD cmdReset(struct IOUsbHWReq *ioreq,
              struct PCIUnit *unit,
              struct PCIDevice *base)
{
    pciusbDebug("UHW", DEBUGCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET "\n", __func__, ioreq, unit);

    uhwDelayMS(1, unit->hu_TimerReq);
    uhwGetUsbState(ioreq, unit, base);

    if(ioreq->iouh_State & UHSF_OPERATIONAL) {
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}
/* \\\ */

/* /// "cmdUsbReset()" */
/*
 *======================================================================
 * cmdUsbReset(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_USBRESET routine.
 *
 * Resets the USB bus. Goes into USBOperational mode right after. Must
 * NOT be called from an interrupt.
 *
 */

WORD cmdUsbReset(struct IOUsbHWReq *ioreq,
                 struct PCIUnit *unit,
                 struct PCIDevice *base)
{
    pciusbDebug("UHW", DEBUGCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET "\n", __func__, ioreq, unit);

    /* FIXME */
    uhwGetUsbState(ioreq, unit, base);

    unit->hu_FrameCounter = 1;
    unit->hu_RootHubAddr = 0;

    if(ioreq->iouh_State & UHSF_OPERATIONAL) {
        /* for poseidon 5/UHA_DriverVersion 0x300 we need to return the root port speed after UHCMD_USBRESET */
        ioreq->iouh_Flags &= ~(UHFF_SUPERSPEED | UHFF_HIGHSPEED | UHFF_LOWSPEED);
        if (unit->hu_RootHub20Ports > 0)
            ioreq->iouh_Flags |= UHFF_HIGHSPEED;
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}
/* \\\ */

/* /// "cmdUsbResume()" */
/*
 *======================================================================
 * cmdUsbResume(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_USBRESUME routine.
 *
 * Tries to resume from USBSuspend mode into USBOperational.
 * Must NOT be called from an interrupt.
 *
 */

WORD cmdUsbResume(struct IOUsbHWReq *ioreq,
                  struct PCIUnit *unit,
                  struct PCIDevice *base)
{
    pciusbDebug("UHW", DEBUGCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET "\n", __func__, ioreq, unit);


    /* FIXME */
    uhwGetUsbState(ioreq, unit, base);
    if(ioreq->iouh_State & UHSF_OPERATIONAL) {
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}
/* \\\ */

/* /// "cmdUsbSuspend()" */
/*
 *======================================================================
 * cmdUsbSuspend(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_USBSUSPEND routine.
 *
 * Sets the USB into USBSuspend mode.
 * Must NOT be called from an interrupt.
 *
 */

WORD cmdUsbSuspend(struct IOUsbHWReq *ioreq,
                   struct PCIUnit *unit,
                   struct PCIDevice *base)
{
    pciusbDebug("UHW", DEBUGCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET "\n", __func__, ioreq, unit);

    /* FIXME */
    uhwGetUsbState(ioreq, unit, base);
    if(ioreq->iouh_State & UHSF_SUSPENDED) {
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}
/* \\\ */

/* /// "cmdUsbOper()" */
/*
 *======================================================================
 * cmdUsbOper(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_USBOPER routine.
 *
 * Sets the USB into USBOperational mode.
 * Must NOT be called from an interrupt.
 *
 */

WORD cmdUsbOper(struct IOUsbHWReq *ioreq,
                struct PCIUnit *unit,
                struct PCIDevice *base)
{
    pciusbDebug("UHW", DEBUGCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET "\n", __func__, ioreq, unit);

    /* FIXME */
    uhwGetUsbState(ioreq, unit, base);
    if(ioreq->iouh_State & UHSF_OPERATIONAL) {
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}
/* \\\ */

/* /// "cmdQueryDevice()" */
/*
 *======================================================================
 * cmdQueryDevice(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_QUERYDEVICE routine.
 *
 * Returns information about the hardware.
 *
 */

WORD cmdQueryDevice(struct IOUsbHWReq *ioreq,
                    struct PCIUnit *unit,
                    struct PCIDevice *base)
{
    struct TagItem *taglist = (struct TagItem *) ioreq->iouh_Data;
    struct TagItem *tag;
    ULONG count = 0;

    pciusbDebug("UHW", DEBUGCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET "\n", __func__, ioreq, unit);

    if((tag = FindTagItem(UHA_State, taglist))) {
        *((ULONG *) tag->ti_Data) = (ULONG) uhwGetUsbState(ioreq, unit, base);
        count++;
    }
    if((tag = FindTagItem(UHA_Manufacturer, taglist))) {
        *((STRPTR *) tag->ti_Data) = "Chris Hodges";
        count++;
    }
    if((tag = FindTagItem(UHA_ProductName, taglist))) {
        *((STRPTR *) tag->ti_Data) = unit->hu_ProductName;
        count++;
    }
    if((tag = FindTagItem(UHA_Description, taglist))) {
        *((STRPTR *) tag->ti_Data) = "Generic adaptive host controller driver for PCI cards";
        count++;
    }
    if((tag = FindTagItem(UHA_Copyright, taglist))) {
        *((STRPTR *) tag->ti_Data) ="\xA9""2007-2009 Chris Hodges";
        count++;
    }
    if((tag = FindTagItem(UHA_Version, taglist))) {
        *((ULONG *) tag->ti_Data) = VERSION_NUMBER;
        count++;
    }
    if((tag = FindTagItem(UHA_Revision, taglist))) {
        *((ULONG *) tag->ti_Data) = REVISION_NUMBER;
        count++;
    }
    if((tag = FindTagItem(UHA_DriverVersion, taglist))) {
        *((ULONG *) tag->ti_Data) = 0x300;
        count++;
    }
    if((tag = FindTagItem(UHA_Capabilities, taglist))) {
        ULONG caps = 0;
        if (unit->hu_RootHub20Ports > 0)
            caps |= UHCF_USB20;
#if defined(PCIUSB_WIP_ISO)
        caps |= UHCF_ISO|UHCF_RT_ISO;
#endif
#if defined(PCIUSB_QUICKIO)
        caps |= UHCF_QUICKIO;
#endif
        *((ULONG *) tag->ti_Data) = caps;
        count++;
    }
    ioreq->iouh_Actual = count;
    return RC_OK;
}
/* \\\ */

/* /// "cmdControlXFerRootHub()" */
WORD cmdControlXFerRootHub(struct IOUsbHWReq *ioreq,
                           struct PCIUnit *unit,
                           struct PCIDevice *base)
{
    struct PCIController *hc;
    struct PCIController *chc;
    UWORD rt = ioreq->iouh_SetupData.bmRequestType;
    UWORD req = ioreq->iouh_SetupData.bRequest;
    UWORD idx = AROS_WORD2LE(ioreq->iouh_SetupData.wIndex);
    UWORD val = AROS_WORD2LE(ioreq->iouh_SetupData.wValue);
    UWORD len = AROS_WORD2LE(ioreq->iouh_SetupData.wLength);
    UWORD hciport;
    ULONG numports = unit->hu_RootHubPorts;

    pciusbDebug("UHW", DEBUGCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET "\n", __func__, ioreq, unit);

    if(ioreq->iouh_Endpoint) {
        return(UHIOERR_STALL);
    }

    if(ioreq->iouh_Length < len) {
        pciusbRHDebug("RH", "Len (%lu > %lu) mismatch!\n",
                      (ULONG)len, (ULONG)ioreq->iouh_Length);
        return(UHIOERR_STALL);
    }
    switch(rt) {
    case (URTF_STANDARD|URTF_DEVICE):
        switch(req) {
        case USR_SET_ADDRESS:
            pciusbRHDebug("RH", "SetAddress = %ld\n", val);
            unit->hu_RootHubAddr = val;
            ioreq->iouh_Actual = len;
            return(0);

        case USR_SET_CONFIGURATION:
            pciusbRHDebug("RH", "SetConfiguration=%ld\n", val);
            ioreq->iouh_Actual = len;
            return(0);
        }
        break;

    case (URTF_IN|URTF_STANDARD|URTF_DEVICE):
       switch(req) {
        case USR_GET_STATUS:
            {
                /*
                 * USB device status (2 bytes):
                 * bit0 = self-powered, bit1 = remote-wakeup.
                 * Keep this consistent with the root hub config descriptor.
                 */
                if (len < 2) {
                    return(UHIOERR_STALL);
                }

                UWORD st = 0;
                if (RHCfgDesc.bmAttributes & USCAF_SELF_POWERED)
                    st |= 0x0001;
                if (RHCfgDesc.bmAttributes & USCAF_REMOTE_WAKEUP)
                    st |= 0x0002;

                st = AROS_WORD2LE(st);
                CopyMem(&st, ioreq->iouh_Data, 2);
                ioreq->iouh_Actual = 2;
                return(0);
            }
        case USR_GET_DESCRIPTOR:
            switch(val>>8) {
            case UDT_DEVICE:
                pciusbRHDebug("RH", "GetDeviceDescriptor (%ld)\n", len);
                ioreq->iouh_Actual = (len > sizeof(struct UsbStdDevDesc)) ? sizeof(struct UsbStdDevDesc) : len;
                CopyMem((APTR) &RHDevDesc, ioreq->iouh_Data, ioreq->iouh_Actual);
                if(ioreq->iouh_Length >= sizeof(struct UsbStdDevDesc)) {
                    struct UsbStdDevDesc *usdd = (struct UsbStdDevDesc *) ioreq->iouh_Data;
                    hc = (struct PCIController *) unit->hu_Controllers.lh_Head;
                    usdd->idVendor = WORD2LE(hc->hc_VendID);
                    usdd->idProduct = WORD2LE(hc->hc_ProdID);

                    if(unit->hu_RootHub20Ports) {
                        pciusbRHDebug("RH", "USB2 Hub Descriptor\n");
                        usdd->bcdUSB = AROS_WORD2LE(0x0200); // signal a highspeed root hub
                        usdd->bDeviceProtocol = 1; // single TT
                        usdd->bMaxPacketSize0 = 64;
                    }
                }
                return(0);

            case UDT_CONFIGURATION: {
                UBYTE tmpbuf[sizeof(struct UsbStdCfgDesc) + sizeof(struct UsbStdIfDesc) + sizeof(struct UsbStdEPDesc)];
                pciusbRHDebug("RH", "GetConfigDescriptor (%ld)\n", len);
                CopyMem((APTR) &RHCfgDesc,
                        tmpbuf,
                        sizeof(struct UsbStdCfgDesc));
                CopyMem((APTR) &RHIfDesc,
                        &tmpbuf[sizeof(struct UsbStdCfgDesc)],
                        sizeof(struct UsbStdIfDesc));
                CopyMem((APTR) &RHEPDesc,
                        &tmpbuf[sizeof(struct UsbStdCfgDesc) + sizeof(struct UsbStdIfDesc)],
                        sizeof(struct UsbStdEPDesc));
                if(unit->hu_RootHub20Ports) {
                    struct UsbStdIfDesc *usid = (struct UsbStdIfDesc *) &tmpbuf[sizeof(struct UsbStdCfgDesc)];
                    struct UsbStdEPDesc *usepd = (struct UsbStdEPDesc *) &tmpbuf[sizeof(struct UsbStdCfgDesc) + sizeof(struct UsbStdIfDesc)];
                    usid->bInterfaceProtocol = 1; // single TT for high-speed root hub
                    usepd->bInterval = 12; // 2048 microframes
                    usepd->wMaxPacketSize = AROS_WORD2LE(64);
                }
                ioreq->iouh_Actual = (len > sizeof(struct UsbStdCfgDesc) + sizeof(struct UsbStdIfDesc) + sizeof(struct UsbStdEPDesc)) ? (sizeof(struct UsbStdCfgDesc) + sizeof(struct UsbStdIfDesc) + sizeof(struct UsbStdEPDesc)) : len;
                CopyMem(tmpbuf, ioreq->iouh_Data, ioreq->iouh_Actual);
                return(0);
            }

            case UDT_STRING:
                if(val & 0xff) { /* get lang array */
                    CONST_STRPTR source = NULL, rhstring;
                    UWORD *mptr = ioreq->iouh_Data;
                    UWORD slen = 1;
                    pciusbRHDebug("RH", "GetString %04lx (%ld)\n", val, len);
                    if((val & 0xff) > 4) { /* index too high? */
                        return(UHIOERR_STALL);
                    }
                    rhstring = RHStrings[(val & 0xff)-1];
                    source = rhstring;
                    if(len > 1) {
                        ioreq->iouh_Actual = 2;
                        while(*source++) {
                            slen++;
                        }
                        source = rhstring;
                        *mptr++ = AROS_WORD2BE((slen<<9)|UDT_STRING);
                        while(ioreq->iouh_Actual+1 < len) {
                            // special hack for unit number in root hub string
                            if(((val & 0xff) == 2) && (source[1] == 0)) {
                                *mptr++ = AROS_WORD2LE('0' + (unit->hu_UnitNo & ~PCIUSBUNIT_MASK));
                            } else {
                                *mptr++ = AROS_WORD2LE(*source);
                            }
                            source++;
                            ioreq->iouh_Actual += 2;
                            if(!(*source)) {
                                break;
                            }
                        }
                    }
                } else {
                    UWORD *mptr = ioreq->iouh_Data;
                    pciusbRHDebug("RH", "GetLangArray %04lx (%ld)\n", val, len);
                    if(len > 1) {
                        ioreq->iouh_Actual = 2;
                        mptr[0] = AROS_WORD2BE((4<<8)|UDT_STRING);
                        if(len > 3) {
                            ioreq->iouh_Actual += 2;
                            mptr[1] = AROS_WORD2LE(0x0409);
                        }
                    }
                }
                return(0);

            default:
                pciusbRHDebug("RH", "Unsupported Descriptor %04lx\n", idx);
            }
            break;

        case USR_GET_CONFIGURATION:
            if(len == 1) {
                pciusbRHDebug("RH", "GetConfiguration\n");
                ((UBYTE *) ioreq->iouh_Data)[0] = 1;
                ioreq->iouh_Actual = len;
                return(0);
            }
            break;
        }
        break;

    case (URTF_CLASS|URTF_OTHER):
        switch(req) {
        case USR_SET_FEATURE: {
            WORD retval = 0;
            if((!idx) || (idx > numports)) {
                pciusbRHDebug("RH", "Port %ld out of range\n", idx);
                return(UHIOERR_STALL);
            }
            chc = unit->hu_PortMap11[idx - 1];
            if(unit->hu_PortOwner[idx - 1] == HCITYPE_EHCI) {
                hc = unit->hu_PortMap20[idx - 1];
                hciport = unit->hu_PortNum20[idx - 1];
            } else {
                hc = chc;
                hciport = unit->hu_PortNum11[idx - 1];
            }
            pciusbRHDebug("RH", "Set Feature %ld maps from glob. Port %ld to local Port %ld (%s)\n", val, idx, hciport,
                    (unit->hu_PortOwner[idx - 1] == HCITYPE_EHCI) ? "EHCI" : "U/OHCI");
            if (hc) {
                switch(hc->hc_HCIType) {
                case HCITYPE_UHCI: {
                    if (uhciSetFeature(unit, hc, hciport, idx, val, &retval)) {
                        pciusbRHDebug("RH", "uhciSetFeature returned (retval %04x)\n", retval);
                        return(retval);
                    }
                }
                break;

                case HCITYPE_OHCI: {
                    if (ohciSetFeature(unit, hc, hciport, idx, val, &retval)) {
                        pciusbRHDebug("RH", "ohciSetFeature returned (retval %04x)\n", retval);
                        return(retval);
                    }
                }
                break;

                case HCITYPE_EHCI: {
                    if (ehciSetFeature(unit, hc, hciport, idx, val, &retval)) {
                        pciusbRHDebug("RH", "ehciSetFeature returned (retval %04x)\n", retval);
                        return(retval);
                    }
                }
                break;
                }
            } else {
                pciusbWarn("RH", "port %ld controller not set\n", idx);
                return(UHIOERR_STALL);
            }
        }
        break;

        case USR_CLEAR_FEATURE: {
            WORD retval = 0;
            if((!idx) || (idx > numports)) {
                pciusbRHDebug("RH", "Port %ld out of range\n", idx);
                return(UHIOERR_STALL);
            }
            if(unit->hu_PortOwner[idx - 1] == HCITYPE_EHCI) {
                hc = unit->hu_PortMap20[idx - 1];
                hciport = unit->hu_PortNum20[idx - 1];
            } else {
                hc = unit->hu_PortMap11[idx - 1];
                hciport = unit->hu_PortNum11[idx - 1];
            }
            pciusbRHDebug("RH", "Clear Feature %ld maps from glob. Port %ld to local Port %ld (%s)\n", val, idx, hciport,
                    (unit->hu_PortOwner[idx - 1] == HCITYPE_EHCI) ? "EHCI" : "U/OHCI");
            if (hc) {
                switch(hc->hc_HCIType) {
                case HCITYPE_UHCI: {
                    if (uhciClearFeature(unit, hc, hciport, idx, val, &retval)) {
                        pciusbRHDebug("RH", "uhciClearFeature returned (retval %04x)\n", retval);
                        return(retval);
                    }
                }
                break;

                case HCITYPE_OHCI: {
                    if (ohciClearFeature(unit, hc, hciport, idx, val, &retval)) {
                        pciusbRHDebug("RH", "ohciClearFeature returned (retval %04x)\n", retval);
                        return(retval);
                    }
                }
                break;

                case HCITYPE_EHCI: {
                    if (ehciClearFeature(unit, hc, hciport, idx, val, &retval)) {
                        pciusbRHDebug("RH", "ehciClearFeature returned (retval %04x)\n", retval);
                        return(retval);
                    }
                }
                break;
                }
            } else {
                pciusbWarn("RH", "port %ld controller not set\n", idx);
                return(UHIOERR_STALL);
            }
        }
        break;
        }
        break;

    case (URTF_IN|URTF_CLASS|URTF_OTHER):
        switch(req) {
        case USR_GET_STATUS: {
            UWORD *mptr = ioreq->iouh_Data;
            WORD retval = 0;

            if(len != sizeof(struct UsbPortStatus)) {
                return(UHIOERR_STALL);
            }
            if((!idx) || (idx > numports)) {
                pciusbRHDebug("RH", "Port %ld out of range\n", idx);
                return(UHIOERR_STALL);
            }
            if(unit->hu_PortOwner[idx - 1] == HCITYPE_EHCI) {
                hc = unit->hu_PortMap20[idx - 1];
                hciport = unit->hu_PortNum20[idx - 1];
            } else {
                hc = unit->hu_PortMap11[idx - 1];
                hciport = unit->hu_PortNum11[idx - 1];
            }
            if (hc) {
                switch(hc->hc_HCIType) {
                case HCITYPE_UHCI: {
                    if (uhciGetStatus(hc, mptr, hciport, idx, &retval)) {
                        pciusbRHDebug("RH", "uhciGetStatus returned (retval %04x)\n", retval);
                        return(retval);
                    }
                }
                break;

                case HCITYPE_OHCI: {
                    if (ohciGetStatus(hc, mptr, hciport, idx, &retval)) {
                        pciusbRHDebug("RH", "ohciGetStatus returned (retval %04x)\n", retval);
                        return(retval);
                    }
                }
                break;

                case HCITYPE_EHCI: {
                    if (ehciGetStatus(hc, mptr, hciport, idx, &retval)) {
                        pciusbRHDebug("RH", "ehciGetStatus returned (retval %04x)\n", retval);
                        return(retval);
                    }
                }
                break;
                }
            } else {
                pciusbWarn("RH", "port %ld controller not set\n", idx);
                return(UHIOERR_STALL);
            }
            return(0);
        }

        }
        break;

    case (URTF_IN|URTF_CLASS|URTF_DEVICE):
        switch(req) {
        case USR_GET_STATUS: {
            UWORD *mptr = ioreq->iouh_Data;

            pciusbRHDebug("RH", "GetHubStatus (%ld)\n", len);

            if(len < sizeof(struct UsbHubStatus)) {
                return(UHIOERR_STALL);
            }
            *mptr++ = 0;
            *mptr++ = 0;
            ioreq->iouh_Actual = 4;
            return(0);
        }

        case USR_GET_DESCRIPTOR:
            switch(val>>8) {
            case UDT_HUB: {
                UWORD hubdesclen = 9;
                UWORD characteristics = UHCF_INDIVID_OVP;
                UBYTE powergood = 1;

                struct UsbHubDesc *uhd = (struct UsbHubDesc *) ioreq->iouh_Data;
                pciusbRHDebug("RH", "GetHubDescriptor (%ld)\n", len);

                if(unit->hu_RootHubPorts > 7) { // needs two bytes for port masks
                    hubdesclen += 2;
                }

                ioreq->iouh_Actual = (len > hubdesclen) ? hubdesclen : len;
                CopyMem((APTR) &RHHubDesc, ioreq->iouh_Data, ioreq->iouh_Actual);

                if(ioreq->iouh_Length) {
                    uhd->bLength = hubdesclen;
                }

                if(ioreq->iouh_Length >= 5) {
                    hc = (struct PCIController *) unit->hu_Controllers.lh_Head;
                    while(hc->hc_Node.ln_Succ) {
                        if(hc->hc_HCIType == HCITYPE_OHCI) {
                            ULONG localpwgood = (READREG32_LE(hc->hc_RegBase, OHCI_HUBDESCA) & OHAM_POWERGOOD) >> OHAS_POWERGOOD;
                            characteristics |= UHCF_INDIVID_POWER;
                            if(localpwgood > powergood)
                                powergood = localpwgood;
                        } else {
                            characteristics |= UHCF_INDIVID_POWER;
                        }

                        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
                    }
                    uhd->wHubCharacteristics = WORD2LE(characteristics);

                    if(ioreq->iouh_Length >= 6) {
                        if (powergood > 1) {
                            pciusbRHDebug("RH", "Increasing power good time to %ld\n", powergood);
                        }
                        uhd->bPwrOn2PwrGood = powergood;
                    }
                }

                if(ioreq->iouh_Length >= hubdesclen) {
                    uhd->bNbrPorts = unit->hu_RootHubPorts;
                    if(hubdesclen == 9) {
                        uhd->DeviceRemovable = 0;
                        uhd->PortPwrCtrlMask = (1<<(unit->hu_RootHubPorts+2))-2;
                    } else {
                        // each field is now 16 bits wide
                        uhd->DeviceRemovable = 0;
                        uhd->PortPwrCtrlMask = 0;
                        ((UBYTE *) ioreq->iouh_Data)[9] = (1<<(unit->hu_RootHubPorts+2))-2;
                        ((UBYTE *) ioreq->iouh_Data)[10] = ((1<<(unit->hu_RootHubPorts+2))-2)>>8;
                    }
                }
                return(0);
            }

            default:
                pciusbRHDebug("RH", "Unsupported Descriptor %04lx\n", idx);
            }
            break;
        }

    }
    KPRINTF(20, DEBUGWARNCOLOR_SET "RH: Unsupported command %02lx %02lx %04lx %04lx %04lx!" DEBUGCOLOR_RESET "\n", rt, req, idx, val, len);
    return(UHIOERR_STALL);
}
/* \\\ */

/* /// "cmdIntXFerRootHub()" */
WORD cmdIntXFerRootHub(struct IOUsbHWReq *ioreq,
                       struct PCIUnit *unit,
                       struct PCIDevice *base)
{
    pciusbDebug("UHW", DEBUGCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET "\n", __func__, ioreq, unit);



    if((ioreq->iouh_Endpoint != 1) || (!ioreq->iouh_Length)) {
        return(UHIOERR_STALL);
    }

    if(unit->hu_RootPortChanges) {
        KPRINTF(1, "Immediate Portchange map %04lx\n", unit->hu_RootPortChanges);
        if((unit->hu_RootHubPorts < 8) || (ioreq->iouh_Length == 1)) {
            *((UBYTE *) ioreq->iouh_Data) = unit->hu_RootPortChanges;
            ioreq->iouh_Actual = 1;
        } else {
            ((UBYTE *) ioreq->iouh_Data)[0] = unit->hu_RootPortChanges;
            ((UBYTE *) ioreq->iouh_Data)[1] = unit->hu_RootPortChanges>>8;
            ioreq->iouh_Actual = 2;
        }
        unit->hu_RootPortChanges = 0;
        return(0);
    }
    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    Disable();
    AddTail(&unit->hu_RHIOQueue, (struct Node *) ioreq);
    Enable();
    return(RC_DONTREPLY);
}
/* \\\ */

/* /// "cmdControlXFer()" */
/*
 *======================================================================
 * cmdControlXFer(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_CONTROLXFER routine.
 *
 * First it check if the usb is in proper state and if user passed arguments
 * are valid. If everything is ok, the request is linked to queue of
 * pending transfer requests.
 *
 */

WORD cmdControlXFer(struct IOUsbHWReq *ioreq,
                    struct PCIUnit *unit,
                    struct PCIDevice *base)
{
    struct PCIController *hc;

    pciusbDebug("UHW", DEBUGCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET "\n", __func__, ioreq, unit);


    uhwGetUsbState(ioreq, unit, base);
    if(!(ioreq->iouh_State & UHSF_OPERATIONAL)) {
        return(UHIOERR_USBOFFLINE);
    }
    /* Root hub emulation */
    if(uhwIsRootHubIOReq(ioreq, unit)) {
        return(cmdControlXFerRootHub(ioreq, unit, base));
    }

    hc = unit->hu_DevControllers[ioreq->iouh_DevAddr];
    if(!hc) {
        KPRINTF(20, "No Host controller assigned to device address %ld\n", ioreq->iouh_DevAddr);
        return(UHIOERR_HOSTERROR);
    }

    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    ioreq->iouh_Actual = 0;

    Disable();
    AddTail(&hc->hc_CtrlXFerQueue, (struct Node *) ioreq);
    Enable();
    SureCause(base, &hc->hc_CompleteInt);

    KPRINTF(10, "UHCMD_CONTROLXFER processed ioreq: 0x%p\n", ioreq);
    return(RC_DONTREPLY);
}
/* \\\ */

/* /// "cmdBulkXFer()" */
/*
 *======================================================================
 * cmdBulkXFer(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_BULKXFER routine.
 *
 * First it check if the usb is in proper state and if user passed arguments
 * are valid. If everything is ok, the request is linked to queue of
 * pending transfer requests.
 *
 */

WORD cmdBulkXFer(struct IOUsbHWReq *ioreq,
                 struct PCIUnit *unit,
                 struct PCIDevice *base)
{
    struct PCIController *hc;

    pciusbDebug("UHW", DEBUGCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET "\n", __func__, ioreq, unit);


    uhwGetUsbState(ioreq, unit, base);
    if(!(ioreq->iouh_State & UHSF_OPERATIONAL)) {
        return(UHIOERR_USBOFFLINE);
    }

    if(ioreq->iouh_Flags & UHFF_LOWSPEED) {
        return(UHIOERR_BADPARAMS);
    }

    hc = unit->hu_DevControllers[ioreq->iouh_DevAddr];
    if(!hc) {
        KPRINTF(20, "No Host controller assigned to device address %ld\n", ioreq->iouh_DevAddr);
        return(UHIOERR_HOSTERROR);
    }

    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    ioreq->iouh_Actual = 0;

    Disable();
    AddTail(&hc->hc_BulkXFerQueue, (struct Node *) ioreq);
    Enable();
    SureCause(base, &hc->hc_CompleteInt);

    KPRINTF(10, "UHCMD_BULKXFER processed ioreq: 0x%p\n", ioreq);
    return(RC_DONTREPLY);
}
/* \\\ */

/* /// "cmdIsoXFer()" */
/*
 *======================================================================
 * cmdIsoXFer(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_ISOXFER routine.
 *
 * First it check if the usb is in proper state and if user passed arguments
 * are valid. If everything is ok, the request is linked to queue of
 * pending transfer requests.
 *
 */

WORD cmdIsoXFer(struct IOUsbHWReq *ioreq,
                struct PCIUnit *unit,
                struct PCIDevice *base)
{
    struct PCIController *hc;

    pciusbDebug("UHW", DEBUGCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET "\n", __func__, ioreq, unit);


    uhwGetUsbState(ioreq, unit, base);
    if(!(ioreq->iouh_State & UHSF_OPERATIONAL)) {
        return(UHIOERR_USBOFFLINE);
    }

    if(ioreq->iouh_Flags & UHFF_LOWSPEED) {
        return(UHIOERR_BADPARAMS);
    }

    hc = unit->hu_DevControllers[ioreq->iouh_DevAddr];
    if(!hc) {
        KPRINTF(20, "No Host controller assigned to device address %ld\n", ioreq->iouh_DevAddr);
        return(UHIOERR_HOSTERROR);
    }

    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    ioreq->iouh_Actual = 0;

    Disable();
    AddTail(&hc->hc_IsoXFerQueue, (struct Node *) ioreq);
    Enable();
    SureCause(base, &hc->hc_CompleteInt);


    
    KPRINTF(10, "UHCMD_ISOXFER processed ioreq: 0x%p\n", ioreq);
    return(RC_DONTREPLY);
}
/* \\\ */

/* /// "cmdIntXFer()" */
/*
 *======================================================================
 * cmdIntXFer(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_INTXFER routine.
 *
 * First it check if the usb is in proper state and if user passed arguments
 * are valid. If everything is ok, the request is linked to queue of
 * pending transfer requests.
 *
 */

WORD cmdIntXFer(struct IOUsbHWReq *ioreq,
                struct PCIUnit *unit,
                struct PCIDevice *base)
{
    struct PCIController *hc;

    pciusbDebug("UHW", DEBUGCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET "\n", __func__, ioreq, unit);

    //uhwDelayMS(1000, unit->hu_TimerReq); /* Wait 200 ms */
    uhwGetUsbState(ioreq, unit, base);
    if(!(ioreq->iouh_State & UHSF_OPERATIONAL)) {
        return(UHIOERR_USBOFFLINE);
    }

    /* Root Hub Emulation */
    if(uhwIsRootHubIOReq(ioreq, unit)) {
        return(cmdIntXFerRootHub(ioreq, unit, base));
    }

    hc = unit->hu_DevControllers[ioreq->iouh_DevAddr];
    if(!hc) {
        KPRINTF(20, "No Host controller assigned to device address %ld\n", ioreq->iouh_DevAddr);
        return(UHIOERR_HOSTERROR);
    }

    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    ioreq->iouh_Actual = 0;

    Disable();
    AddTail(&hc->hc_IntXFerQueue, (struct Node *) ioreq);
    Enable();
    SureCause(base, &hc->hc_CompleteInt);

    KPRINTF(10, "UHCMD_INTXFER processed ioreq: 0x%p\n", ioreq);
    return(RC_DONTREPLY);
}
/* \\\ */

/* /// "cmdFlush()" */
/*
 *======================================================================
 * cmdFlush(ioreq, base)
 *======================================================================
 *
 * This is the device CMD_FLUSH routine.
 *
 * This routine abort all pending transfer requests.
 *
 */

WORD cmdFlush(struct IOUsbHWReq *ioreq,
              struct PCIUnit *unit,
              struct PCIDevice *base)
{
    struct IOUsbHWReq *cmpioreq;
    struct PCIController *hc;
    UWORD devadrep;

    pciusbDebug("UHW", DEBUGCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET "\n", __func__, ioreq, unit);


    Disable();
    cmpioreq = (struct IOUsbHWReq *) unit->hu_RHIOQueue.lh_Head;
    while(((struct Node *) cmpioreq)->ln_Succ) {
        Remove(&cmpioreq->iouh_Req.io_Message.mn_Node);
        cmpioreq->iouh_Req.io_Error = IOERR_ABORTED;
        ReplyMsg(&cmpioreq->iouh_Req.io_Message);
        cmpioreq = (struct IOUsbHWReq *) unit->hu_RHIOQueue.lh_Head;
    }
    hc = (struct PCIController *) unit->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ) {
        cmpioreq = (struct IOUsbHWReq *) hc->hc_CtrlXFerQueue.lh_Head;
        while(((struct Node *) cmpioreq)->ln_Succ) {
            Remove(&cmpioreq->iouh_Req.io_Message.mn_Node);
            cmpioreq->iouh_Req.io_Error = IOERR_ABORTED;
            ReplyMsg(&cmpioreq->iouh_Req.io_Message);
            cmpioreq = (struct IOUsbHWReq *) hc->hc_CtrlXFerQueue.lh_Head;
        }
        cmpioreq = (struct IOUsbHWReq *) hc->hc_IntXFerQueue.lh_Head;
        while(((struct Node *) cmpioreq)->ln_Succ) {
            Remove(&cmpioreq->iouh_Req.io_Message.mn_Node);
            cmpioreq->iouh_Req.io_Error = IOERR_ABORTED;
            ReplyMsg(&cmpioreq->iouh_Req.io_Message);
            cmpioreq = (struct IOUsbHWReq *) hc->hc_IntXFerQueue.lh_Head;
        }
        cmpioreq = (struct IOUsbHWReq *) hc->hc_IsoXFerQueue.lh_Head;
        while(((struct Node *) cmpioreq)->ln_Succ) {
            Remove(&cmpioreq->iouh_Req.io_Message.mn_Node);
            cmpioreq->iouh_Req.io_Error = IOERR_ABORTED;
            ReplyMsg(&cmpioreq->iouh_Req.io_Message);
            cmpioreq = (struct IOUsbHWReq *) hc->hc_IsoXFerQueue.lh_Head;
        }
        cmpioreq = (struct IOUsbHWReq *) hc->hc_BulkXFerQueue.lh_Head;
        while(((struct Node *) cmpioreq)->ln_Succ) {
            Remove(&cmpioreq->iouh_Req.io_Message.mn_Node);
            cmpioreq->iouh_Req.io_Error = IOERR_ABORTED;
            ReplyMsg(&cmpioreq->iouh_Req.io_Message);
            cmpioreq = (struct IOUsbHWReq *) hc->hc_BulkXFerQueue.lh_Head;
        }
        switch(hc->hc_HCIType) {
        case HCITYPE_UHCI:
            cmpioreq = (struct IOUsbHWReq *) hc->hc_TDQueue.lh_Head;
            while(((struct Node *) cmpioreq)->ln_Succ) {
                Remove(&cmpioreq->iouh_Req.io_Message.mn_Node);
                devadrep = (cmpioreq->iouh_DevAddr<<5) + cmpioreq->iouh_Endpoint + ((cmpioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
                unit->hu_DevBusyReq[devadrep] = NULL;
                uhciFreeQContext(hc, (struct UhciQH *) cmpioreq->iouh_DriverPrivate1);
                cmpioreq->iouh_Req.io_Error = IOERR_ABORTED;
                ReplyMsg(&cmpioreq->iouh_Req.io_Message);
                cmpioreq = (struct IOUsbHWReq *) hc->hc_TDQueue.lh_Head;
            }
            break;

        case HCITYPE_EHCI:
            cmpioreq = (struct IOUsbHWReq *) hc->hc_TDQueue.lh_Head;
            while(((struct Node *) cmpioreq)->ln_Succ) {
                ehciFreeAsyncContext(hc, cmpioreq);
                cmpioreq->iouh_Req.io_Error = IOERR_ABORTED;
                ReplyMsg(&cmpioreq->iouh_Req.io_Message);
                cmpioreq = (struct IOUsbHWReq *) hc->hc_TDQueue.lh_Head;
            }
            cmpioreq = (struct IOUsbHWReq *) hc->hc_PeriodicTDQueue.lh_Head;
            while(((struct Node *) cmpioreq)->ln_Succ) {
                ehciFreePeriodicContext(hc, cmpioreq);
                cmpioreq->iouh_Req.io_Error = IOERR_ABORTED;
                ReplyMsg(&cmpioreq->iouh_Req.io_Message);
                cmpioreq = (struct IOUsbHWReq *) hc->hc_PeriodicTDQueue.lh_Head;
            }
            break;

        }
        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }
    Enable();
    /* Return success
    */
    return RC_OK;
}
/* \\\ */

#if defined(PCIUSB_WIP_ISO)
/* /// "cmdAddIsoHandler()" */
/*
 *======================================================================
 * cmdAddIsoHandler(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_ADDISOHANDLER routine.
 *
 * First it check if the usb is in proper state and if user passed arguments
 * are valid. If everything is ok, the request is linked to the list of
 * realtime iso handlers.
 *
 */

WORD cmdAddIsoHandler(struct IOUsbHWReq *ioreq,
                      struct PCIUnit *unit,
                      struct PCIDevice *base)
{
    struct PCIController *hc;
    struct RTIsoNode *rtn;

    WORD retval;
    UWORD requested_ptds;

    pciusbDebug("UHW", DEBUGCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET "\n", __func__, ioreq, unit);


    //uhwDelayMS(1000, unit->hu_TimerReq); /* Wait 200 ms */
    uhwGetUsbState(ioreq, unit, base);
    if(!(ioreq->iouh_State & UHSF_OPERATIONAL)) {
        return(UHIOERR_USBOFFLINE);
    }

    /* Root Hub Emulation */
    if(uhwIsRootHubIOReq(ioreq, unit)) {
        return(UHIOERR_BADPARAMS);
    }

    hc = unit->hu_DevControllers[ioreq->iouh_DevAddr];
    if(!hc) {
        return(UHIOERR_HOSTERROR);
    }
    if(!ioreq->iouh_Data) {
        return(UHIOERR_BADPARAMS);
    }

    ioreq->iouh_Actual = 0;

    Disable();
    rtn = (struct RTIsoNode *) RemHead((struct List *) &unit->hu_FreeRTIsoNodes);
    Enable();

    if(!rtn) {
        return UHIOERR_OUTOFMEMORY;
    }

    requested_ptds = hc->hc_IsoPTDCount ? hc->hc_IsoPTDCount : PCIUSB_ISO_PTD_COUNT;
    if(requested_ptds < 2)
        requested_ptds = 2;

    if(rtn->rtn_PTDs && rtn->rtn_PTDCount != requested_ptds) {
        FreeMem(rtn->rtn_PTDs, rtn->rtn_PTDCount * sizeof(struct PTDNode *));
        rtn->rtn_PTDs = NULL;
        rtn->rtn_PTDCount = 0;
    }

    if(!rtn->rtn_PTDs) {
        rtn->rtn_PTDs = AllocMem(requested_ptds * sizeof(struct PTDNode *), MEMF_CLEAR);
        rtn->rtn_PTDCount = rtn->rtn_PTDs ? requested_ptds : 0;
    }

    if(!rtn->rtn_PTDs) {
        Disable();
        AddTail((struct List *) &unit->hu_FreeRTIsoNodes, (struct Node *) &rtn->rtn_Node);
        Enable();
        return UHIOERR_OUTOFMEMORY;
    }

    for(UWORD ptdidx = 0; ptdidx < rtn->rtn_PTDCount; ptdidx++) {
        struct PTDNode *ptd = rtn->rtn_PTDs[ptdidx];

        if(!ptd) {
            ptd = AllocMem(sizeof(*ptd), MEMF_CLEAR);
            if(!ptd) {
                for(UWORD freectr = 0; freectr < ptdidx; freectr++) {
                    if(rtn->rtn_PTDs[freectr]) {
                        pciusbFreePTDChipset(hc, rtn->rtn_PTDs[freectr]);
                        FreeMem(rtn->rtn_PTDs[freectr], sizeof(struct PTDNode));
                    }
                    rtn->rtn_PTDs[freectr] = NULL;
                }
                Disable();
                AddTail((struct List *) &unit->hu_FreeRTIsoNodes, (struct Node *) &rtn->rtn_Node);
                Enable();
                return UHIOERR_OUTOFMEMORY;
            }
            rtn->rtn_PTDs[ptdidx] = ptd;
        } else {
            APTR descriptor = ptd->ptd_Descriptor;
            APTR chipset = ptd->ptd_Chipset;
            bzero(ptd, sizeof(*ptd));
            ptd->ptd_Descriptor = descriptor;
            ptd->ptd_Chipset = chipset;
            rtn->rtn_PTDs[ptdidx] = ptd;
        }
    }

    rtn->rtn_PTDCount = requested_ptds;

    /* copy some variables */
    rtn->rtn_IOReq.iouh_Flags = ioreq->iouh_Flags;
    rtn->rtn_IOReq.iouh_Dir = ioreq->iouh_Dir;
    rtn->rtn_IOReq.iouh_DevAddr = ioreq->iouh_DevAddr;
    rtn->rtn_IOReq.iouh_Endpoint = ioreq->iouh_Endpoint;
    rtn->rtn_IOReq.iouh_MaxPktSize = ioreq->iouh_MaxPktSize;
    rtn->rtn_IOReq.iouh_Interval = ioreq->iouh_Interval;
    rtn->rtn_IOReq.iouh_SplitHubAddr = ioreq->iouh_SplitHubAddr;
    rtn->rtn_IOReq.iouh_SplitHubPort = ioreq->iouh_SplitHubPort;

    rtn->rtn_RTIso = (struct IOUsbHWRTIso *) ioreq->iouh_Data;
    rtn->rtn_RTIso->urti_DriverPrivate1 = rtn; // backlink
    rtn->rtn_NextPTD = 0;
    rtn->rtn_NextFrame = 0;
    rtn->rtn_StdReq = NULL;
    rtn->rtn_BufferReq.ubr_Buffer = NULL;
    rtn->rtn_BufferReq.ubr_Length = ioreq->iouh_MaxPktSize;
    rtn->rtn_BufferReq.ubr_Frame = 0;
    rtn->rtn_BufferReq.ubr_Flags = 0;

    switch(hc->hc_HCIType) {
    case HCITYPE_EHCI:
        retval = ehciInitIsochIO(hc, rtn);
        break;
    case HCITYPE_UHCI:
        retval = uhciInitIsochIO(hc, rtn);
        break;
    default:
        retval = ohciInitIsochIO(hc, rtn);
        break;
    };

    if (retval != RC_OK) {
        Disable();
        AddTail((struct List *) &unit->hu_FreeRTIsoNodes, (struct Node *) &rtn->rtn_Node);
        Enable();
        return retval;
    }
    rtn->rtn_RTIso->urti_DriverPrivate1 = rtn; // backlink

    Disable();
    if (retval == RC_OK)
        AddTail((struct List *) &hc->hc_RTIsoHandlers, (struct Node *) &rtn->rtn_Node);
    else
        AddTail((struct List *) &unit->hu_FreeRTIsoNodes, (struct Node *) &rtn->rtn_Node);
    Enable();

    KPRINTF(10, "UHCMD_ADDISOHANDLER processed ioreq: 0x%08lx\n", ioreq);
    return(retval);
}
/* \\\ */

/* /// "cmdRemIsoHandler()" */
/*
 *======================================================================
 * cmdRemIsoHandler(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_REMISOHANDLER routine.
 *
 * Removes a previously added real time ISO handler.
 *
 */

WORD cmdRemIsoHandler(struct IOUsbHWReq *ioreq,
                      struct PCIUnit *unit,
                      struct PCIDevice *base)
{
    struct PCIController *hc;
    struct RTIsoNode *rtn;

    pciusbDebug("UHW", DEBUGCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET "\n", __func__, ioreq, unit);


    hc = unit->hu_DevControllers[ioreq->iouh_DevAddr];
    if(!hc) {
        return(UHIOERR_HOSTERROR);
    }

    Disable();
    rtn = (struct RTIsoNode *) hc->hc_RTIsoHandlers.mlh_Head;
    while(rtn->rtn_Node.mln_Succ) {
        if(rtn->rtn_RTIso == ioreq->iouh_Data) {
            break;
        }
        rtn = (struct RTIsoNode *) rtn->rtn_Node.mln_Succ;
    }
    if(!rtn->rtn_Node.mln_Succ) {
        Enable();
        KPRINTF(200, "UHCMD_REMISOHANDLER unable to find RTIso handler\n", ioreq);
        return(UHIOERR_BADPARAMS);
    }
    Remove((struct Node *) rtn);

    rtn->rtn_RTIso->urti_DriverPrivate1 = NULL;
    rtn->rtn_RTIso = NULL;

    switch(hc->hc_HCIType) {
    case HCITYPE_EHCI:
        ehciFreeIsochIO(hc, rtn);
        break;
    case HCITYPE_UHCI:
        uhciFreeIsochIO(hc, rtn);
        break;
    default:
        ohciFreeIsochIO(hc, rtn);
        break;
    };

    if(rtn->rtn_PTDs) {
        FreeMem(rtn->rtn_PTDs, rtn->rtn_PTDCount * sizeof(struct PTDNode *));
        rtn->rtn_PTDs = NULL;
        rtn->rtn_PTDCount = 0;
    }

    AddHead((struct List *) &unit->hu_FreeRTIsoNodes, (struct Node *) &rtn->rtn_Node);
    Enable();

    KPRINTF(10, "UHCMD_REMISOHANDLER processed ioreq: 0x%08lx\n", ioreq);
    return(RC_OK);
}
/* \\\ */

/* /// "cmdStartRTIso()" */
/*
 *======================================================================
 * cmdStartRTIso(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_STARTRTISO routine.
 *
 * Enables a previously added realtime iso handler.
 *
 */

WORD cmdStartRTIso(struct IOUsbHWReq *ioreq,
                   struct PCIUnit *unit,
                   struct PCIDevice *base)
{
    struct PCIController *hc;
    struct RTIsoNode *rtn;

    pciusbDebug("UHW", DEBUGCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET "\n", __func__, ioreq, unit);


    hc = unit->hu_DevControllers[ioreq->iouh_DevAddr];
    if(!hc) {
        return(UHIOERR_HOSTERROR);
    }

    Disable();
    rtn = (struct RTIsoNode *) hc->hc_RTIsoHandlers.mlh_Head;
    while(rtn->rtn_Node.mln_Succ) {
        if(rtn->rtn_RTIso == ioreq->iouh_Data) {
            break;
        }
        rtn = (struct RTIsoNode *) rtn->rtn_Node.mln_Succ;
    }
    if(!rtn->rtn_Node.mln_Succ) {
        Enable();
        return(UHIOERR_BADPARAMS);
    }

    UWORD prefill = (rtn->rtn_PTDCount > 1) ? 2 : 1;
        for (UWORD cnt = 0; cnt < prefill; cnt++) {
            WORD queueresult = RC_OK;
            switch(hc->hc_HCIType) {
        case HCITYPE_EHCI:
            queueresult = ehciQueueIsochIO(hc, rtn);
            break;
        case HCITYPE_UHCI:
            queueresult = uhciQueueIsochIO(hc, rtn);
            break;
        default:
            queueresult = ohciQueueIsochIO(hc, rtn);
            break;
        };
        if (queueresult != RC_OK) {
            Enable();
            return queueresult;
        }
    }

    switch(hc->hc_HCIType) {
    case HCITYPE_EHCI:
        if(!ehciStartIsochIO(hc, rtn)) {
            Enable();
            return UHIOERR_OUTOFMEMORY;
        }
        break;
    case HCITYPE_UHCI:
        uhciStartIsochIO(hc, rtn);
        break;
    default:
        ohciStartIsochIO(hc, rtn);
        break;
    };

    Enable();

    return(RC_OK);
}
/* \\\ */

/* /// "cmdStopRTIso()" */
/*
 *======================================================================
 * cmdStopRTIso(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_STOPRTISO routine.
 *
 * Disables a previously added realtime iso handler.
 *
 */

WORD cmdStopRTIso(struct IOUsbHWReq *ioreq,
                  struct PCIUnit *unit,
                  struct PCIDevice *base)
{
    struct PCIController *hc;
    struct RTIsoNode *rtn;

    pciusbDebug("UHW", DEBUGCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET "\n", __func__, ioreq, unit);


    hc = unit->hu_DevControllers[ioreq->iouh_DevAddr];
    if(!hc) {
        return(UHIOERR_HOSTERROR);
    }

    Disable();
    rtn = (struct RTIsoNode *) hc->hc_RTIsoHandlers.mlh_Head;
    while(rtn->rtn_Node.mln_Succ) {
        if(rtn->rtn_RTIso == ioreq->iouh_Data) {
            break;
        }
        rtn = (struct RTIsoNode *) rtn->rtn_Node.mln_Succ;
    }
    if(!rtn->rtn_Node.mln_Succ) {
        Enable();
        return(UHIOERR_BADPARAMS);
    }

    switch(hc->hc_HCIType) {
    case HCITYPE_EHCI:
        ehciStopIsochIO(hc, rtn);
        break;
    case HCITYPE_UHCI:
        uhciStopIsochIO(hc, rtn);
        break;
    default:
        ohciStopIsochIO(hc, rtn);
        break;
    };

    Enable();

    return(RC_OK);
}
/* \\\ */
#endif /* PCIUSB_WIP_ISO */

/* /// "NSD stuff" */

static
const UWORD NSDSupported[] = {
    CMD_FLUSH, CMD_RESET,
    UHCMD_QUERYDEVICE, UHCMD_USBRESET,
    UHCMD_USBRESUME, UHCMD_USBSUSPEND,
    UHCMD_USBOPER, UHCMD_CONTROLXFER,
    UHCMD_ISOXFER, UHCMD_INTXFER,
    UHCMD_BULKXFER,
#if defined(PCIUSB_WIP_ISO)
    UHCMD_ADDISOHANDLER, UHCMD_REMISOHANDLER,
    UHCMD_STARTRTISO, UHCMD_STOPRTISO,
#endif
    NSCMD_DEVICEQUERY, 0
};

WORD cmdNSDeviceQuery(struct IOStdReq *ioreq,
                      struct PCIUnit *unit,
                      struct PCIDevice *base)
{
    struct NSDeviceQueryResult *query;

    query = (struct NSDeviceQueryResult *) ioreq->io_Data;


    pciusbDebug("UHW", DEBUGCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET "\n", __func__, ioreq, base);
    KPRINTF(10, "NSCMD_DEVICEQUERY query: 0x%p\n", query);

    /* NULL ptr?
       Enough data?
       Valid request?
    */
    if((!query) ||
            (ioreq->io_Length < sizeof(struct NSDeviceQueryResult)) ||
            (query->DevQueryFormat != 0) ||
            (query->SizeAvailable != 0)) {
        /* Return error. This is special handling, since iorequest is only
           guaranteed to be sizeof(struct IOStdReq). If we'd let our
           devBeginIO dispatcher return the error, it would trash some
           memory past end of the iorequest (ios2_WireError field).
         */
        ioreq->io_Error = IOERR_NOCMD;
        TermIO((struct IOUsbHWReq *) ioreq, base);

        /* Don't reply, we already did.
        */
        return RC_DONTREPLY;
    }

    ioreq->io_Actual         = query->SizeAvailable
                               = sizeof(struct NSDeviceQueryResult);
    query->DeviceType        = NSDEVTYPE_USBHARDWARE;
    query->DeviceSubType     = 0;
    query->SupportedCommands = (UWORD *)NSDSupported;

    /* Return success (note that this will NOT poke ios2_WireError).
    */
    return RC_OK;
}
/* \\\ */

/* /// "TermIO()" */
/*
 *===========================================================
 * TermIO(ioreq, base)
 *===========================================================
 *
 * Return completed ioreq to sender.
 *
 */

void TermIO(struct IOUsbHWReq *ioreq,
            struct PCIDevice *base)
{
    ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_FREEMSG;

    /* If not quick I/O, reply the message
    */
    if(!(ioreq->iouh_Req.io_Flags & IOF_QUICK)) {
        ReplyMsg(&ioreq->iouh_Req.io_Message);
    }
}
/* \\\ */

/* /// "cmdAbortIO()" */
BOOL cmdAbortIO(struct IOUsbHWReq *ioreq, struct PCIDevice *base)
{
    struct PCIUnit *unit = (struct PCIUnit *) ioreq->iouh_Req.io_Unit;
    struct IOUsbHWReq *cmpioreq;
    struct PCIController *hc;
    UWORD devadrep;
    BOOL foundit = FALSE;

    pciusbDebug("UHW", DEBUGCOLOR_SET "%s(0x%p)" DEBUGCOLOR_RESET "\n", __func__, ioreq);

    Disable();
    cmpioreq = (struct IOUsbHWReq *) unit->hu_RHIOQueue.lh_Head;
    while(((struct Node *) cmpioreq)->ln_Succ) {
        if(ioreq == cmpioreq) {
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            Enable();
            return TRUE;
        }
        cmpioreq = (struct IOUsbHWReq *) cmpioreq->iouh_Req.io_Message.mn_Node.ln_Succ;
    }

    hc = (struct PCIController *) unit->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ) {
        cmpioreq = (struct IOUsbHWReq *) hc->hc_CtrlXFerQueue.lh_Head;
        while(((struct Node *) cmpioreq)->ln_Succ) {
            if(ioreq == cmpioreq) {
                foundit = TRUE;
                break;
            }
            cmpioreq = (struct IOUsbHWReq *) cmpioreq->iouh_Req.io_Message.mn_Node.ln_Succ;
        }
        if(!foundit) {
            cmpioreq = (struct IOUsbHWReq *) hc->hc_IntXFerQueue.lh_Head;
            while(((struct Node *) cmpioreq)->ln_Succ) {
                if(ioreq == cmpioreq) {
                    foundit = TRUE;
                    break;
                }
                cmpioreq = (struct IOUsbHWReq *) cmpioreq->iouh_Req.io_Message.mn_Node.ln_Succ;
            }
        }
        if(!foundit) {
            cmpioreq = (struct IOUsbHWReq *) hc->hc_IsoXFerQueue.lh_Head;
            while(((struct Node *) cmpioreq)->ln_Succ) {
                if(ioreq == cmpioreq) {
                    foundit = TRUE;
                    break;
                }
                cmpioreq = (struct IOUsbHWReq *) cmpioreq->iouh_Req.io_Message.mn_Node.ln_Succ;
            }
        }
        if(!foundit) {
            cmpioreq = (struct IOUsbHWReq *) hc->hc_BulkXFerQueue.lh_Head;
            while(((struct Node *) cmpioreq)->ln_Succ) {
                if(ioreq == cmpioreq) {
                    foundit = TRUE;
                    break;
                }
                cmpioreq = (struct IOUsbHWReq *) cmpioreq->iouh_Req.io_Message.mn_Node.ln_Succ;
            }
        }
        if(!foundit) {
            // IOReq is probably pending in some transfer structure
            devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
            switch(hc->hc_HCIType) {
            case HCITYPE_UHCI:
                cmpioreq = (struct IOUsbHWReq *) hc->hc_TDQueue.lh_Head;
                while(((struct Node *) cmpioreq)->ln_Succ) {
                    if(ioreq == cmpioreq) {
                        foundit = TRUE;
                        unit->hu_DevBusyReq[devadrep] = NULL;
                        uhciFreeQContext(hc, (struct UhciQH *) ioreq->iouh_DriverPrivate1);
                        break;
                    }
                    cmpioreq = (struct IOUsbHWReq *) cmpioreq->iouh_Req.io_Message.mn_Node.ln_Succ;
                }
                break;

            case HCITYPE_OHCI:
                cmpioreq = (struct IOUsbHWReq *) hc->hc_TDQueue.lh_Head;
                while(((struct Node *) cmpioreq)->ln_Succ) {
                    if(ioreq == cmpioreq) {
                        /*
                         * Request's ED is in use by the HC, as well as its TDs and
                         * data buffers.
                         * Schedule abort on the HC driver and reply the request
                         * only when done. However return success.
                         */
                        ioreq->iouh_Req.io_Error = IOERR_ABORTED;
                        ohciAbortRequest(hc, ioreq);
                        Enable();
                        return TRUE;
                    }
                    cmpioreq = (struct IOUsbHWReq *) cmpioreq->iouh_Req.io_Message.mn_Node.ln_Succ;
                }
                break;

            case HCITYPE_EHCI:
                cmpioreq = (struct IOUsbHWReq *) hc->hc_TDQueue.lh_Head;
                while(((struct Node *) cmpioreq)->ln_Succ) {
                    if(ioreq == cmpioreq) {
                        /*
                         * CHECKME: Perhaps immediate freeing can cause issues similar to OHCI.
                         * Should synchronized abort routine be implemented here too ?
                         */
                        ehciFreeAsyncContext(hc, ioreq);
                        Enable();
                        ioreq->iouh_Req.io_Error = IOERR_ABORTED;
                        TermIO(ioreq, base);
                        return TRUE;
                    }
                    cmpioreq = (struct IOUsbHWReq *) cmpioreq->iouh_Req.io_Message.mn_Node.ln_Succ;
                }
                cmpioreq = (struct IOUsbHWReq *) hc->hc_PeriodicTDQueue.lh_Head;
                while(((struct Node *) cmpioreq)->ln_Succ) {
                    if(ioreq == cmpioreq) {
                        ehciFreePeriodicContext(hc, ioreq);
                        Enable();
                        ioreq->iouh_Req.io_Error = IOERR_ABORTED;
                        TermIO(ioreq, base);
                        return TRUE;
                    }
                    cmpioreq = (struct IOUsbHWReq *) cmpioreq->iouh_Req.io_Message.mn_Node.ln_Succ;
                }
                break;

            }
        }
        if(foundit) {
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            break;
        }
        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }
    Enable();

    if (foundit) {
        ioreq->iouh_Req.io_Error = IOERR_ABORTED;
        TermIO(ioreq, base);
    } else {
        KPRINTF(20, "WARNING, could not abort unknown IOReq %p\n", ioreq);
    }
    return(foundit);
}
/* \\\ */

/* /// "uhwCheckRootHubChanges()" */
void uhwCheckRootHubChanges(struct PCIUnit *unit)
{
    struct PCIDevice *base = unit->hu_Device;
    struct IOUsbHWReq *ioreq;

    pciusbDebugV("UHW", DEBUGCOLOR_SET "%s(0x%p)" DEBUGCOLOR_RESET "\n", __func__, unit);

    if(unit->hu_RootPortChanges && unit->hu_RHIOQueue.lh_Head->ln_Succ) {
        KPRINTF(1, "Portchange map %04lx\n", unit->hu_RootPortChanges);
        Disable();
        ioreq = (struct IOUsbHWReq *) unit->hu_RHIOQueue.lh_Head;
        while(((struct Node *) ioreq)->ln_Succ) {
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            if((unit->hu_RootHubPorts < 8) || (ioreq->iouh_Length == 1)) {
                *((UBYTE *) ioreq->iouh_Data) = unit->hu_RootPortChanges;
                ioreq->iouh_Actual = 1;
            } else {
                ((UBYTE *) ioreq->iouh_Data)[0] = unit->hu_RootPortChanges;
                ((UBYTE *) ioreq->iouh_Data)[1] = unit->hu_RootPortChanges>>8;
                ioreq->iouh_Actual = 2;
            }
            ReplyMsg(&ioreq->iouh_Req.io_Message);
            ioreq = (struct IOUsbHWReq *) unit->hu_RHIOQueue.lh_Head;
        }
        unit->hu_RootPortChanges = 0;
        Enable();
    }
}
/* \\\ */

/* /// "uhwCheckSpecialCtrlTransfers()" */
void uhwCheckSpecialCtrlTransfers(struct PCIController *hc, struct IOUsbHWReq *ioreq)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct PCIDevice *base = unit->hu_Device;

    pciusbDebug("UHW", DEBUGCOLOR_SET "%s()" DEBUGCOLOR_RESET "\n", __func__);

    /* Clear Feature(Endpoint halt) */
    if((ioreq->iouh_SetupData.bmRequestType == (URTF_STANDARD|URTF_ENDPOINT)) &&
            (ioreq->iouh_SetupData.bRequest == USR_CLEAR_FEATURE) &&
            (ioreq->iouh_SetupData.wValue == AROS_WORD2LE(UFS_ENDPOINT_HALT))) {
        pciusbDebug("UHW", DEBUGCOLOR_SET "%s: Resetting toggle bit for endpoint %ld" DEBUGCOLOR_RESET "\n", __func__, AROS_WORD2LE(ioreq->iouh_SetupData.wIndex) & 0xf);
        unit->hu_DevDataToggle[(ioreq->iouh_DevAddr<<5)|(AROS_WORD2LE(ioreq->iouh_SetupData.wIndex) & 0xf)|((AROS_WORD2LE(ioreq->iouh_SetupData.wIndex) & 0x80)>>3)] = 0;
    } else if((ioreq->iouh_SetupData.bmRequestType == (URTF_STANDARD|URTF_DEVICE)) &&
              (ioreq->iouh_SetupData.bRequest == USR_SET_ADDRESS)) {
        /* Set Address -> clear all endpoints */
        ULONG epnum;
        ULONG adr = AROS_WORD2BE(ioreq->iouh_SetupData.wValue)>>3;
        pciusbDebug("UHW", DEBUGCOLOR_SET "%s: Resetting toggle bits for device address %ld" DEBUGCOLOR_RESET "\n", __func__, adr>>5);
        for(epnum = 0; epnum < 31; epnum++) {
            unit->hu_DevDataToggle[adr+epnum] = 0;
        }
        // transfer host controller ownership
        unit->hu_DevControllers[ioreq->iouh_DevAddr] = NULL;
        unit->hu_DevControllers[adr>>5] = hc;
    } else if((ioreq->iouh_SetupData.bmRequestType == (URTF_CLASS|URTF_OTHER)) &&
              (ioreq->iouh_SetupData.bRequest == USR_SET_FEATURE) &&
              (ioreq->iouh_SetupData.wValue == AROS_WORD2LE(UFS_PORT_RESET))) {
        // a hub will be enumerating a device on this host controller soon!
        pciusbDebug("UHW", DEBUGCOLOR_SET "%s: Hub RESET caught, assigning Dev0 to %p!" DEBUGCOLOR_RESET "\n", __func__, hc);
        unit->hu_DevControllers[0] = hc;
    }
}
/* \\\ */

/* /// "uhwNakTimeoutInt()" */
AROS_INTH1(uhwNakTimeoutInt, struct PCIUnit *,  unit)
{
    AROS_INTFUNC_INIT

    struct PCIDevice *base = unit->hu_Device;
    struct PCIController *hc;
    struct IOUsbHWReq *ioreq;
    struct UhciQH *uqh;
    struct UhciTD *utd;
    struct EhciQH *eqh;
    UWORD devadrep;
    UWORD cnt;
    ULONG linkelem;
    ULONG ctrlstatus;
    BOOL causeint;

    pciusbDebugV("UHW", DEBUGCOLOR_SET "%s(0x%p)" DEBUGCOLOR_RESET "\n", __func__, unit);

    // check for port status change for UHCI and frame rollovers and NAK Timeouts
    hc = (struct PCIController *) unit->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ) {
        if (!(hc->hc_Flags & HCF_ONLINE)) {
            hc = (struct PCIController *) hc->hc_Node.ln_Succ;
            continue;
        }
        ULONG framecnt;
        causeint = FALSE;
        switch(hc->hc_HCIType) {
        case HCITYPE_UHCI: {
            uhciUpdateFrameCounter(hc);
            framecnt = hc->hc_FrameCounter;

            // NakTimeout
            ioreq = (struct IOUsbHWReq *) hc->hc_TDQueue.lh_Head;
            while(((struct Node *) ioreq)->ln_Succ) {
                if(ioreq->iouh_Flags & UHFF_NAKTIMEOUT) {
                    uqh = (struct UhciQH *) ioreq->iouh_DriverPrivate1;
                    if(uqh) {
                        KPRINTF(1, "UHCI: Examining IOReq=%p with UQH=%p\n", ioreq, uqh);
                        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
                        linkelem = READMEM32_LE(&uqh->uqh_Element);
                        if(linkelem & UHCI_TERMINATE) {
                            KPRINTF(1, "UHCI: UQH terminated %08lx\n", linkelem);
                            if(framecnt > unit->hu_NakTimeoutFrame[devadrep]) {
                                // give the thing the chance to exit gracefully
                                KPRINTF(20, "UHCI: Terminated? NAK timeout %ld > %ld, IOReq=%p\n", framecnt, unit->hu_NakTimeoutFrame[devadrep], ioreq);
                                causeint = TRUE;
                            }
                        } else {
                            utd = (struct UhciTD *) (((IPTR)linkelem & UHCI_PTRMASK) - hc->hc_PCIVirtualAdjust - 16); // struct UhciTD starts 16 before physical TD
                            ctrlstatus = READMEM32_LE(&utd->utd_CtrlStatus);
                            if(ctrlstatus & UTCF_ACTIVE) {
                                if(framecnt > unit->hu_NakTimeoutFrame[devadrep]) {
                                    // give the thing the chance to exit gracefully
                                    KPRINTF(20, "UHCI: NAK timeout %ld > %ld, IOReq=%p\n", framecnt, unit->hu_NakTimeoutFrame[devadrep], ioreq);
                                    ctrlstatus &= ~UTCF_ACTIVE;
                                    WRITEMEM32_LE(&utd->utd_CtrlStatus, ctrlstatus);
                                    causeint = TRUE;
                                }
                            } else {
                                if(framecnt > unit->hu_NakTimeoutFrame[devadrep]) {
                                    // give the thing the chance to exit gracefully
                                    KPRINTF(20, "UHCI: Terminated? NAK timeout %ld > %ld, IOReq=%p\n", framecnt, unit->hu_NakTimeoutFrame[devadrep], ioreq);
                                    causeint = TRUE;
                                }
                            }
                        }
                    }
                }
                ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            }

            uhciCheckPortStatusChange(hc);
            break;
        }

        case HCITYPE_OHCI: {
            struct OhciHCPrivate *ohcihcp = (struct OhciHCPrivate *)hc->hc_CPrivate;
            ohciUpdateFrameCounter(hc);
            framecnt = hc->hc_FrameCounter;
            // NakTimeout
            ioreq = (struct IOUsbHWReq *) hc->hc_TDQueue.lh_Head;
            while(((struct Node *) ioreq)->ln_Succ) {
                // Remember the successor because ohciAbortRequest() will move the request to another list
                struct IOUsbHWReq *succ = (struct IOUsbHWReq *)ioreq->iouh_Req.io_Message.mn_Node.ln_Succ;

                if(ioreq->iouh_Flags & UHFF_NAKTIMEOUT) {
                    KPRINTF(1, "OHCI: Examining IOReq=%p with OED=%p\n", ioreq, ioreq->iouh_DriverPrivate1);
                    if (ioreq->iouh_DriverPrivate1) {
                        KPRINTF(1, "OHCI: CTRL=%04lx, CMD=%01lx, F=%ld, hccaDH=%08lx, hcDH=%08lx, CH=%08lx, CCH=%08lx, IntEn=%08lx\n",
                                READREG32_LE(hc->hc_RegBase, OHCI_CONTROL),
                                READREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS),
                                READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT),
                                READMEM32_LE(&ohcihcp->ohc_OhciHCCA->oha_DoneHead),
                                READREG32_LE(hc->hc_RegBase, OHCI_DONEHEAD),
                                READREG32_LE(hc->hc_RegBase, OHCI_CTRL_HEAD_ED),
                                READREG32_LE(hc->hc_RegBase, OHCI_CTRL_ED),
                                READREG32_LE(hc->hc_RegBase, OHCI_INTEN));

                        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
                        if(framecnt > unit->hu_NakTimeoutFrame[devadrep]) {
                            // give the thing the chance to exit gracefully
                            KPRINTF(200, "OHCI: HC 0x%p NAK timeout %ld > %ld, IOReq=%p\n", hc, framecnt, unit->hu_NakTimeoutFrame[devadrep], ioreq);
                            ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                            ohciAbortRequest(hc, ioreq);
                        }
                    }
                }
                ioreq = succ;
            }
            break;
        }

        case HCITYPE_EHCI: {
            ehciUpdateFrameCounter(hc);
            framecnt = hc->hc_FrameCounter;
            // NakTimeout
            for(cnt = 0; cnt < 2; cnt++) {
                ioreq = (struct IOUsbHWReq *) (cnt ? hc->hc_PeriodicTDQueue.lh_Head : hc->hc_TDQueue.lh_Head);
                while(((struct Node *) ioreq)->ln_Succ) {
                    if(ioreq->iouh_Flags & UHFF_NAKTIMEOUT) {
                        eqh = (struct EhciQH *) ioreq->iouh_DriverPrivate1;
                        if(eqh) {
                            KPRINTF(1, "EHCI: Examining IOReq=%p with EQH=%p\n", ioreq, eqh);
                            devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
                            ctrlstatus = READMEM32_LE(&eqh->eqh_CtrlStatus);
                            if(ctrlstatus & ETCF_ACTIVE) {
                                if(framecnt > unit->hu_NakTimeoutFrame[devadrep]) {
                                    // give the thing the chance to exit gracefully
                                    KPRINTF(20, "EHCI: NAK timeout %ld > %ld, IOReq=%p\n", framecnt, unit->hu_NakTimeoutFrame[devadrep], ioreq);
                                    ctrlstatus &= ~ETCF_ACTIVE;
                                    ctrlstatus |= ETSF_HALTED;
                                    WRITEMEM32_LE(&eqh->eqh_CtrlStatus, ctrlstatus);
                                    causeint = TRUE;
                                }
                            } else {
                                if(ctrlstatus & ETCF_READYINTEN) {
                                    KPRINTF(10, "EHCI: INT missed?!? Manually causing it! %08lx, IOReq=%p\n",
                                            ctrlstatus, ioreq);
                                    causeint = TRUE;
                                }
                            }
                        }
                    }
                    ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
                }
            }
            break;
        }

        }
        if(causeint) {
            SureCause(base, &hc->hc_CompleteInt);
        }

        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }

    uhwCheckRootHubChanges(unit);

    unit->hu_NakTimeoutReq.tr_time.tv_micro = 150*1000;
    SendIO((APTR) &unit->hu_NakTimeoutReq);

//    KPRINTF(1, "Exit NakTimeoutInt(0x%p)\n", unit);

    return FALSE;

    AROS_INTFUNC_EXIT
}
/* \\\ */
