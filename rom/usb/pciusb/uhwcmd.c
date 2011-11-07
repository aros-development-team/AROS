/* uhwcmd.c - pciusb.device by Chris Hodges
*/

#include <devices/usb_hub.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/timer.h>

#include <strings.h>

#include "uhwcmd.h"
#include "ohciproto.h"

#define NewList NEWLIST

/* we cannot use AROS_WORD2LE in struct initializer */
#if AROS_BIG_ENDIAN
#define WORD2LE(w) (UWORD)(((w) >> 8) & 0x00FF) | (((w) << 8) & 0xFF00)
#else
#define WORD2LE(w) (w)
#endif

/* Root hub data */
const struct UsbStdDevDesc RHDevDesc = { sizeof(struct UsbStdDevDesc), UDT_DEVICE, WORD2LE(0x0110), HUB_CLASSCODE, 0, 0, 8, WORD2LE(0x0000), WORD2LE(0x0000), WORD2LE(0x0100), 1, 2, 0, 1 };

const struct UsbStdCfgDesc RHCfgDesc = { 9, UDT_CONFIGURATION, WORD2LE(9+9+7), 1, 1, 3, USCAF_ONE|USCAF_SELF_POWERED, 0 };
const struct UsbStdIfDesc  RHIfDesc  = { 9, UDT_INTERFACE, 0, 0, 1, HUB_CLASSCODE, 0, 0, 4 };
const struct UsbStdEPDesc  RHEPDesc  = { 7, UDT_ENDPOINT, URTF_IN|1, USEAF_INTERRUPT, WORD2LE(8), 255 };
const struct UsbHubDesc    RHHubDesc = { 9,                                              // 0 Number of bytes in this descriptor, including this byte
                                         UDT_HUB,                                        // 1 Descriptor Type, value: 29H for hub descriptor
                                         0,                                              // 2 Number of downstream facing ports that this hub supports
                                         WORD2LE(UHCF_INDIVID_POWER|UHCF_INDIVID_OVP),   // 3 wHubCharacteristics
                                         0,                                              // 5 bPwrOn2PwrGood
                                         1,                                              // 6 bHubContrCurrent
                                         1,                                              // 7 DeviceRemovable (size is variable)
                                         0                                               // x PortPwrCtrlMask (size is variable)
                                       };
#if (AROS_USB30_CODE)
const struct UsbSSHubDesc  RHSSHubDesc = { 12,                                           // 0 Number of bytes in this descriptor, including this byte. (12 bytes)
                                           UDT_SSHUB,                                    // 1 Descriptor Type, value: 2AH for SuperSpeed hub descriptor
                                           0,                                            // 2 Number of downstream facing ports that this hub supports. The maximum number of ports of ports a hub can support is 15
                                           WORD2LE(UHCF_INDIVID_POWER|UHCF_INDIVID_OVP), // 3 wHubCharacteristics
                                           0,                                            // 5 bPwrOn2PwrGood
                                           10,                                           // 6 bHubContrCurrent
                                           0,                                            // 7 bHubHdrDecLat
                                           0,                                            // 8 wHubDelay
                                           0                                             // 10 DeviceRemovable
                                         };
#endif

const CONST_STRPTR RHStrings[] = { "Chris Hodges", "PCI Root Hub Unit x", "Standard Config", "Hub interface" };

/* /// "SureCause()" */
void SureCause(struct PCIDevice *base, struct Interrupt *interrupt)
{
    /* this is a workaround for the original Cause() function missing tailed calls */
    Disable();

    if((interrupt->is_Node.ln_Type == NT_SOFTINT) || (interrupt->is_Node.ln_Type == NT_USER))
    {
        // signal tailed call
        interrupt->is_Node.ln_Type = NT_USER;
    } else {
        do
        {
            interrupt->is_Node.ln_Type = NT_SOFTINT;
            Forbid(); // make sure code is not interrupted by other tasks
            Enable();
            (*((void (*)(struct Interrupt *)) (interrupt->is_Code)))(interrupt->is_Data);
            Disable();
            Permit();
        } while(interrupt->is_Node.ln_Type != NT_SOFTINT);
        interrupt->is_Node.ln_Type = NT_INTERRUPT;
    }
    Enable();
}
/* \\\ */

/* /// "uhwOpenTimer()" */
BOOL uhwOpenTimer(struct PCIUnit *unit, struct PCIDevice *base)
{
    if((unit->hu_MsgPort = CreateMsgPort()))
    {
        if((unit->hu_TimerReq = (struct timerequest *) CreateIORequest(unit->hu_MsgPort, sizeof(struct timerequest))))
        {
            if(!OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *) unit->hu_TimerReq, 0))
            {
                unit->hu_TimerReq->tr_node.io_Message.mn_Node.ln_Name = "PCI hardware";
                unit->hu_TimerReq->tr_node.io_Command = TR_ADDREQUEST;
                KPRINTF(1, ("opened timer device\n"));
                return(TRUE);
            }
            DeleteIORequest((struct IORequest *) unit->hu_TimerReq);
            unit->hu_TimerReq = NULL;
        }
        DeleteMsgPort(unit->hu_MsgPort);
        unit->hu_MsgPort = NULL;
    }
    KPRINTF(5, ("failed to open timer.device\n"));
    return(FALSE);
}
/* \\\ */

/* /// "uhwDelayMS()" */
void uhwDelayMS(ULONG milli, struct PCIUnit *unit)
{
    unit->hu_TimerReq->tr_time.tv_secs  = 0;
    unit->hu_TimerReq->tr_time.tv_micro = milli * 1000;
    DoIO((struct IORequest *) unit->hu_TimerReq);
}
/* \\\ */

/* /// "uhwDelayMicro()" */
void uhwDelayMicro(ULONG micro, struct PCIUnit *unit)
{
    unit->hu_TimerReq->tr_time.tv_secs  = 0;
    unit->hu_TimerReq->tr_time.tv_micro = micro;
    DoIO((struct IORequest *) unit->hu_TimerReq);
}
/* \\\ */

/* /// "uhwCloseTimer()" */
void uhwCloseTimer(struct PCIUnit *unit, struct PCIDevice *base)
{
    if(unit->hu_MsgPort)
    {
        if(unit->hu_TimerReq)
        {
            KPRINTF(1, ("closing timer.device\n"));
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

    if(!base->hd_ScanDone)
    {
        base->hd_ScanDone = TRUE;
        if(!pciInit(base))
        {
            return NULL;
        }
    }
    unit = (struct PCIUnit *) base->hd_Units.lh_Head;
    while(((struct Node *) unit)->ln_Succ)
    {
        if(unit->hu_UnitNo == unitnr)
        {
            break;
        }
        unit = (struct PCIUnit *) ((struct Node *) unit)->ln_Succ;
    }
    if(!((struct Node *) unit)->ln_Succ)
    {
        KPRINTF(20, ("Unit %ld does not exist!\n", unitnr));
        return NULL;
    }
    if(unit->hu_UnitAllocated)
    {
        ioreq->iouh_Req.io_Error = IOERR_UNITBUSY;
        KPRINTF(5, ("Unit %ld already open!\n", unitnr));
        return NULL;
    }

    if(uhwOpenTimer(unit, base))
    {

        if(pciAllocUnit(unit)) // hardware self test
        {
            unit->hu_UnitAllocated = TRUE;
            unit->hu_NakTimeoutInt.is_Node.ln_Type = NT_INTERRUPT;
            unit->hu_NakTimeoutInt.is_Node.ln_Name = "PCI NakTimeout";
            unit->hu_NakTimeoutInt.is_Node.ln_Pri  = -16;
            unit->hu_NakTimeoutInt.is_Data = unit;
            unit->hu_NakTimeoutInt.is_Code = (void (*)(void)) &uhwNakTimeoutInt;

            CopyMem(unit->hu_TimerReq, &unit->hu_NakTimeoutReq, sizeof(struct timerequest));
            unit->hu_NakTimeoutReq.tr_node.io_Message.mn_ReplyPort = &unit->hu_NakTimeoutMsgPort;
            unit->hu_NakTimeoutMsgPort.mp_Node.ln_Type = NT_MSGPORT;
            unit->hu_NakTimeoutMsgPort.mp_Flags = PA_SOFTINT;
            unit->hu_NakTimeoutMsgPort.mp_SigTask = &unit->hu_NakTimeoutInt;
            NewList(&unit->hu_NakTimeoutMsgPort.mp_MsgList);
            Cause(&unit->hu_NakTimeoutInt);
            return(&unit->hu_Unit);
        } else {
            ioreq->iouh_Req.io_Error = IOERR_SELFTEST;
            KPRINTF(20, ("Hardware allocation failure!\n"));
        }
        uhwCloseTimer(unit, base);
    }
    return(NULL);
}
/* \\\ */

/* /// "Close_Unit()" */
void Close_Unit(struct PCIDevice *base,
                struct PCIUnit *unit,
                struct IOUsbHWReq *ioreq)
{
    /* Disable all interrupts */
    unit->hu_NakTimeoutMsgPort.mp_Flags = PA_IGNORE;
    unit->hu_NakTimeoutInt.is_Node.ln_Type = NT_SOFTINT;
    AbortIO((APTR) &unit->hu_NakTimeoutReq);

    pciFreeUnit(unit);

    uhwCloseTimer(unit, base);
    unit->hu_UnitAllocated = FALSE;
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
    KPRINTF(10, ("CMD_RESET ioreq: 0x%p\n", ioreq));

    uhwDelayMS(1, unit);
    uhwGetUsbState(ioreq, unit, base);

    if(ioreq->iouh_State & UHSF_OPERATIONAL)
    {
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
    KPRINTF(10, ("UHCMD_USBRESET ioreq: 0x%p\n", ioreq));

    /* FIXME */
    uhwGetUsbState(ioreq, unit, base);

    unit->hu_FrameCounter = 1;
    unit->hu_RootHubAddr = 0;

    if(ioreq->iouh_State & UHSF_OPERATIONAL)
    {
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
    KPRINTF(10, ("UHCMD_USBRESUME ioreq: 0x%p\n", ioreq));

    /* FIXME */
    uhwGetUsbState(ioreq, unit, base);
    if(ioreq->iouh_State & UHSF_OPERATIONAL)
    {
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
    KPRINTF(10, ("UHCMD_USBSUSPEND ioreq: 0x%p\n", ioreq));

    /* FIXME */
    uhwGetUsbState(ioreq, unit, base);
    if(ioreq->iouh_State & UHSF_SUSPENDED)
    {
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
    KPRINTF(10, ("UHCMD_USBOPER ioreq: 0x%p\n", ioreq));

    /* FIXME */
    uhwGetUsbState(ioreq, unit, base);
    if(ioreq->iouh_State & UHSF_OPERATIONAL)
    {
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

    KPRINTF(10, ("UHCMD_QUERYDEVICE ioreq: 0x%p, taglist: 0x%p\n", ioreq, taglist));

    if((tag = FindTagItem(UHA_State, taglist)))
    {
        *((ULONG *) tag->ti_Data) = (ULONG) uhwGetUsbState(ioreq, unit, base);
        count++;
    }
    if((tag = FindTagItem(UHA_Manufacturer, taglist)))
    {
        *((STRPTR *) tag->ti_Data) = "Chris Hodges";
        count++;
    }
    if((tag = FindTagItem(UHA_ProductName, taglist)))
    {
        *((STRPTR *) tag->ti_Data) = unit->hu_ProductName;
        count++;
    }
    if((tag = FindTagItem(UHA_Description, taglist)))
    {
        *((STRPTR *) tag->ti_Data) = "Generic adaptive host controller driver for PCI cards";
        count++;
    }
    if((tag = FindTagItem(UHA_Copyright, taglist)))
    {
        *((STRPTR *) tag->ti_Data) ="©2007-2009 Chris Hodges";
        count++;
    }
    if((tag = FindTagItem(UHA_Version, taglist)))
    {
        *((ULONG *) tag->ti_Data) = VERSION_NUMBER;
        count++;
    }
    if((tag = FindTagItem(UHA_Revision, taglist)))
    {
        *((ULONG *) tag->ti_Data) = REVISION_NUMBER;
        count++;
    }
    if((tag = FindTagItem(UHA_DriverVersion, taglist)))
    {
        *((ULONG *) tag->ti_Data) = 0x220;
        count++;
    }
    if((tag = FindTagItem(UHA_Capabilities, taglist)))
    {
        *((ULONG *) tag->ti_Data) = UHCF_USB20;
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
    BOOL cmdgood;
    ULONG cnt;

    if(ioreq->iouh_Endpoint)
    {
        return(UHIOERR_STALL);
    }

    if(len != ioreq->iouh_Length)
    {
        KPRINTF(20, ("RH: Len (%ld != %ld) mismatch!\n", len != ioreq->iouh_Length));
        return(UHIOERR_STALL);
    }
    switch(rt)
    {
        case (URTF_STANDARD|URTF_DEVICE):
            switch(req)
            {
                case USR_SET_ADDRESS:
                    KPRINTF(1, ("RH: SetAddress = %ld\n", val));
                    unit->hu_RootHubAddr = val;
                    ioreq->iouh_Actual = len;
                    return(0);

                case USR_SET_CONFIGURATION:
                    KPRINTF(1, ("RH: SetConfiguration=%ld\n", val));
                    ioreq->iouh_Actual = len;
                    return(0);
            }
            break;

        case (URTF_IN|URTF_STANDARD|URTF_DEVICE):
            switch(req)
            {
                case USR_GET_DESCRIPTOR:
                    switch(val>>8)
                    {
                        case UDT_DEVICE:
                            KPRINTF(1, ("RH: GetDeviceDescriptor (%ld)\n", len));
                            ioreq->iouh_Actual = (len > sizeof(struct UsbStdDevDesc)) ? sizeof(struct UsbStdDevDesc) : len;
                            CopyMem((APTR) &RHDevDesc, ioreq->iouh_Data, ioreq->iouh_Actual);
                            if(ioreq->iouh_Length >= sizeof(struct UsbStdDevDesc))
                            {
                                if(unit->hu_RootHub20Ports)
                                {
                                    struct UsbStdDevDesc *usdd = (struct UsbStdDevDesc *) ioreq->iouh_Data;
                                    usdd->bcdUSB = AROS_WORD2LE(0x0200); // signal a highspeed root hub
                                    usdd->bDeviceProtocol = 1; // single TT
                                }
                                #if (AROS_USB30_CODE)
                                if(unit->hu_RootHub30Ports)
                                {
                                    struct UsbStdDevDesc *usdd = (struct UsbStdDevDesc *) ioreq->iouh_Data;
                                    usdd->bcdUSB = AROS_WORD2LE(0x0300); // signal a superspeed root hub
                                    usdd->bDeviceProtocol = 3;
                                }
                                #endif
                            }
                            return(0);

                        case UDT_CONFIGURATION:
                        {
                            UBYTE tmpbuf[9+9+7];
                            KPRINTF(1, ("RH: GetConfigDescriptor (%ld)\n", len));
                            CopyMem((APTR) &RHCfgDesc, tmpbuf, 9);
                            CopyMem((APTR) &RHIfDesc, &tmpbuf[9], 9);
                            CopyMem((APTR) &RHEPDesc, &tmpbuf[9+9], 7);
                            if(unit->hu_RootHub20Ports)
                            {
                                struct UsbStdEPDesc *usepd = (struct UsbStdEPDesc *) &tmpbuf[9+9];
                                usepd->bInterval = 12; // 2048 µFrames
                            }
                            ioreq->iouh_Actual = (len > 9+9+7) ? 9+9+7 : len;
                            CopyMem(tmpbuf, ioreq->iouh_Data, ioreq->iouh_Actual);
                            return(0);
                        }

                        case UDT_STRING:
                            if(val & 0xff) /* get lang array */
                            {
                                CONST_STRPTR source = NULL;
                                UWORD *mptr = ioreq->iouh_Data;
                                UWORD slen = 1;
                                KPRINTF(1, ("RH: GetString %04lx (%ld)\n", val, len));
                                if((val & 0xff) > 4) /* index too high? */
                                {
                                    return(UHIOERR_STALL);
                                }
                                source = RHStrings[(val & 0xff)-1];
                                if(len > 1)
                                {
                                    ioreq->iouh_Actual = 2;
                                    while(*source++)
                                    {
                                        slen++;
                                    }
                                    source = RHStrings[(val & 0xff)-1];
                                    *mptr++ = AROS_WORD2BE((slen<<9)|UDT_STRING);
                                    while(ioreq->iouh_Actual+1 < len)
                                    {
                                        // special hack for unit number in root hub string
                                        if(((val & 0xff) == 2) && (source[1] == 0))
                                        {
                                            *mptr++ = AROS_WORD2LE('0' + unit->hu_UnitNo);
                                        } else {
                                            *mptr++ = AROS_WORD2LE(*source);
                                        }
                                        source++;
                                        ioreq->iouh_Actual += 2;
                                        if(!(*source))
                                        {
                                            break;
                                        }
                                    }
                                }
                            } else {
                                UWORD *mptr = ioreq->iouh_Data;
                                KPRINTF(1, ("RH: GetLangArray %04lx (%ld)\n", val, len));
                                if(len > 1)
                                {
                                   ioreq->iouh_Actual = 2;
                                   mptr[0] = AROS_WORD2BE((4<<8)|UDT_STRING);
                                   if(len > 3)
                                   {
                                      ioreq->iouh_Actual += 2;
                                      mptr[1] = AROS_WORD2LE(0x0409);
                                   }
                                }
                            }
                            return(0);

                        default:
                            KPRINTF(1, ("RH: Unsupported Descriptor %04lx\n", idx));
                    }
                    break;

                case USR_GET_CONFIGURATION:
                    if(len == 1)
                    {
                        KPRINTF(1, ("RH: GetConfiguration\n"));
                        ((UBYTE *) ioreq->iouh_Data)[0] = 1;
                        ioreq->iouh_Actual = len;
                        return(0);
                    }
                    break;
            }
            break;

        case (URTF_CLASS|URTF_OTHER):
            switch(req)
            {
                case USR_SET_FEATURE:
                    if((!idx) && (idx > numports))
                    {
                        KPRINTF(20, ("Port %ld out of range\n", idx));
                        return(UHIOERR_STALL);
                    }
                    chc = unit->hu_PortMap11[idx - 1];
                    if(unit->hu_EhciOwned[idx - 1])
                    {
                        hc = unit->hu_PortMap20[idx - 1];
                        hciport = idx - 1;
                    } else {
                        hc = chc;
                        hciport = unit->hu_PortNum11[idx - 1];
                    }
                    KPRINTF(10, ("Set Feature %ld maps from glob. Port %ld to local Port %ld (%s)\n", val, idx, hciport, unit->hu_EhciOwned[idx - 1] ? "EHCI" : "U/OHCI"));
                    cmdgood = FALSE;
                    switch(hc->hc_HCIType)
                    {
                        case HCITYPE_UHCI:
                        {
                            UWORD portreg = hciport ? UHCI_PORT2STSCTRL : UHCI_PORT1STSCTRL;
                            ULONG oldval = READIO16_LE(hc->hc_RegBase, portreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE); // these are clear-on-write!
                            ULONG newval = oldval;
                            switch(val)
                            {
                                /* case UFS_PORT_CONNECTION: not possible */
                                case UFS_PORT_ENABLE:
                                    KPRINTF(10, ("Enabling Port (%s)\n", newval & UHPF_PORTENABLE ? "already" : "ok"));
                                    newval |= UHPF_PORTENABLE;
                                    cmdgood = TRUE;
                                    break;

                                case UFS_PORT_SUSPEND:
                                    newval |= UHPF_PORTSUSPEND;
                                    hc->hc_PortChangeMap[hciport] |= UPSF_PORT_SUSPEND; // manually fake suspend change
                                    cmdgood = TRUE;
                                    break;

                                /* case UFS_PORT_OVER_CURRENT: not possible */
                                case UFS_PORT_RESET:
                                    KPRINTF(10, ("Resetting Port (%s)\n", newval & UHPF_PORTRESET ? "already" : "ok"));

                                    // this is an ugly blocking workaround to the inability of UHCI to clear reset automatically
                                    newval &= ~(UHPF_PORTSUSPEND|UHPF_PORTENABLE);
                                    newval |= UHPF_PORTRESET;
                                    WRITEIO16_LE(hc->hc_RegBase, portreg, newval);
                                    uhwDelayMS(25, unit);
                                    newval = READIO16_LE(hc->hc_RegBase, portreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE|UHPF_PORTSUSPEND|UHPF_PORTENABLE);
                                    KPRINTF(10, ("Reset=%s\n", newval & UHPF_PORTRESET ? "GOOD" : "BAD!"));
                                    // like windows does it
                                    newval &= ~UHPF_PORTRESET;
                                    WRITEIO16_LE(hc->hc_RegBase, portreg, newval);
                                    uhwDelayMicro(50, unit);
                                    newval = READIO16_LE(hc->hc_RegBase, portreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE|UHPF_PORTSUSPEND);
                                    KPRINTF(10, ("Reset=%s\n", newval & UHPF_PORTRESET ? "BAD!" : "GOOD"));
                                    newval &= ~(UHPF_PORTSUSPEND|UHPF_PORTRESET);
                                    newval |= UHPF_PORTENABLE;
                                    WRITEIO16_LE(hc->hc_RegBase, portreg, newval);
                                    hc->hc_PortChangeMap[hciport] |= UPSF_PORT_RESET|UPSF_PORT_ENABLE; // manually fake reset change

                                    cnt = 100;
                                    do
                                    {
                                        uhwDelayMS(1, unit);
                                        newval = READIO16_LE(hc->hc_RegBase, portreg);
                                    } while(--cnt && (!(newval & UHPF_PORTENABLE)));
                                    if(cnt)
                                    {
                                        KPRINTF(10, ("Enabled after %ld ticks\n", 100-cnt));
                                    } else {
                                        KPRINTF(20, ("Port refuses to be enabled!\n"));
                                        return(UHIOERR_HOSTERROR);
                                    }
                                    // make enumeration possible
                                    unit->hu_DevControllers[0] = hc;
                                    cmdgood = TRUE;
                                    break;

                                case UFS_PORT_POWER:
                                    KPRINTF(10, ("Powering Port\n"));
                                    // ignore for UHCI, is always powered
                                    cmdgood = TRUE;
                                    break;

                                /* case UFS_PORT_LOW_SPEED: not possible */
                                /* case UFS_C_PORT_CONNECTION:
                                case UFS_C_PORT_ENABLE:
                                case UFS_C_PORT_SUSPEND:
                                case UFS_C_PORT_OVER_CURRENT:
                                case UFS_C_PORT_RESET: */
                            }
                            if(cmdgood)
                            {
                                KPRINTF(5, ("Port %ld SET_FEATURE %04lx->%04lx\n", idx, oldval, newval));
                                WRITEIO16_LE(hc->hc_RegBase, portreg, newval);
                                return(0);
                            }
                            break;
                        }

                        case HCITYPE_OHCI:
                        {
                            UWORD portreg = OHCI_PORTSTATUS + (hciport<<2);
                            ULONG oldval = READREG32_LE(hc->hc_RegBase, portreg);

                            switch(val)
                            {
                                /* case UFS_PORT_CONNECTION: not possible */
                                case UFS_PORT_ENABLE:
                                    KPRINTF(10, ("Enabling Port (%s)\n", oldval & OHPF_PORTENABLE ? "already" : "ok"));
                                    WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTENABLE);
                                    cmdgood = TRUE;
                                    break;

                                case UFS_PORT_SUSPEND:
                                    KPRINTF(10, ("Suspending Port (%s)\n", oldval & OHPF_PORTSUSPEND ? "already" : "ok"));
                                    //hc->hc_PortChangeMap[hciport] |= UPSF_PORT_SUSPEND; // manually fake suspend change
                                    WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTSUSPEND);
                                    cmdgood = TRUE;
                                    break;

                                /* case UFS_PORT_OVER_CURRENT: not possible */
                                case UFS_PORT_RESET:
                                    KPRINTF(10, ("Resetting Port (%s)\n", oldval & OHPF_PORTRESET ? "already" : "ok"));
                                    // make sure we have at least 50ms of reset time here, as required for a root hub port
                                    WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTRESET);
                                    uhwDelayMS(10, unit);
                                    WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTRESET);
                                    uhwDelayMS(10, unit);
                                    WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTRESET);
                                    uhwDelayMS(10, unit);
                                    WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTRESET);
                                    uhwDelayMS(10, unit);
                                    WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTRESET);
                                    uhwDelayMS(15, unit);
                                    oldval = READREG32_LE(hc->hc_RegBase, portreg);
                                    KPRINTF(10, ("OHCI Reset release (%s %s)\n", oldval & OHPF_PORTRESET ? "didn't turn off" : "okay",
                                                                                 oldval & OHPF_PORTENABLE ? "enabled" : "not enabled"));
                                    if(oldval & OHPF_PORTRESET)
                                    {
                                         uhwDelayMS(40, unit);
                                         oldval = READREG32_LE(hc->hc_RegBase, portreg);
                                         KPRINTF(10, ("OHCI Reset 2nd release (%s %s)\n", oldval & OHPF_PORTRESET ? "didn't turn off" : "okay",
                                                                                          oldval & OHPF_PORTENABLE ? "enabled" : "still not enabled"));
                                    }
                                    // make enumeration possible
                                    unit->hu_DevControllers[0] = hc;
                                    cmdgood = TRUE;
                                    break;

                                case UFS_PORT_POWER:
                                    KPRINTF(10, ("Powering Port (%s)\n", oldval & OHPF_PORTPOWER ? "already" : "ok"));
                                    WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTPOWER);
                                    cmdgood = TRUE;
                                    break;

                                /* case UFS_PORT_LOW_SPEED: not possible */
                                /* case UFS_C_PORT_CONNECTION:
                                case UFS_C_PORT_ENABLE:
                                case UFS_C_PORT_SUSPEND:
                                case UFS_C_PORT_OVER_CURRENT:
                                case UFS_C_PORT_RESET: */
                            }
                            if(cmdgood)
                            {
                                return(0);
                            }
                            break;
                        }

                        case HCITYPE_EHCI:
                        {
                            UWORD portreg = EHCI_PORTSC1 + (hciport<<2);
                            ULONG oldval = READREG32_LE(hc->hc_RegBase, portreg) & ~(EHPF_OVERCURRENTCHG|EHPF_ENABLECHANGE|EHPF_CONNECTCHANGE); // these are clear-on-write!
                            ULONG newval = oldval;
                            switch(val)
                            {
                                /* case UFS_PORT_CONNECTION: not possible */
                                case UFS_PORT_ENABLE:
                                    KPRINTF(10, ("Enabling Port (%s)\n", newval & EHPF_PORTENABLE ? "already" : "ok"));
                                    newval |= EHPF_PORTENABLE;
                                    cmdgood = TRUE;
                                    break;

                                case UFS_PORT_SUSPEND:
                                    newval |= EHPF_PORTSUSPEND;
                                    hc->hc_PortChangeMap[hciport] |= UPSF_PORT_SUSPEND; // manually fake suspend change
                                    cmdgood = TRUE;
                                    break;

                                /* case UFS_PORT_OVER_CURRENT: not possible */
                                case UFS_PORT_RESET:
                                    KPRINTF(10, ("Resetting Port (%s)\n", newval & EHPF_PORTRESET ? "already" : "ok"));

                                    // this is an ugly blocking workaround to the inability of EHCI to clear reset automatically
                                    newval &= ~(EHPF_PORTSUSPEND|EHPF_PORTENABLE);
                                    newval |= EHPF_PORTRESET;
                                    WRITEREG32_LE(hc->hc_RegBase, portreg, newval);
                                    uhwDelayMS(50, unit);
                                    newval = READREG32_LE(hc->hc_RegBase, portreg) & ~(EHPF_OVERCURRENTCHG|EHPF_ENABLECHANGE|EHPF_CONNECTCHANGE|EHPF_PORTSUSPEND|EHPF_PORTENABLE);
                                    KPRINTF(10, ("Reset=%s\n", newval & EHPF_PORTRESET ? "GOOD" : "BAD!"));
                                    newval &= ~EHPF_PORTRESET;
                                    WRITEREG32_LE(hc->hc_RegBase, portreg, newval);
                                    uhwDelayMS(10, unit);
                                    newval = READREG32_LE(hc->hc_RegBase, portreg) & ~(EHPF_OVERCURRENTCHG|EHPF_ENABLECHANGE|EHPF_CONNECTCHANGE|EHPF_PORTSUSPEND);
                                    KPRINTF(10, ("Reset=%s\n", newval & EHPF_PORTRESET ? "BAD!" : "GOOD"));
                                    KPRINTF(10, ("Highspeed=%s\n", newval & EHPF_PORTENABLE ? "YES!" : "NO"));
                                    KPRINTF(10, ("EHCI Port status=%08lx\n", newval));
                                    if(!(newval & EHPF_PORTENABLE))
                                    {
                                        // if not highspeed, release ownership
                                        KPRINTF(20, ("Transferring ownership to UHCI/OHCI port %ld\n", unit->hu_PortNum11[idx - 1]));
                                        KPRINTF(10, ("Device is %s\n", newval & EHPF_LINESTATUS_DM ? "LOWSPEED" : "FULLSPEED"));
                                        newval |= EHPF_NOTPORTOWNER;
                                        if(!chc)
                                        {
                                            KPRINTF(20, ("EHCI has no companion controller, can't transfer ownership!\n"));
                                            WRITEREG32_LE(hc->hc_RegBase, portreg, newval);
                                            return(UHIOERR_HOSTERROR);
                                        }
                                        switch(chc->hc_HCIType)
                                        {
                                            case HCITYPE_UHCI:
                                            {
                                                UWORD uhcihciport = unit->hu_PortNum11[idx - 1];
                                                UWORD uhciportreg = uhcihciport ? UHCI_PORT2STSCTRL : UHCI_PORT1STSCTRL;
                                                ULONG __unused uhcinewval = READREG16_LE(chc->hc_RegBase, uhciportreg);

                                                KPRINTF(10, ("UHCI Port status before handover=%04lx\n", uhcinewval));
                                                break;
                                            }

                                            case HCITYPE_OHCI:
                                            {
                                                UWORD ohcihciport = unit->hu_PortNum11[idx - 1];
                                                UWORD ohciportreg = OHCI_PORTSTATUS + (ohcihciport<<2);
                                                ULONG __unused ohcioldval = READREG32_LE(chc->hc_RegBase, ohciportreg);

                                                KPRINTF(10, ("OHCI Port status before handover=%08lx\n", ohcioldval));
                                                KPRINTF(10, ("Powering Port (%s)\n", ohcioldval & OHPF_PORTPOWER ? "already" : "ok"));
                                                WRITEREG32_LE(chc->hc_RegBase, ohciportreg, OHPF_PORTPOWER);
                                                uhwDelayMS(10, unit);
                                                KPRINTF(10, ("OHCI Port status after handover=%08lx\n", READREG32_LE(chc->hc_RegBase, ohciportreg)));
                                                break;
                                            }
                                        }
                                        newval = READREG32_LE(hc->hc_RegBase, portreg) & ~(EHPF_OVERCURRENTCHG|EHPF_ENABLECHANGE|EHPF_CONNECTCHANGE|EHPF_PORTSUSPEND);
                                        KPRINTF(10, ("EHCI Port status (reread)=%08lx\n", newval));
                                        newval |= EHPF_NOTPORTOWNER;
                                        unit->hu_EhciOwned[idx - 1] = FALSE;
                                        WRITEREG32_LE(hc->hc_RegBase, portreg, newval);
                                        uhwDelayMS(90, unit);
                                        KPRINTF(10, ("EHCI Port status (after handover)=%08lx\n", READREG32_LE(hc->hc_RegBase, portreg) & ~(EHPF_OVERCURRENTCHG|EHPF_ENABLECHANGE|EHPF_CONNECTCHANGE|EHPF_PORTSUSPEND)));
                                        // enable companion controller port
                                        switch(chc->hc_HCIType)
                                        {
                                            case HCITYPE_UHCI:
                                            {
                                                UWORD uhcihciport = unit->hu_PortNum11[idx - 1];
                                                UWORD uhciportreg = uhcihciport ? UHCI_PORT2STSCTRL : UHCI_PORT1STSCTRL;
                                                ULONG uhcinewval;

                                                uhcinewval = READIO16_LE(chc->hc_RegBase, uhciportreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE|UHPF_PORTSUSPEND);
                                                KPRINTF(10, ("UHCI Reset=%s\n", uhcinewval & UHPF_PORTRESET ? "BAD!" : "GOOD"));
                                                if((uhcinewval & UHPF_PORTRESET))//|| (newval & EHPF_LINESTATUS_DM))
                                                {
                                                    // this is an ugly blocking workaround to the inability of UHCI to clear reset automatically
                                                    KPRINTF(20, ("Uhm, UHCI reset was bad!\n"));
                                                    uhcinewval &= ~(UHPF_PORTSUSPEND|UHPF_PORTENABLE);
                                                    uhcinewval |= UHPF_PORTRESET;
                                                    WRITEIO16_LE(chc->hc_RegBase, uhciportreg, uhcinewval);
                                                    uhwDelayMS(50, unit);
                                                    uhcinewval = READIO16_LE(chc->hc_RegBase, uhciportreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE|UHPF_PORTSUSPEND|UHPF_PORTENABLE);
                                                    KPRINTF(10, ("ReReset=%s\n", uhcinewval & UHPF_PORTRESET ? "GOOD" : "BAD!"));
                                                    uhcinewval &= ~UHPF_PORTRESET;
                                                    WRITEIO16_LE(chc->hc_RegBase, uhciportreg, uhcinewval);
                                                    uhwDelayMicro(50, unit);
                                                    uhcinewval = READIO16_LE(chc->hc_RegBase, uhciportreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE|UHPF_PORTSUSPEND);
                                                    KPRINTF(10, ("ReReset=%s\n", uhcinewval & UHPF_PORTRESET ? "STILL BAD!" : "GOOD"));
                                                }
                                                uhcinewval &= ~UHPF_PORTRESET;
                                                uhcinewval |= UHPF_PORTENABLE;
                                                WRITEIO16_LE(chc->hc_RegBase, uhciportreg, uhcinewval);
                                                chc->hc_PortChangeMap[uhcihciport] |= UPSF_PORT_RESET|UPSF_PORT_ENABLE; // manually fake reset change
                                                uhwDelayMS(5, unit);
                                                cnt = 100;
                                                do
                                                {
                                                    uhwDelayMS(1, unit);
                                                    uhcinewval = READIO16_LE(chc->hc_RegBase, uhciportreg);
                                                } while(--cnt && (!(uhcinewval & UHPF_PORTENABLE)));
                                                if(cnt)
                                                {
                                                    KPRINTF(10, ("Enabled after %ld ticks\n", 100-cnt));
                                                } else {
                                                    KPRINTF(20, ("Port refuses to be enabled!\n"));
                                                    return(UHIOERR_HOSTERROR);
                                                }
                                                break;
                                            }

                                            case HCITYPE_OHCI:
                                            {
                                                UWORD ohcihciport = unit->hu_PortNum11[idx - 1];
                                                UWORD ohciportreg = OHCI_PORTSTATUS + (ohcihciport<<2);
                                                ULONG ohcioldval = READREG32_LE(chc->hc_RegBase, ohciportreg);
                                                KPRINTF(10, ("OHCI Resetting Port (%s)\n", ohcioldval & OHPF_PORTRESET ? "already" : "ok"));
                                                // make sure we have at least 50ms of reset time here, as required for a root hub port
                                                WRITEREG32_LE(chc->hc_RegBase, ohciportreg, OHPF_PORTRESET);
                                                uhwDelayMS(10, unit);
                                                WRITEREG32_LE(chc->hc_RegBase, ohciportreg, OHPF_PORTRESET);
                                                uhwDelayMS(10, unit);
                                                WRITEREG32_LE(chc->hc_RegBase, ohciportreg, OHPF_PORTRESET);
                                                uhwDelayMS(10, unit);
                                                WRITEREG32_LE(chc->hc_RegBase, ohciportreg, OHPF_PORTRESET);
                                                uhwDelayMS(10, unit);
                                                WRITEREG32_LE(chc->hc_RegBase, ohciportreg, OHPF_PORTRESET);
                                                uhwDelayMS(15, unit);
                                                ohcioldval = READREG32_LE(chc->hc_RegBase, ohciportreg);
                                                KPRINTF(10, ("OHCI Reset release (%s %s)\n", ohcioldval & OHPF_PORTRESET ? "didn't turn off" : "okay",
                                                                                             ohcioldval & OHPF_PORTENABLE ? "enabled" : "not enabled"));
                                                if(ohcioldval & OHPF_PORTRESET)
                                                {
                                                    uhwDelayMS(40, unit);
                                                    ohcioldval = READREG32_LE(chc->hc_RegBase, ohciportreg);
                                                    KPRINTF(10, ("OHCI Reset 2nd release (%s %s)\n", ohcioldval & OHPF_PORTRESET ? "didn't turn off" : "okay",
                                                                                                     ohcioldval & OHPF_PORTENABLE ? "enabled" : "still not enabled"));
                                                }
                                                break;
                                            }

                                        }
                                        // make enumeration possible
                                        unit->hu_DevControllers[0] = chc;
                                        return(0);
                                    } else {
                                        newval &= ~EHPF_PORTRESET;
                                        WRITEREG16_LE(hc->hc_RegBase, portreg, newval);
                                        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_RESET; // manually fake reset change
                                        uhwDelayMS(10, unit);
                                        cnt = 100;
                                        do
                                        {
                                            uhwDelayMS(1, unit);
                                            newval = READREG32_LE(hc->hc_RegBase, portreg);
                                        } while(--cnt && (!(newval & EHPF_PORTENABLE)));
                                        if(cnt)
                                        {
                                            KPRINTF(10, ("Enabled after %ld ticks\n", 100-cnt));
                                        } else {
                                            KPRINTF(20, ("Port refuses to be enabled!\n"));
                                            return(UHIOERR_HOSTERROR);
                                        }
                                        // make enumeration possible
                                        unit->hu_DevControllers[0] = hc;
                                    }
                                    cmdgood = TRUE;
                                    break;

                                case UFS_PORT_POWER:
                                    KPRINTF(10, ("Powering Port\n"));
                                    newval |= EHPF_PORTPOWER;
                                    cmdgood = TRUE;
                                    break;

                                /* case UFS_PORT_LOW_SPEED: not possible */
                                /* case UFS_C_PORT_CONNECTION:
                                case UFS_C_PORT_ENABLE:
                                case UFS_C_PORT_SUSPEND:
                                case UFS_C_PORT_OVER_CURRENT:
                                case UFS_C_PORT_RESET: */
                            }
                            if(cmdgood)
                            {
                                KPRINTF(5, ("Port %ld SET_FEATURE %04lx->%04lx\n", idx, oldval, newval));
                                WRITEREG32_LE(hc->hc_RegBase, portreg, newval);
                                return(0);
                            }
                            break;
                        }

                        #if (AROS_USB30_CODE)
                        /* (URTF_CLASS|URTF_OTHER) USR_SET_FEATURE */
                        case HCITYPE_XHCI:
                        {
                            cmdgood = TRUE;
                            if(cmdgood)
                            {
                                KPRINTF(1000, ("XHCI (URTF_CLASS|URTF_OTHER) USR_SET_FEATURE\n"));
                                return(0);
                            }
                            break;
                        }
                        #endif

                    }
                    break;

                case USR_CLEAR_FEATURE:
                    if((!idx) && (idx > numports))
                    {
                        KPRINTF(20, ("Port %ld out of range\n", idx));
                        return(UHIOERR_STALL);
                    }
                    if(unit->hu_EhciOwned[idx - 1])
                    {
                        hc = unit->hu_PortMap20[idx - 1];
                        hciport = idx - 1;
                    } else {
                        hc = unit->hu_PortMap11[idx - 1];
                        hciport = unit->hu_PortNum11[idx - 1];
                    }
                    KPRINTF(10, ("Clear Feature %ld maps from glob. Port %ld to local Port %ld (%s)\n", val, idx, hciport, unit->hu_EhciOwned[idx - 1] ? "EHCI" : "U/OHCI"));
                    cmdgood = FALSE;
                    switch(hc->hc_HCIType)
                    {
                        case HCITYPE_UHCI:
                        {
                            UWORD portreg = hciport ? UHCI_PORT2STSCTRL : UHCI_PORT1STSCTRL;
                            ULONG oldval = READIO16_LE(hc->hc_RegBase, portreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE); // these are clear-on-write!
                            ULONG newval = oldval;
                            switch(val)
                            {
                                case UFS_PORT_ENABLE:
                                    KPRINTF(10, ("Disabling Port (%s)\n", newval & UHPF_PORTENABLE ? "ok" : "already"));
                                    newval &= ~UHPF_PORTENABLE;
                                    cmdgood = TRUE;
                                    // disable enumeration
                                    unit->hu_DevControllers[0] = NULL;
                                    break;

                                case UFS_PORT_SUSPEND:
                                    newval &= ~UHPF_PORTSUSPEND;
                                    cmdgood = TRUE;
                                    break;

                                case UFS_PORT_POWER: // ignore for UHCI, there's no power control here
                                    KPRINTF(10, ("Disabling Power\n"));
                                    KPRINTF(10, ("Disabling Port (%s)\n", newval & UHPF_PORTENABLE ? "ok" : "already"));
                                    newval &= ~UHPF_PORTENABLE;
                                    cmdgood = TRUE;
                                    break;

                                case UFS_C_PORT_CONNECTION:
                                    newval |= UHPF_CONNECTCHANGE; // clear-on-write!
                                    hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_CONNECTION;
                                    cmdgood = TRUE;
                                    break;

                                case UFS_C_PORT_ENABLE:
                                    newval |= UHPF_ENABLECHANGE; // clear-on-write!
                                    hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_ENABLE;
                                    cmdgood = TRUE;
                                    break;

                                case UFS_C_PORT_SUSPEND: // ignore for UHCI, there's no bit indicating this
                                    hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_SUSPEND; // manually fake suspend change clearing
                                    cmdgood = TRUE;
                                    break;

                                case UFS_C_PORT_OVER_CURRENT: // ignore for UHCI, there's no bit indicating this
                                    hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_OVER_CURRENT; // manually fake over current clearing
                                    cmdgood = TRUE;
                                    break;

                                case UFS_C_PORT_RESET: // ignore for UHCI, there's no bit indicating this
                                    hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_RESET; // manually fake reset change clearing
                                    cmdgood = TRUE;
                                    break;
                            }
                            if(cmdgood)
                            {
                                KPRINTF(5, ("Port %ld CLEAR_FEATURE %04lx->%04lx\n", idx, oldval, newval));
                                WRITEIO16_LE(hc->hc_RegBase, portreg, newval);
                                if(hc->hc_PortChangeMap[hciport])
                                {
                                    unit->hu_RootPortChanges |= 1UL<<idx;
                                } else {
                                    unit->hu_RootPortChanges &= ~(1UL<<idx);
                                }
                                return(0);
                            }
                            break;
                        }

                        case HCITYPE_OHCI:
                        {
                            UWORD portreg = OHCI_PORTSTATUS + (hciport<<2);
                            ULONG __unused oldval = READREG32_LE(hc->hc_RegBase, portreg);

                            switch(val)
                            {
                                case UFS_PORT_ENABLE:
                                    KPRINTF(10, ("Disabling Port (%s)\n", oldval & OHPF_PORTENABLE ? "ok" : "already"));
                                    WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTDISABLE);
                                    cmdgood = TRUE;
                                    break;

                                case UFS_PORT_SUSPEND:
                                    KPRINTF(10, ("Resuming Port (%s)\n", oldval & OHPF_PORTSUSPEND ? "ok" : "already"));
                                    //hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_SUSPEND; // manually fake suspend change
                                    WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_RESUME);
                                    cmdgood = TRUE;
                                    break;

                                case UFS_PORT_POWER:
                                    KPRINTF(10, ("Unpowering Port (%s)\n", oldval & OHPF_PORTPOWER ? "ok" : "already"));
                                    WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTUNPOWER);
                                    cmdgood = TRUE;
                                    break;

                               case UFS_C_PORT_CONNECTION:
                                    WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_CONNECTCHANGE);
                                    hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_CONNECTION;
                                    cmdgood = TRUE;
                                    break;

                                case UFS_C_PORT_ENABLE:
                                    WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_ENABLECHANGE);
                                    hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_ENABLE;
                                    cmdgood = TRUE;
                                    break;

                                case UFS_C_PORT_SUSPEND:
                                    WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_RESUMEDTX);
                                    hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_SUSPEND;
                                    cmdgood = TRUE;
                                    break;

                                case UFS_C_PORT_OVER_CURRENT:
                                    WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_OVERCURRENTCHG);
                                    hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_OVER_CURRENT;
                                    cmdgood = TRUE;
                                    break;

                                case UFS_C_PORT_RESET:
                                    WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_RESETCHANGE);
                                    hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_RESET;
                                    cmdgood = TRUE;
                                    break;
                            }
                            if(cmdgood)
                            {
                                return(0);
                            }
                            break;
                        }

                        case HCITYPE_EHCI:
                        {
                            UWORD portreg = EHCI_PORTSC1 + (hciport<<2);
                            ULONG oldval = READREG32_LE(hc->hc_RegBase, portreg) & ~(EHPF_OVERCURRENTCHG|EHPF_ENABLECHANGE|EHPF_CONNECTCHANGE); // these are clear-on-write!
                            ULONG newval = oldval;
                            switch(val)
                            {
                                case UFS_PORT_ENABLE:
                                    KPRINTF(10, ("Disabling Port (%s)\n", newval & EHPF_PORTENABLE ? "ok" : "already"));
                                    newval &= ~EHPF_PORTENABLE;
                                    cmdgood = TRUE;
                                    // disable enumeration
                                    unit->hu_DevControllers[0] = NULL;
                                    break;

                                case UFS_PORT_SUSPEND:
                                    newval &= ~EHPF_PORTSUSPEND;
                                    cmdgood = TRUE;
                                    break;

                                case UFS_PORT_POWER: // ignore for UHCI, there's no power control here
                                    KPRINTF(10, ("Disabling Power (%s)\n", newval & EHPF_PORTPOWER ? "ok" : "already"));
                                    KPRINTF(10, ("Disabling Port (%s)\n", newval & EHPF_PORTENABLE ? "ok" : "already"));
                                    newval &= ~(EHPF_PORTENABLE|EHPF_PORTPOWER);
                                    cmdgood = TRUE;
                                    break;

                                case UFS_C_PORT_CONNECTION:
                                    newval |= EHPF_CONNECTCHANGE; // clear-on-write!
                                    hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_CONNECTION;
                                    cmdgood = TRUE;
                                    break;

                                case UFS_C_PORT_ENABLE:
                                    newval |= EHPF_ENABLECHANGE; // clear-on-write!
                                    hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_ENABLE;
                                    cmdgood = TRUE;
                                    break;

                                case UFS_C_PORT_SUSPEND: // ignore for EHCI, there's no bit indicating this
                                    hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_SUSPEND; // manually fake suspend change clearing
                                    cmdgood = TRUE;
                                    break;

                                case UFS_C_PORT_OVER_CURRENT:
                                    newval |= EHPF_OVERCURRENTCHG; // clear-on-write!
                                    hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_OVER_CURRENT; // manually fake over current clearing
                                    cmdgood = TRUE;
                                    break;

                                case UFS_C_PORT_RESET: // ignore for EHCI, there's no bit indicating this
                                    hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_RESET; // manually fake reset change clearing
                                    cmdgood = TRUE;
                                    break;
                            }
                            if(cmdgood)
                            {
                                KPRINTF(5, ("Port %ld CLEAR_FEATURE %08lx->%08lx\n", idx, oldval, newval));
                                WRITEREG32_LE(hc->hc_RegBase, portreg, newval);
                                if(hc->hc_PortChangeMap[hciport])
                                {
                                    unit->hu_RootPortChanges |= 1UL<<idx;
                                } else {
                                    unit->hu_RootPortChanges &= ~(1UL<<idx);
                                }
                                return(0);
                            }
                            break;
                        }

                        #if (AROS_USB30_CODE)
                        /* (URTF_CLASS|URTF_OTHER) USR_CLEAR_FEATURE */
                        case HCITYPE_XHCI:
                        {
                            cmdgood = TRUE;
                            if(cmdgood)
                            {
                                KPRINTF(1000, ("XHCI (URTF_CLASS|URTF_OTHER) USR_CLEAR_FEATURE\n"));
                                return(0);
                            }
                            break;
                        }
                        #endif

                    }
                    break;
            }
            break;

        case (URTF_IN|URTF_CLASS|URTF_OTHER):
            switch(req)
            {
                case USR_GET_STATUS:
                {
                    UWORD *mptr = ioreq->iouh_Data;
                    if(len != sizeof(struct UsbPortStatus))
                    {
                        return(UHIOERR_STALL);
                    }
                    if((!idx) && (idx > numports))
                    {
                        KPRINTF(20, ("Port %ld out of range\n", idx));
                        return(UHIOERR_STALL);
                    }
                    if(unit->hu_EhciOwned[idx - 1])
                    {
                        hc = unit->hu_PortMap20[idx - 1];
                        hciport = idx - 1;
                    } else {
                        hc = unit->hu_PortMap11[idx - 1];
                        hciport = unit->hu_PortNum11[idx - 1];
                    }
                    switch(hc->hc_HCIType)
                    {
                        case HCITYPE_UHCI:
                        {
                            UWORD portreg = hciport ? UHCI_PORT2STSCTRL : UHCI_PORT1STSCTRL;
                            UWORD oldval = READIO16_LE(hc->hc_RegBase, portreg);
                            *mptr = AROS_WORD2LE(UPSF_PORT_POWER);
                            if(oldval & UHPF_PORTCONNECTED) *mptr |= AROS_WORD2LE(UPSF_PORT_CONNECTION);
                            if(oldval & UHPF_PORTENABLE) *mptr |= AROS_WORD2LE(UPSF_PORT_ENABLE);
                            if(oldval & UHPF_LOWSPEED) *mptr |= AROS_WORD2LE(UPSF_PORT_LOW_SPEED);
                            if(oldval & UHPF_PORTRESET) *mptr |= AROS_WORD2LE(UPSF_PORT_RESET);
                            if(oldval & UHPF_PORTSUSPEND) *mptr |= AROS_WORD2LE(UPSF_PORT_SUSPEND);

                            KPRINTF(5, ("UHCI Port %ld is %s\n", idx, oldval & UHPF_LOWSPEED ? "LOWSPEED" : "FULLSPEED"));
                            KPRINTF(5, ("UHCI Port %ld Status %08lx\n", idx, *mptr));

                            mptr++;
                            if(oldval & UHPF_ENABLECHANGE)
                            {
                                hc->hc_PortChangeMap[hciport] |= UPSF_PORT_ENABLE;
                            }
                            if(oldval & UHPF_CONNECTCHANGE)
                            {
                                hc->hc_PortChangeMap[hciport] |= UPSF_PORT_CONNECTION;
                            }
                            if(oldval & UHPF_RESUMEDTX)
                            {
                                hc->hc_PortChangeMap[hciport] |= UPSF_PORT_SUSPEND|UPSF_PORT_ENABLE;
                            }
                            *mptr = AROS_WORD2LE(hc->hc_PortChangeMap[hciport]);
                            WRITEIO16_LE(hc->hc_RegBase, portreg, oldval);
                            KPRINTF(5, ("UHCI Port %ld Change %08lx\n", idx, *mptr));
                            return(0);
                        }

                        case HCITYPE_OHCI:
                        {
                            UWORD portreg = OHCI_PORTSTATUS + (hciport<<2);
                            ULONG oldval = READREG32_LE(hc->hc_RegBase, portreg);

                            *mptr = 0;
                            if(oldval & OHPF_PORTPOWER) *mptr |= AROS_WORD2LE(UPSF_PORT_POWER);
                            if(oldval & OHPF_OVERCURRENT) *mptr |= AROS_WORD2LE(UPSF_PORT_OVER_CURRENT);
                            if(oldval & OHPF_PORTCONNECTED) *mptr |= AROS_WORD2LE(UPSF_PORT_CONNECTION);
                            if(oldval & OHPF_PORTENABLE) *mptr |= AROS_WORD2LE(UPSF_PORT_ENABLE);
                            if(oldval & OHPF_LOWSPEED) *mptr |= AROS_WORD2LE(UPSF_PORT_LOW_SPEED);
                            if(oldval & OHPF_PORTRESET) *mptr |= AROS_WORD2LE(UPSF_PORT_RESET);
                            if(oldval & OHPF_PORTSUSPEND) *mptr |= AROS_WORD2LE(UPSF_PORT_SUSPEND);

                            KPRINTF(5, ("OHCI Port %ld (glob. %ld) is %s\n", hciport, idx, oldval & OHPF_LOWSPEED ? "LOWSPEED" : "FULLSPEED"));
                            KPRINTF(5, ("OHCI Port %ld Status %08lx (%08lx)\n", idx, *mptr, oldval));

                            mptr++;
                            if(oldval & OHPF_OVERCURRENTCHG)
                            {
                                hc->hc_PortChangeMap[hciport] |= UPSF_PORT_OVER_CURRENT;
                            }
                            if(oldval & OHPF_RESETCHANGE)
                            {
                                hc->hc_PortChangeMap[hciport] |= UPSF_PORT_RESET;
                            }
                            if(oldval & OHPF_ENABLECHANGE)
                            {
                                hc->hc_PortChangeMap[hciport] |= UPSF_PORT_ENABLE;
                            }
                            if(oldval & OHPF_CONNECTCHANGE)
                            {
                                hc->hc_PortChangeMap[hciport] |= UPSF_PORT_CONNECTION;
                            }
                            if(oldval & OHPF_RESUMEDTX)
                            {
                                hc->hc_PortChangeMap[hciport] |= UPSF_PORT_SUSPEND;
                            }
                            *mptr = AROS_WORD2LE(hc->hc_PortChangeMap[hciport]);
                            KPRINTF(5, ("OHCI Port %ld Change %08lx\n", idx, *mptr));
                            return(0);
                        }

                        case HCITYPE_EHCI:
                        {
                            UWORD portreg = EHCI_PORTSC1 + (hciport<<2);
                            ULONG oldval = READREG32_LE(hc->hc_RegBase, portreg);

                            *mptr = 0;
                            if(oldval & EHPF_PORTCONNECTED) *mptr |= AROS_WORD2LE(UPSF_PORT_CONNECTION);
                            if(oldval & EHPF_PORTENABLE) *mptr |= AROS_WORD2LE(UPSF_PORT_ENABLE|UPSF_PORT_HIGH_SPEED);
                            if((oldval & (EHPF_LINESTATUS_DM|EHPF_PORTCONNECTED|EHPF_PORTENABLE)) ==
                               (EHPF_LINESTATUS_DM|EHPF_PORTCONNECTED))
                            {
                                KPRINTF(10, ("EHCI Port %ld is LOWSPEED\n", idx));
                                // we need to detect low speed devices prior to reset
                                *mptr |= AROS_WORD2LE(UPSF_PORT_LOW_SPEED);
                            }

                            if(oldval & EHPF_PORTRESET) *mptr |= AROS_WORD2LE(UPSF_PORT_RESET);
                            if(oldval & EHPF_PORTSUSPEND) *mptr |= AROS_WORD2LE(UPSF_PORT_SUSPEND);
                            if(oldval & EHPF_PORTPOWER) *mptr |= AROS_WORD2LE(UPSF_PORT_POWER);
                            if(oldval & EHPM_PORTINDICATOR) *mptr |= AROS_WORD2LE(UPSF_PORT_INDICATOR);

                            KPRINTF(5, ("EHCI Port %ld Status %08lx\n", idx, *mptr));

                            mptr++;
                            if(oldval & EHPF_ENABLECHANGE)
                            {
                                hc->hc_PortChangeMap[hciport] |= UPSF_PORT_ENABLE;
                            }
                            if(oldval & EHPF_CONNECTCHANGE)
                            {
                                hc->hc_PortChangeMap[hciport] |= UPSF_PORT_CONNECTION;
                            }
                            if(oldval & EHPF_RESUMEDTX)
                            {
                                hc->hc_PortChangeMap[hciport] |= UPSF_PORT_SUSPEND|UPSF_PORT_ENABLE;
                            }
                            if(oldval & EHPF_OVERCURRENTCHG)
                            {
                                hc->hc_PortChangeMap[hciport] |= UPSF_PORT_OVER_CURRENT;
                            }
                            *mptr = AROS_WORD2LE(hc->hc_PortChangeMap[hciport]);
                            WRITEREG32_LE(hc->hc_RegBase, portreg, oldval);
                            KPRINTF(5, ("EHCI Port %ld Change %08lx\n", idx, *mptr));
                            return(0);
                        }

                        #if (AROS_USB30_CODE)
                        /* (URTF_IN|URTF_CLASS|URTF_OTHER) USR_GET_STATUS */
                        case HCITYPE_XHCI:
                        {
                            KPRINTF(1000, ("XHCI (URTF_IN|URTF_CLASS|URTF_OTHER) USR_GET_STATUS\n"));
                            return(0);
                        }
                        #endif

                    }
                    return(0);
                }

            }
            break;

        case (URTF_IN|URTF_CLASS|URTF_DEVICE):
            switch(req)
            {
                case USR_GET_STATUS:
                {
                    UWORD *mptr = ioreq->iouh_Data;
                    if(len < sizeof(struct UsbHubStatus))
                    {
                        return(UHIOERR_STALL);
                    }
                    *mptr++ = 0;
                    *mptr++ = 0;
                    ioreq->iouh_Actual = 4;
                    return(0);
                }

                case USR_GET_DESCRIPTOR:
                    switch(val>>8)
                    {
//FIXME: Add USB3.0 hub descriptor support
                        #if (AROS_USB30_CODE) 
                        case UDT_SSHUB:
                        {
                            ULONG hubdesclen = 12;

                            struct UsbSSHubDesc *uhd = (struct UsbSSHubDesc *) ioreq->iouh_Data;
                            KPRINTF(1, ("RH: Get(SS)HubDescriptor (%ld)\n", len));

                            ioreq->iouh_Actual = (len > hubdesclen) ? hubdesclen : len;
                            CopyMem((APTR) &RHSSHubDesc, ioreq->iouh_Data, ioreq->iouh_Actual);

                            if(ioreq->iouh_Length)
                            {
                                uhd->bLength = hubdesclen;
                            }

                            uhd->bNbrPorts = unit->hu_RootHubPorts;

                            return(0);
                        }
                        #endif
                        case UDT_HUB:
                        {
                            ULONG hubdesclen = 9;
                            ULONG powergood = 1;

                            struct UsbHubDesc *uhd = (struct UsbHubDesc *) ioreq->iouh_Data;
                            KPRINTF(1, ("RH: GetHubDescriptor (%ld)\n", len));

                            if(unit->hu_RootHubPorts > 7) // needs two bytes for port masks
                            {
                                hubdesclen += 2;
                            }

                            ioreq->iouh_Actual = (len > hubdesclen) ? hubdesclen : len;
                            CopyMem((APTR) &RHHubDesc, ioreq->iouh_Data, ioreq->iouh_Actual);

                            if(ioreq->iouh_Length)
                            {
                                uhd->bLength = hubdesclen;
                            }

                            if(ioreq->iouh_Length >= 6)
                            {
                                hc = (struct PCIController *) unit->hu_Controllers.lh_Head;
                                while(hc->hc_Node.ln_Succ)
                                {
                                    if(hc->hc_HCIType == HCITYPE_OHCI)
                                    {
                                        ULONG localpwgood = (READREG32_LE(hc->hc_RegBase, OHCI_HUBDESCA) & OHAM_POWERGOOD) >> OHAS_POWERGOOD;
                                        if(localpwgood > powergood)
                                        {
                                            powergood = localpwgood;
                                            KPRINTF(10, ("Increasing power good time to %ld\n", powergood));
                                        }
                                    }
                                    hc = (struct PCIController *) hc->hc_Node.ln_Succ;
                                }

                                uhd->bPwrOn2PwrGood = powergood;
                            }
                            if(ioreq->iouh_Length >= hubdesclen)
                            {
                                uhd->bNbrPorts = unit->hu_RootHubPorts;
                                if(hubdesclen == 9)
                                {
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
                            KPRINTF(20, ("RH: Unsupported Descriptor %04lx\n", idx));
                    }
                    break;
            }

    }
    KPRINTF(20, ("RH: Unsupported command %02lx %02lx %04lx %04lx %04lx!\n", rt, req, idx, val, len));
    return(UHIOERR_STALL);
}
/* \\\ */

/* /// "cmdIntXFerRootHub()" */
WORD cmdIntXFerRootHub(struct IOUsbHWReq *ioreq,
                       struct PCIUnit *unit,
                       struct PCIDevice *base)
{
    if((ioreq->iouh_Endpoint != 1) || (!ioreq->iouh_Length))
    {
        return(UHIOERR_STALL);
    }

    if(unit->hu_RootPortChanges)
    {
        KPRINTF(1, ("Immediate Portchange map %04lx\n", unit->hu_RootPortChanges));
        if((unit->hu_RootHubPorts < 8) || (ioreq->iouh_Length == 1))
        {
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

    KPRINTF(10, ("UHCMD_CONTROLXFER ioreq: 0x%p\n", ioreq));
    uhwGetUsbState(ioreq, unit, base);
    if(!(ioreq->iouh_State & UHSF_OPERATIONAL))
    {
        return(UHIOERR_USBOFFLINE);
    }
    /* Root hub emulation */
    if(ioreq->iouh_DevAddr == unit->hu_RootHubAddr)
    {
        return(cmdControlXFerRootHub(ioreq, unit, base));
    }

    hc = unit->hu_DevControllers[ioreq->iouh_DevAddr];
    if(!hc)
    {
        KPRINTF(20, ("No Host controller assigned to device address %ld\n", ioreq->iouh_DevAddr));
        return(UHIOERR_HOSTERROR);
    }

    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    ioreq->iouh_Actual = 0;

    Disable();
    AddTail(&hc->hc_CtrlXFerQueue, (struct Node *) ioreq);
    Enable();
    SureCause(base, &hc->hc_CompleteInt);

    KPRINTF(10, ("UHCMD_CONTROLXFER processed ioreq: 0x%p\n", ioreq));
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

    KPRINTF(10, ("UHCMD_BULKXFER ioreq: 0x%p\n", ioreq));
    uhwGetUsbState(ioreq, unit, base);
    if(!(ioreq->iouh_State & UHSF_OPERATIONAL))
    {
        return(UHIOERR_USBOFFLINE);
    }

    if(ioreq->iouh_Flags & UHFF_LOWSPEED)
    {
        return(UHIOERR_BADPARAMS);
    }

    hc = unit->hu_DevControllers[ioreq->iouh_DevAddr];
    if(!hc)
    {
        return(UHIOERR_HOSTERROR);
    }

    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    ioreq->iouh_Actual = 0;

    Disable();
    AddTail(&hc->hc_BulkXFerQueue, (struct Node *) ioreq);
    Enable();
    SureCause(base, &hc->hc_CompleteInt);

    KPRINTF(10, ("UHCMD_BULKXFER processed ioreq: 0x%p\n", ioreq));
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

    KPRINTF(10, ("UHCMD_ISOXFER ioreq: 0x%p\n", ioreq));
    uhwGetUsbState(ioreq, unit, base);
    if(!(ioreq->iouh_State & UHSF_OPERATIONAL))
    {
        return(UHIOERR_USBOFFLINE);
    }

    if(ioreq->iouh_Flags & UHFF_LOWSPEED)
    {
        return(UHIOERR_BADPARAMS);
    }

    hc = unit->hu_DevControllers[ioreq->iouh_DevAddr];
    if(!hc)
    {
        return(UHIOERR_HOSTERROR);
    }

    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    ioreq->iouh_Actual = 0;

    Disable();
    AddTail(&hc->hc_IsoXFerQueue, (struct Node *) ioreq);
    Enable();
    SureCause(base, &hc->hc_CompleteInt);

    KPRINTF(10, ("UHCMD_ISOXFER processed ioreq: 0x%p\n", ioreq));
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

    KPRINTF(10, ("UHCMD_INTXFER ioreq: 0x%p\n", ioreq));
    //uhwDelayMS(1000, unit); /* Wait 200 ms */
    uhwGetUsbState(ioreq, unit, base);
    if(!(ioreq->iouh_State & UHSF_OPERATIONAL))
    {
        return(UHIOERR_USBOFFLINE);
    }

    /* Root Hub Emulation */
    if(ioreq->iouh_DevAddr == unit->hu_RootHubAddr)
    {
        return(cmdIntXFerRootHub(ioreq, unit, base));
    }

    hc = unit->hu_DevControllers[ioreq->iouh_DevAddr];
    if(!hc)
    {
        return(UHIOERR_HOSTERROR);
    }

    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    ioreq->iouh_Actual = 0;

    Disable();
    AddTail(&hc->hc_IntXFerQueue, (struct Node *) ioreq);
    Enable();
    SureCause(base, &hc->hc_CompleteInt);

    KPRINTF(10, ("UHCMD_INTXFER processed ioreq: 0x%p\n", ioreq));
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

    KPRINTF(10, ("CMD_FLUSH ioreq: 0x%p\n", ioreq));

    Disable();
    cmpioreq = (struct IOUsbHWReq *) unit->hu_RHIOQueue.lh_Head;
    while(((struct Node *) cmpioreq)->ln_Succ)
    {
        Remove(&cmpioreq->iouh_Req.io_Message.mn_Node);
        cmpioreq->iouh_Req.io_Error = IOERR_ABORTED;
        ReplyMsg(&cmpioreq->iouh_Req.io_Message);
        cmpioreq = (struct IOUsbHWReq *) unit->hu_RHIOQueue.lh_Head;
    }
    hc = (struct PCIController *) unit->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ)
    {
        cmpioreq = (struct IOUsbHWReq *) hc->hc_CtrlXFerQueue.lh_Head;
        while(((struct Node *) cmpioreq)->ln_Succ)
        {
            Remove(&cmpioreq->iouh_Req.io_Message.mn_Node);
            cmpioreq->iouh_Req.io_Error = IOERR_ABORTED;
            ReplyMsg(&cmpioreq->iouh_Req.io_Message);
            cmpioreq = (struct IOUsbHWReq *) hc->hc_CtrlXFerQueue.lh_Head;
        }
        cmpioreq = (struct IOUsbHWReq *) hc->hc_IntXFerQueue.lh_Head;
        while(((struct Node *) cmpioreq)->ln_Succ)
        {
            Remove(&cmpioreq->iouh_Req.io_Message.mn_Node);
            cmpioreq->iouh_Req.io_Error = IOERR_ABORTED;
            ReplyMsg(&cmpioreq->iouh_Req.io_Message);
            cmpioreq = (struct IOUsbHWReq *) hc->hc_IntXFerQueue.lh_Head;
        }
        cmpioreq = (struct IOUsbHWReq *) hc->hc_IsoXFerQueue.lh_Head;
        while(((struct Node *) cmpioreq)->ln_Succ)
        {
            Remove(&cmpioreq->iouh_Req.io_Message.mn_Node);
            cmpioreq->iouh_Req.io_Error = IOERR_ABORTED;
            ReplyMsg(&cmpioreq->iouh_Req.io_Message);
            cmpioreq = (struct IOUsbHWReq *) hc->hc_IsoXFerQueue.lh_Head;
        }
        cmpioreq = (struct IOUsbHWReq *) hc->hc_BulkXFerQueue.lh_Head;
        while(((struct Node *) cmpioreq)->ln_Succ)
        {
            Remove(&cmpioreq->iouh_Req.io_Message.mn_Node);
            cmpioreq->iouh_Req.io_Error = IOERR_ABORTED;
            ReplyMsg(&cmpioreq->iouh_Req.io_Message);
            cmpioreq = (struct IOUsbHWReq *) hc->hc_BulkXFerQueue.lh_Head;
        }
        switch(hc->hc_HCIType)
        {
            case HCITYPE_UHCI:
                cmpioreq = (struct IOUsbHWReq *) hc->hc_TDQueue.lh_Head;
                while(((struct Node *) cmpioreq)->ln_Succ)
                {
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
                while(((struct Node *) cmpioreq)->ln_Succ)
                {
                    ehciFreeAsyncContext(hc, cmpioreq);
                    cmpioreq->iouh_Req.io_Error = IOERR_ABORTED;
                    ReplyMsg(&cmpioreq->iouh_Req.io_Message);
                    cmpioreq = (struct IOUsbHWReq *) hc->hc_TDQueue.lh_Head;
                }
                cmpioreq = (struct IOUsbHWReq *) hc->hc_PeriodicTDQueue.lh_Head;
                while(((struct Node *) cmpioreq)->ln_Succ)
                {
                    ehciFreePeriodicContext(hc, cmpioreq);
                    cmpioreq->iouh_Req.io_Error = IOERR_ABORTED;
                    ReplyMsg(&cmpioreq->iouh_Req.io_Message);
                    cmpioreq = (struct IOUsbHWReq *) hc->hc_PeriodicTDQueue.lh_Head;
                }
                break;
            #if (AROS_USB30_CODE)
            case HCITYPE_XHCI:
                KPRINTF(1000, ("XHCI cmdFlush\n"));
                break;
            #endif
        }
        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }
    Enable();
    /* Return success
    */
    return RC_OK;
}
/* \\\ */

/* /// "NSD stuff" */

static
const UWORD NSDSupported[] =
{
    CMD_FLUSH, CMD_RESET,
    UHCMD_QUERYDEVICE, UHCMD_USBRESET,
    UHCMD_USBRESUME, UHCMD_USBSUSPEND,
    UHCMD_USBOPER, UHCMD_CONTROLXFER ,
    UHCMD_ISOXFER, UHCMD_INTXFER,
    UHCMD_BULKXFER,
    NSCMD_DEVICEQUERY, 0
};

WORD cmdNSDeviceQuery(struct IOStdReq *ioreq,
                       struct PCIUnit *unit,
                       struct PCIDevice *base)
{
    struct my_NSDeviceQueryResult *query;

    query = (struct my_NSDeviceQueryResult *) ioreq->io_Data;

    KPRINTF(10, ("NSCMD_DEVICEQUERY ioreq: 0x%p query: 0x%p\n", ioreq, query));

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
         TermIO((struct IOUsbHWReq *) ioreq, base);

         /* Don't reply, we already did.
         */
         return RC_DONTREPLY;
    }

    ioreq->io_Actual         = query->SizeAvailable
                             = sizeof(struct my_NSDeviceQueryResult);
    query->DeviceType        = NSDEVTYPE_USBHARDWARE;
    query->DeviceSubType     = 0;
    query->SupportedCommands = NSDSupported;

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
    if(!(ioreq->iouh_Req.io_Flags & IOF_QUICK))
    {
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

    KPRINTF(10, ("cmdAbort(%p)\n", ioreq));

    Disable();
    cmpioreq = (struct IOUsbHWReq *) unit->hu_RHIOQueue.lh_Head;
    while(((struct Node *) cmpioreq)->ln_Succ)
    {
        if(ioreq == cmpioreq)
        {
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            Enable();
            return TRUE;
        }
        cmpioreq = (struct IOUsbHWReq *) cmpioreq->iouh_Req.io_Message.mn_Node.ln_Succ;
    }

    hc = (struct PCIController *) unit->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ)
    {
        cmpioreq = (struct IOUsbHWReq *) hc->hc_CtrlXFerQueue.lh_Head;
        while(((struct Node *) cmpioreq)->ln_Succ)
        {
            if(ioreq == cmpioreq)
            {
                foundit = TRUE;
                break;
            }
            cmpioreq = (struct IOUsbHWReq *) cmpioreq->iouh_Req.io_Message.mn_Node.ln_Succ;
        }
        if(!foundit)
        {
            cmpioreq = (struct IOUsbHWReq *) hc->hc_IntXFerQueue.lh_Head;
            while(((struct Node *) cmpioreq)->ln_Succ)
            {
                if(ioreq == cmpioreq)
                {
                    foundit = TRUE;
                    break;
                }
                cmpioreq = (struct IOUsbHWReq *) cmpioreq->iouh_Req.io_Message.mn_Node.ln_Succ;
            }
        }
        if(!foundit)
        {
            cmpioreq = (struct IOUsbHWReq *) hc->hc_IsoXFerQueue.lh_Head;
            while(((struct Node *) cmpioreq)->ln_Succ)
            {
                if(ioreq == cmpioreq)
                {
                    foundit = TRUE;
                    break;
                }
                cmpioreq = (struct IOUsbHWReq *) cmpioreq->iouh_Req.io_Message.mn_Node.ln_Succ;
            }
        }
        if(!foundit)
        {
            cmpioreq = (struct IOUsbHWReq *) hc->hc_BulkXFerQueue.lh_Head;
            while(((struct Node *) cmpioreq)->ln_Succ)
            {
                if(ioreq == cmpioreq)
                {
                    foundit = TRUE;
                    break;
                }
                cmpioreq = (struct IOUsbHWReq *) cmpioreq->iouh_Req.io_Message.mn_Node.ln_Succ;
            }
        }
        if(!foundit)
        {
            // IOReq is probably pending in some transfer structure
            devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
            switch(hc->hc_HCIType)
            {
                case HCITYPE_UHCI:
                    cmpioreq = (struct IOUsbHWReq *) hc->hc_TDQueue.lh_Head;
                    while(((struct Node *) cmpioreq)->ln_Succ)
                    {
                        if(ioreq == cmpioreq)
                        {
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
                    while(((struct Node *) cmpioreq)->ln_Succ)
                    {
                        if(ioreq == cmpioreq)
                        {
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
                    while(((struct Node *) cmpioreq)->ln_Succ)
                    {
                        if(ioreq == cmpioreq)
                        {
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
                    while(((struct Node *) cmpioreq)->ln_Succ)
                    {
                        if(ioreq == cmpioreq)
                        {
                            ehciFreePeriodicContext(hc, ioreq);
                            Enable();
                            ioreq->iouh_Req.io_Error = IOERR_ABORTED;
                            TermIO(ioreq, base);
                            return TRUE;
                        }
                        cmpioreq = (struct IOUsbHWReq *) cmpioreq->iouh_Req.io_Message.mn_Node.ln_Succ;
                    }
                    break;

                #if (AROS_USB30_CODE)
                case HCITYPE_XHCI:
                    KPRINTF(1000, ("XHCI cmdAbortIO\n"));
                    break;
                #endif

            }
        }
        if(foundit)
        {
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            break;
        }
        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }
    Enable();

    if (foundit)
    {
        ioreq->iouh_Req.io_Error = IOERR_ABORTED;
        TermIO(ioreq, base);
    }
    else
    {
        KPRINTF(20, ("WARNING, could not abort unknown IOReq %p\n", ioreq));
    }
    return(foundit);
}
/* \\\ */

/* /// "uhwCheckRootHubChanges()" */
void uhwCheckRootHubChanges(struct PCIUnit *unit)
{
    struct IOUsbHWReq *ioreq;

    if(unit->hu_RootPortChanges && unit->hu_RHIOQueue.lh_Head->ln_Succ)
    {
        KPRINTF(1, ("Portchange map %04lx\n", unit->hu_RootPortChanges));
        Disable();
        ioreq = (struct IOUsbHWReq *) unit->hu_RHIOQueue.lh_Head;
        while(((struct Node *) ioreq)->ln_Succ)
        {
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            if((unit->hu_RootHubPorts < 8) || (ioreq->iouh_Length == 1))
            {
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

    /* Clear Feature(Endpoint halt) */
    if((ioreq->iouh_SetupData.bmRequestType == (URTF_STANDARD|URTF_ENDPOINT)) &&
       (ioreq->iouh_SetupData.bRequest == USR_CLEAR_FEATURE) &&
       (ioreq->iouh_SetupData.wValue == AROS_WORD2LE(UFS_ENDPOINT_HALT)))
    {
        KPRINTF(10, ("Resetting toggle bit for endpoint %ld\n", AROS_WORD2LE(ioreq->iouh_SetupData.wIndex) & 0xf));
        unit->hu_DevDataToggle[(ioreq->iouh_DevAddr<<5)|(AROS_WORD2LE(ioreq->iouh_SetupData.wIndex) & 0xf)|((AROS_WORD2LE(ioreq->iouh_SetupData.wIndex) & 0x80)>>3)] = 0;
    }
    else if((ioreq->iouh_SetupData.bmRequestType == (URTF_STANDARD|URTF_DEVICE)) &&
            (ioreq->iouh_SetupData.bRequest == USR_SET_ADDRESS))
    {
        /* Set Address -> clear all endpoints */
        ULONG epnum;
        ULONG adr = AROS_WORD2BE(ioreq->iouh_SetupData.wValue)>>3;
        KPRINTF(10, ("Resetting toggle bits for device address %ld\n", adr>>5));
        for(epnum = 0; epnum < 31; epnum++)
        {
            unit->hu_DevDataToggle[adr+epnum] = 0;
        }
        // transfer host controller ownership
        unit->hu_DevControllers[ioreq->iouh_DevAddr] = NULL;
        unit->hu_DevControllers[adr>>5] = hc;
    }
    else if((ioreq->iouh_SetupData.bmRequestType == (URTF_CLASS|URTF_OTHER)) &&
            (ioreq->iouh_SetupData.bRequest == USR_SET_FEATURE) &&
            (ioreq->iouh_SetupData.wValue == AROS_WORD2LE(UFS_PORT_RESET)))
    {
        // a hub will be enumerating a device on this host controller soon!
        KPRINTF(10, ("Hub RESET caught, assigning Dev0 to %p!\n", hc));
        unit->hu_DevControllers[0] = hc;
    }
}
/* \\\ */

/* /// "uhwNakTimeoutInt()" */
AROS_UFH1(void, uhwNakTimeoutInt,
          AROS_UFHA(struct PCIUnit *,  unit, A1))
{
    AROS_USERFUNC_INIT

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

    KPRINTF(1, ("Enter NakTimeoutInt(0x%p)\n", unit));

    // check for port status change for UHCI and frame rollovers and NAK Timeouts
    hc = (struct PCIController *) unit->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ)
    {
        if (!(hc->hc_Flags & HCF_ONLINE))
        {
            hc = (struct PCIController *) hc->hc_Node.ln_Succ;
            continue;
        }
        causeint = FALSE;
        switch(hc->hc_HCIType)
        {
            case HCITYPE_UHCI:
            {
                ULONG framecnt;
                uhciUpdateFrameCounter(hc);
                framecnt = hc->hc_FrameCounter;

                // NakTimeout
                ioreq = (struct IOUsbHWReq *) hc->hc_TDQueue.lh_Head;
                while(((struct Node *) ioreq)->ln_Succ)
                {
                    if(ioreq->iouh_Flags & UHFF_NAKTIMEOUT)
                    {
                        uqh = (struct UhciQH *) ioreq->iouh_DriverPrivate1;
                        if(uqh)
                        {
                            KPRINTF(1, ("Examining IOReq=%p with UQH=%p\n", ioreq, uqh));
                            devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
                            linkelem = READMEM32_LE(&uqh->uqh_Element);
                            if(linkelem & UHCI_TERMINATE)
                            {
                                KPRINTF(1, ("UQH terminated %08lx\n", linkelem));
                                if(framecnt > unit->hu_NakTimeoutFrame[devadrep])
                                {
                                    // give the thing the chance to exit gracefully
                                    KPRINTF(20, ("Terminated? NAK timeout %ld > %ld, IOReq=%p\n", framecnt, unit->hu_NakTimeoutFrame[devadrep], ioreq));
                                    causeint = TRUE;
                                }
                            } else {
                                utd = (struct UhciTD *) (((IPTR)linkelem & UHCI_PTRMASK) - hc->hc_PCIVirtualAdjust - 16); // struct UhciTD starts 16 before physical TD
                                ctrlstatus = READMEM32_LE(&utd->utd_CtrlStatus);
                                if(ctrlstatus & UTCF_ACTIVE)
                                {
                                    if(framecnt > unit->hu_NakTimeoutFrame[devadrep])
                                    {
                                        // give the thing the chance to exit gracefully
                                        KPRINTF(20, ("NAK timeout %ld > %ld, IOReq=%p\n", framecnt, unit->hu_NakTimeoutFrame[devadrep], ioreq));
                                        ctrlstatus &= ~UTCF_ACTIVE;
                                        WRITEMEM32_LE(&utd->utd_CtrlStatus, ctrlstatus);
                                        causeint = TRUE;
                                    }
                                } else {
                                    if(framecnt > unit->hu_NakTimeoutFrame[devadrep])
                                    {
                                        // give the thing the chance to exit gracefully
                                        KPRINTF(20, ("Terminated? NAK timeout %ld > %ld, IOReq=%p\n", framecnt, unit->hu_NakTimeoutFrame[devadrep], ioreq));
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

            case HCITYPE_OHCI:
            {
                ULONG framecnt;
                ohciUpdateFrameCounter(hc);
                framecnt = hc->hc_FrameCounter;
                // NakTimeout
                ioreq = (struct IOUsbHWReq *) hc->hc_TDQueue.lh_Head;
                while(((struct Node *) ioreq)->ln_Succ)
                {
                    // Remember the successor because ohciAbortRequest() will move the request to another list
                    struct IOUsbHWReq *succ = (struct IOUsbHWReq *)ioreq->iouh_Req.io_Message.mn_Node.ln_Succ;

                    if(ioreq->iouh_Flags & UHFF_NAKTIMEOUT)
                    {
                        KPRINTF(1, ("Examining IOReq=%p with OED=%p\n", ioreq, ioreq->iouh_DriverPrivate1));
                        if (ioreq->iouh_DriverPrivate1)
                        {
                            KPRINTF(1, ("CTRL=%04lx, CMD=%01lx, F=%ld, hccaDH=%08lx, hcDH=%08lx, CH=%08lx, CCH=%08lx, IntEn=%08lx\n",
                                         READREG32_LE(hc->hc_RegBase, OHCI_CONTROL),
                                         READREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS),
                                         READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT),
                                         READMEM32_LE(&hc->hc_OhciHCCA->oha_DoneHead),
                                         READREG32_LE(hc->hc_RegBase, OHCI_DONEHEAD),
                                         READREG32_LE(hc->hc_RegBase, OHCI_CTRL_HEAD_ED),
                                         READREG32_LE(hc->hc_RegBase, OHCI_CTRL_ED),
                                         READREG32_LE(hc->hc_RegBase, OHCI_INTEN)));

                            devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
                            if(framecnt > unit->hu_NakTimeoutFrame[devadrep])
                            {
                            	// give the thing the chance to exit gracefully
                            	KPRINTF(200, ("HC 0x%p NAK timeout %ld > %ld, IOReq=%p\n", hc, framecnt, unit->hu_NakTimeoutFrame[devadrep], ioreq));
				ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                            	ohciAbortRequest(hc, ioreq);
                            }
                        }
                    }
                    ioreq = succ;
                }
                break;
            }

            case HCITYPE_EHCI:
            {
                ULONG framecnt;
                ehciUpdateFrameCounter(hc);
                framecnt = hc->hc_FrameCounter;
                // NakTimeout
                for(cnt = 0; cnt < 1; cnt++)
                {
                    ioreq = (struct IOUsbHWReq *) (cnt ? hc->hc_PeriodicTDQueue.lh_Head : hc->hc_TDQueue.lh_Head);
                    while(((struct Node *) ioreq)->ln_Succ)
                    {
                        if(ioreq->iouh_Flags & UHFF_NAKTIMEOUT)
                        {
                            eqh = (struct EhciQH *) ioreq->iouh_DriverPrivate1;
                            if(eqh)
                            {
                                KPRINTF(1, ("Examining IOReq=%p with EQH=%p\n", ioreq, eqh));
                                devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
                                ctrlstatus = READMEM32_LE(&eqh->eqh_CtrlStatus);
                                if(ctrlstatus & ETCF_ACTIVE)
                                {
                                    if(framecnt > unit->hu_NakTimeoutFrame[devadrep])
                                    {
                                        // give the thing the chance to exit gracefully
                                        KPRINTF(20, ("NAK timeout %ld > %ld, IOReq=%p\n", framecnt, unit->hu_NakTimeoutFrame[devadrep], ioreq));
                                        ctrlstatus &= ~ETCF_ACTIVE;
                                        ctrlstatus |= ETSF_HALTED;
                                        WRITEMEM32_LE(&eqh->eqh_CtrlStatus, ctrlstatus);
                                        causeint = TRUE;
                                    }
                                } else {
                                    if(ctrlstatus & ETCF_READYINTEN)
                                    {
                                        KPRINTF(10, ("INT missed?!? Manually causing it! %08lx, IOReq=%p\n",
                                                     ctrlstatus, ioreq));
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
            #if (AROS_USB30_CODE)
            case HCITYPE_XHCI:
            {
                //KPRINTF(1000, ("XHCI uhwNakTimeoutInt\n"));
                break;
            }
            #endif
        }
        if(causeint)
        {
            SureCause(base, &hc->hc_CompleteInt);
        }

        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }

    uhwCheckRootHubChanges(unit);

    unit->hu_NakTimeoutReq.tr_time.tv_micro = 150*1000;
    SendIO((APTR) &unit->hu_NakTimeoutReq);

    KPRINTF(1, ("Exit NakTimeoutInt(0x%p)\n", unit));

    AROS_USERFUNC_EXIT
}
/* \\\ */

