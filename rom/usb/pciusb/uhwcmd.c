/* uhwcmd.c - pciusb.device by Chris Hodges
*/

#include "uhwcmd.h"
#include <devices/usb_hub.h>
#include <strings.h>

#include <proto/utility.h>
#include <proto/exec.h>

#define NewList NEWLIST

/* Root hub data */
const struct UsbStdDevDesc RHDevDesc = { sizeof(struct UsbStdDevDesc), UDT_DEVICE, AROS_WORD2LE(0x0110), HUB_CLASSCODE, 0, 0, 8, AROS_WORD2LE(0x0000), AROS_WORD2LE(0x0000), AROS_WORD2LE(0x0100), 1, 2, 0, 1 };

const struct UsbStdCfgDesc RHCfgDesc = { 9, UDT_CONFIGURATION, AROS_WORD2LE(9+9+7), 1, 1, 3, USCAF_ONE|USCAF_SELF_POWERED, 0 };
const struct UsbStdIfDesc  RHIfDesc  = { 9, UDT_INTERFACE, 0, 0, 1, HUB_CLASSCODE, 0, 0, 4 };
const struct UsbStdEPDesc  RHEPDesc  = { 7, UDT_ENDPOINT, URTF_IN|1, USEAF_INTERRUPT, AROS_WORD2LE(1), 255 };
const struct UsbHubDesc    RHHubDesc = { 9, UDT_HUB, 0, AROS_WORD2LE(UHCF_INDIVID_POWER|UHCF_INDIVID_OVP), 0, 1, 1, 0 };

const CONST_STRPTR RHStrings[] = { "Chris Hodges", "PCI Root Hub", "Standard Config", "Hub interface" };

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
            Enable();
            (*((void (*)(struct Interrupt *)) (interrupt->is_Code)))(interrupt->is_Data);
            Disable();
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
void uhwDelayMS(ULONG milli, struct PCIUnit *unit, struct PCIDevice *base)
{
    unit->hu_TimerReq->tr_time.tv_secs  = 0;
    unit->hu_TimerReq->tr_time.tv_micro = milli * 1000;
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

/* /// "uhwHWInit()" */
void uhwHWInit(struct PCIController *hc)
{
    KPRINTF(1, ("Reset\n"));
    //unit->hu_FrameCounter = 1;
    //unit->hu_RootHubAddr = 0;
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
    KPRINTF(10, ("CMD_RESET ioreq: 0x%08lx\n", ioreq));
    //uhwHWInit(unit);

    uhwDelayMS(1, unit, base);
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
    KPRINTF(10, ("UHCMD_USBRESET ioreq: 0x%08lx\n", ioreq));

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
    KPRINTF(10, ("UHCMD_USBRESUME ioreq: 0x%08lx\n", ioreq));

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
    KPRINTF(10, ("UHCMD_USBSUSPEND ioreq: 0x%08lx\n", ioreq));

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
    KPRINTF(10, ("UHCMD_USBOPER ioreq: 0x%08lx\n", ioreq));

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

    KPRINTF(10, ("UHCMD_QUERYDEVICE ioreq: 0x%08lx, taglist: 0x%08lx\n", ioreq, taglist));

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
        *((STRPTR *) tag->ti_Data) = "PCI UHCI/OHCI/EHCI USB Host Controller";
        count++;
    }
    if((tag = FindTagItem(UHA_Description, taglist)))
    {
        *((STRPTR *) tag->ti_Data) = "Generic adaptive host controller driver for PCI cards";
        count++;
    }
    if((tag = FindTagItem(UHA_Copyright, taglist)))
    {
        *((STRPTR *) tag->ti_Data) = "©2007-2009 Chris Hodges";
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
                                        *mptr++ = AROS_WORD2LE(*source);
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
                    cmdgood = FALSE;
                    switch(hc->hc_HCIType)
                    {
                        case HCITYPE_UHCI:
                        {
                            UWORD portreg = hciport ? UHCI_PORT2STSCTRL : UHCI_PORT1STSCTRL;
                            ULONG oldval = READREG16_LE(hc->hc_RegBase, portreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE); // these are clear-on-write!
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
                                    WRITEREG16_LE(hc->hc_RegBase, portreg, newval);
                                    uhwDelayMS(75, unit, base);
                                    newval = READREG16_LE(hc->hc_RegBase, portreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE|UHPF_PORTSUSPEND|UHPF_PORTENABLE);
                                    KPRINTF(10, ("Reset=%s\n", newval & UHPF_PORTRESET ? "GOOD" : "BAD!"));
                                    newval &= ~UHPF_PORTRESET;
                                    WRITEREG16_LE(hc->hc_RegBase, portreg, newval);
                                    uhwDelayMS(5, unit, base);
                                    newval = READREG16_LE(hc->hc_RegBase, portreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE|UHPF_PORTSUSPEND);
                                    KPRINTF(10, ("Reset=%s\n", newval & UHPF_PORTRESET ? "BAD!" : "GOOD"));
                                    newval &= ~UHPF_PORTRESET;
                                    newval |= UHPF_PORTENABLE;
                                    WRITEREG16_LE(hc->hc_RegBase, portreg, newval);
                                    hc->hc_PortChangeMap[hciport] |= UPSF_PORT_RESET|UPSF_PORT_ENABLE; // manually fake reset change
                                    uhwDelayMS(10, unit, base);

                                    cnt = 100;
                                    do
                                    {
                                        uhwDelayMS(1, unit, base);
                                        newval = READREG16_LE(hc->hc_RegBase, portreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE);
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
                                WRITEREG16_LE(hc->hc_RegBase, portreg, newval);
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
                                    WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTRESET);
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

                                    // this is an ugly blocking workaround to the inability of UHCI to clear reset automatically
                                    newval &= ~(EHPF_PORTSUSPEND|EHPF_PORTENABLE);
                                    newval |= EHPF_PORTRESET;
                                    WRITEREG32_LE(hc->hc_RegBase, portreg, newval);
                                    uhwDelayMS(75, unit, base);
                                    newval = READREG32_LE(hc->hc_RegBase, portreg) & ~(EHPF_OVERCURRENTCHG|EHPF_ENABLECHANGE|EHPF_CONNECTCHANGE|EHPF_PORTSUSPEND|EHPF_PORTENABLE);
                                    KPRINTF(10, ("Reset=%s\n", newval & EHPF_PORTRESET ? "GOOD" : "BAD!"));
                                    newval &= ~EHPF_PORTRESET;
                                    WRITEREG32_LE(hc->hc_RegBase, portreg, newval);
                                    uhwDelayMS(10, unit, base);
                                    newval = READREG32_LE(hc->hc_RegBase, portreg) & ~(EHPF_OVERCURRENTCHG|EHPF_ENABLECHANGE|EHPF_CONNECTCHANGE|EHPF_PORTSUSPEND);
                                    KPRINTF(10, ("Reset=%s\n", newval & EHPF_PORTRESET ? "BAD!" : "GOOD"));
                                    KPRINTF(10, ("Highspeed=%s\n", newval & EHPF_PORTENABLE ? "YES!" : "NO"));
                                    if(!(newval & EHPF_PORTENABLE))
                                    {
                                        // if not highspeed, release ownership
                                        KPRINTF(20, ("Transferring ownership to UHCI/OHCI port %ld\n", unit->hu_PortNum11[idx - 1]));
                                        KPRINTF(10, ("Device is %s\n", newval & EHPF_LINESTATUS_DM ? "LOWSPEED" : "FULLSPEED"));
                                        unit->hu_EhciOwned[idx - 1] = FALSE;
                                        newval |= EHPF_NOTPORTOWNER;
                                        WRITEREG32_LE(hc->hc_RegBase, portreg, newval);
                                        // enable companion controller port
                                        switch(chc->hc_HCIType)
                                        {
                                            case HCITYPE_UHCI:
                                            {
                                                UWORD uhcihciport = unit->hu_PortNum11[idx - 1];
                                                UWORD uhciportreg = uhcihciport ? UHCI_PORT2STSCTRL : UHCI_PORT1STSCTRL;
                                                ULONG uhcinewval;

                                                uhcinewval = READREG16_LE(chc->hc_RegBase, uhciportreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE|UHPF_PORTSUSPEND);
                                                KPRINTF(10, ("UHCI Reset=%s\n", uhcinewval & UHPF_PORTRESET ? "BAD!" : "GOOD"));
                                                if((uhcinewval & UHPF_PORTRESET))//|| (newval & EHPF_LINESTATUS_DM))
                                                {
                                                    // this is an ugly blocking workaround to the inability of UHCI to clear reset automatically
                                                    KPRINTF(20, ("Uhm, UHCI reset was bad!\n"));
                                                    uhcinewval &= ~(UHPF_PORTSUSPEND|UHPF_PORTENABLE);
                                                    uhcinewval |= UHPF_PORTRESET;
                                                    WRITEREG16_LE(chc->hc_RegBase, uhciportreg, uhcinewval);
                                                    uhwDelayMS(75, unit, base);
                                                    uhcinewval = READREG16_LE(chc->hc_RegBase, uhciportreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE|UHPF_PORTSUSPEND|UHPF_PORTENABLE);
                                                    KPRINTF(10, ("ReReset=%s\n", uhcinewval & UHPF_PORTRESET ? "GOOD" : "BAD!"));
                                                    uhcinewval &= ~UHPF_PORTRESET;
                                                    WRITEREG16_LE(chc->hc_RegBase, uhciportreg, uhcinewval);
                                                    uhwDelayMS(5, unit, base);
                                                    uhcinewval = READREG16_LE(chc->hc_RegBase, uhciportreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE|UHPF_PORTSUSPEND);
                                                    KPRINTF(10, ("ReReset=%s\n", uhcinewval & UHPF_PORTRESET ? "STILL BAD!" : "GOOD"));
                                                }
                                                uhcinewval &= ~UHPF_PORTRESET;
                                                uhcinewval |= UHPF_PORTENABLE;
                                                WRITEREG16_LE(chc->hc_RegBase, uhciportreg, uhcinewval);
                                                chc->hc_PortChangeMap[uhcihciport] |= UPSF_PORT_RESET|UPSF_PORT_ENABLE; // manually fake reset change
                                                uhwDelayMS(5, unit, base);
                                                cnt = 100;
                                                do
                                                {
                                                    uhwDelayMS(1, unit, base);
                                                    uhcinewval = READREG16_LE(chc->hc_RegBase, uhciportreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE);
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
                                                ULONG ohcioldval = READREG32_LE(hc->hc_RegBase, portreg);
                                                KPRINTF(10, ("OHCI Resetting Port (%s)\n", ohcioldval & OHPF_PORTRESET ? "already" : "ok"));
                                                WRITEREG32_LE(chc->hc_RegBase, ohciportreg, OHPF_PORTPOWER|OHPF_PORTRESET);
                                                break;
                                            }

                                        }
                                        // make enumeration possible
                                        unit->hu_DevControllers[0] = chc;
                                    } else {
                                        newval &= ~EHPF_PORTRESET;
                                        newval |= EHPF_PORTENABLE;
                                        WRITEREG16_LE(hc->hc_RegBase, portreg, newval);
                                        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_RESET; // manually fake reset change
                                        uhwDelayMS(10, unit, base);
                                        cnt = 100;
                                        do
                                        {
                                            uhwDelayMS(1, unit, base);
                                            newval = READREG32_LE(hc->hc_RegBase, portreg) & ~(EHPF_OVERCURRENTCHG|EHPF_ENABLECHANGE|EHPF_CONNECTCHANGE);
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
                    KPRINTF(10, ("Clear Feature %ld maps from glob. Port %ld to local Port %ld\n", val, idx, hciport));
                    cmdgood = FALSE;
                    switch(hc->hc_HCIType)
                    {
                        case HCITYPE_UHCI:
                        {
                            UWORD portreg = hciport ? UHCI_PORT2STSCTRL : UHCI_PORT1STSCTRL;
                            ULONG oldval = READREG16_LE(hc->hc_RegBase, portreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE); // these are clear-on-write!
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
                                WRITEREG16_LE(hc->hc_RegBase, portreg, newval);
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
                            ULONG oldval = READREG32_LE(hc->hc_RegBase, portreg);

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
                            UWORD oldval = READREG16_LE(hc->hc_RegBase, portreg);
                            *mptr = UPSF_PORT_POWER;
                            if(oldval & UHPF_PORTCONNECTED) *mptr |= UPSF_PORT_CONNECTION;
                            if(oldval & UHPF_PORTENABLE) *mptr |= UPSF_PORT_ENABLE;
                            if(oldval & UHPF_LOWSPEED) *mptr |= UPSF_PORT_LOW_SPEED;
                            if(oldval & UHPF_PORTRESET) *mptr |= UPSF_PORT_RESET;
                            if(oldval & UHPF_PORTSUSPEND) *mptr |= UPSF_PORT_SUSPEND;

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
                            *mptr = hc->hc_PortChangeMap[hciport];
                            WRITEREG16_LE(hc->hc_RegBase, portreg, oldval);
                            KPRINTF(5, ("UHCI Port %ld Change %08lx\n", idx, *mptr));
                            return(0);
                        }

                        case HCITYPE_OHCI:
                        {
                            UWORD portreg = OHCI_PORTSTATUS + (hciport<<2);
                            ULONG oldval = READREG32_LE(hc->hc_RegBase, portreg);

                            *mptr = 0;
                            if(oldval & OHPF_PORTPOWER) *mptr |= UPSF_PORT_POWER;
                            if(oldval & OHPF_OVERCURRENT) *mptr |= UPSF_PORT_OVER_CURRENT;
                            if(oldval & OHPF_PORTCONNECTED) *mptr |= UPSF_PORT_CONNECTION;
                            if(oldval & OHPF_PORTENABLE) *mptr |= UPSF_PORT_ENABLE;
                            if(oldval & OHPF_LOWSPEED) *mptr |= UPSF_PORT_LOW_SPEED;
                            if(oldval & OHPF_PORTRESET) *mptr |= UPSF_PORT_RESET;
                            if(oldval & OHPF_PORTSUSPEND) *mptr |= UPSF_PORT_SUSPEND;

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
                            *mptr = hc->hc_PortChangeMap[hciport];
                            KPRINTF(5, ("OHCI Port %ld Change %08lx\n", idx, *mptr));
                            return(0);
                        }

                        case HCITYPE_EHCI:
                        {
                            UWORD portreg = EHCI_PORTSC1 + (hciport<<2);
                            ULONG oldval = READREG32_LE(hc->hc_RegBase, portreg);

                            *mptr = 0;
                            if(oldval & EHPF_PORTCONNECTED) *mptr |= UPSF_PORT_CONNECTION;
                            if(oldval & EHPF_PORTENABLE) *mptr |= UPSF_PORT_ENABLE|UPSF_PORT_HIGH_SPEED;
                            if((oldval & (EHPF_LINESTATUS_DM|EHPF_PORTCONNECTED|EHPF_PORTENABLE)) ==
                               (EHPF_LINESTATUS_DM|EHPF_PORTCONNECTED))
                            {
                                KPRINTF(10, ("EHCI Port %ld is LOWSPEED\n", idx));
                                // we need to detect low speed devices prior to reset
                                *mptr |= UPSF_PORT_LOW_SPEED;
                            }

                            if(oldval & EHPF_PORTRESET) *mptr |= UPSF_PORT_RESET;
                            if(oldval & EHPF_PORTSUSPEND) *mptr |= UPSF_PORT_SUSPEND;
                            if(oldval & EHPF_PORTPOWER) *mptr |= UPSF_PORT_POWER;
                            if(oldval & EHPM_PORTINDICATOR) *mptr |= UPSF_PORT_INDICATOR;

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
                            *mptr = hc->hc_PortChangeMap[hciport];
                            WRITEREG32_LE(hc->hc_RegBase, portreg, oldval);
                            KPRINTF(5, ("EHCI Port %ld Change %08lx\n", idx, *mptr));
                            return(0);
                        }
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
                                        ULONG localpwgood = READREG32_LE(hc->hc_RegBase, OHCI_HUBDESCA & OHAM_POWERGOOD) >> OHAS_POWERGOOD;
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

    KPRINTF(10, ("UHCMD_CONTROLXFER ioreq: 0x%08lx\n", ioreq));
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

    KPRINTF(10, ("UHCMD_CONTROLXFER processed ioreq: 0x%08lx\n", ioreq));
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

    KPRINTF(10, ("UHCMD_BULKXFER ioreq: 0x%08lx\n", ioreq));
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

    KPRINTF(10, ("UHCMD_BULKXFER processed ioreq: 0x%08lx\n", ioreq));
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

    KPRINTF(10, ("UHCMD_ISOXFER ioreq: 0x%08lx\n", ioreq));
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

    KPRINTF(10, ("UHCMD_ISOXFER processed ioreq: 0x%08lx\n", ioreq));
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

    KPRINTF(10, ("UHCMD_INTXFER ioreq: 0x%08lx\n", ioreq));
    //uhwDelayMS(1000, unit, base); /* Wait 200 ms */
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

    KPRINTF(10, ("UHCMD_INTXFER processed ioreq: 0x%08lx\n", ioreq));
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

    KPRINTF(10, ("CMD_FLUSH ioreq: 0x%08lx\n", ioreq));

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
                    Remove(&cmpioreq->iouh_Req.io_Message.mn_Node);
                    devadrep = (cmpioreq->iouh_DevAddr<<5) + cmpioreq->iouh_Endpoint + ((cmpioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
                    unit->hu_DevBusyReq[devadrep] = NULL;
                    ehciFreeAsyncContext(hc, (struct EhciQH *) cmpioreq->iouh_DriverPrivate1);
                    cmpioreq->iouh_Req.io_Error = IOERR_ABORTED;
                    ReplyMsg(&cmpioreq->iouh_Req.io_Message);
                    cmpioreq = (struct IOUsbHWReq *) hc->hc_TDQueue.lh_Head;
                }
                cmpioreq = (struct IOUsbHWReq *) hc->hc_PeriodicTDQueue.lh_Head;
                while(((struct Node *) cmpioreq)->ln_Succ)
                {
                    Remove(&cmpioreq->iouh_Req.io_Message.mn_Node);
                    devadrep = (cmpioreq->iouh_DevAddr<<5) + cmpioreq->iouh_Endpoint + ((cmpioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
                    unit->hu_DevBusyReq[devadrep] = NULL;
                    ehciFreePeriodicContext(hc, (struct EhciQH *) cmpioreq->iouh_DriverPrivate1);
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

    KPRINTF(10, ("cmdAbort(%08lx)\n", ioreq));

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
        if(foundit)
        {
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            break;
        } else {
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
                            foundit = TRUE;
                            unit->hu_DevBusyReq[devadrep] = NULL;
                            ohciFreeEDContext(hc, (struct OhciED *) ioreq->iouh_DriverPrivate1);
                            break;
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
                            foundit = TRUE;
                            unit->hu_DevBusyReq[devadrep] = NULL;
                            ehciFreeAsyncContext(hc, (struct EhciQH *) ioreq->iouh_DriverPrivate1);
                            break;
                        }
                        cmpioreq = (struct IOUsbHWReq *) cmpioreq->iouh_Req.io_Message.mn_Node.ln_Succ;
                    }
                    cmpioreq = (struct IOUsbHWReq *) hc->hc_PeriodicTDQueue.lh_Head;
                    while(((struct Node *) cmpioreq)->ln_Succ)
                    {
                        if(ioreq == cmpioreq)
                        {
                            foundit = TRUE;
                            unit->hu_DevBusyReq[devadrep] = NULL;
                            ehciFreePeriodicContext(hc, (struct EhciQH *) ioreq->iouh_DriverPrivate1);
                            break;
                        }
                        cmpioreq = (struct IOUsbHWReq *) cmpioreq->iouh_Req.io_Message.mn_Node.ln_Succ;
                    }
                    break;
            }
            if(foundit)
            {
                Remove(&ioreq->iouh_Req.io_Message.mn_Node);
                break;
            }
        }
        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }
    Enable();
    if(!foundit)
    {
        KPRINTF(20, ("WARNING, could not abort unknown IOReq %08lx\n", ioreq));
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
            if((ioreq->iouh_Length > 0) || (unit->hu_RootHubPorts < 8))
            {
                *((UBYTE *) ioreq->iouh_Data) = unit->hu_RootPortChanges;
                ioreq->iouh_Actual = 1;
            }
            else if(ioreq->iouh_Length > 1)
            {
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
        ULONG adr = ioreq->iouh_SetupData.wValue>>3;
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
        KPRINTF(10, ("Hub RESET caught, assigning Dev0 to %08lx!\n", hc));
        unit->hu_DevControllers[0] = hc;
    }
}
/* \\\ */

/* ---------------------------------------------------------------------- *
 *                    UHCI Specific Stuff                                 *
 * ---------------------------------------------------------------------- */

/* /// "uhciFreeQContext()" */
void uhciFreeQContext(struct PCIController *hc, struct UhciQH *uqh)
{
    struct UhciTD *utd = NULL;
    struct UhciTD *nextutd;

    KPRINTF(5, ("Unlinking QContext %08lx\n", uqh));
    // unlink from schedule
    uqh->uqh_Pred->uxx_Link = uqh->uqh_Succ->uxx_Self;
    SYNC;
    EIEIO;
    uqh->uqh_Succ->uxx_Pred = uqh->uqh_Pred;
    uqh->uqh_Pred->uxx_Succ = uqh->uqh_Succ;
    SYNC;
    EIEIO;

    nextutd = uqh->uqh_FirstTD;
    while(nextutd)
    {
        KPRINTF(1, ("FreeTD %08lx\n", nextutd));
        utd = nextutd;
        nextutd = (struct UhciTD *) utd->utd_Succ;
        uhciFreeTD(hc, utd);
    }
    uhciFreeQH(hc, uqh);
}
/* \\\ */

/* /// "uhciAllocQH()" */
inline struct UhciQH * uhciAllocQH(struct PCIController *hc)
{
    struct UhciQH *uqh = hc->hc_UhciQHPool;

    if(!uqh)
    {
        // out of QHs!
        KPRINTF(20, ("Out of QHs!\n"));
        return NULL;
    }

    hc->hc_UhciQHPool = (struct UhciQH *) uqh->uqh_Succ;
    return(uqh);
}
/* \\\ */

/* /// "uhciFreeQH()" */
inline void uhciFreeQH(struct PCIController *hc, struct UhciQH *uqh)
{
    uqh->uqh_Succ = (struct UhciXX *) hc->hc_UhciQHPool;
    hc->hc_UhciQHPool = uqh;
}
/* \\\ */

/* /// "uhciAllocTD()" */
inline struct UhciTD * uhciAllocTD(struct PCIController *hc)
{
    struct UhciTD *utd = hc->hc_UhciTDPool;

    if(!utd)
    {
        // out of TDs!
        KPRINTF(20, ("Out of TDs!\n"));
        return NULL;
    }

    hc->hc_UhciTDPool = (struct UhciTD *) utd->utd_Succ;
    return(utd);
}
/* \\\ */

/* /// "uhciFreeTD()" */
inline void uhciFreeTD(struct PCIController *hc, struct UhciTD *utd)
{
    utd->utd_Succ = (struct UhciXX *) hc->hc_UhciTDPool;
    hc->hc_UhciTDPool = utd;
}
/* \\\ */

/* /// "uhciUpdateIntTree()" */
void uhciUpdateIntTree(struct PCIController *hc)
{
    struct UhciXX *uxx;
    struct UhciXX *preduxx;
    struct UhciXX *lastuseduxx;
    UWORD cnt;

    // optimize linkage between queue heads
    preduxx = lastuseduxx = (struct UhciXX *) hc->hc_UhciCtrlQH; //hc->hc_UhciIsoTD;
    for(cnt = 0; cnt < 9; cnt++)
    {
        uxx = (struct UhciXX *) hc->hc_UhciIntQH[cnt];
        if(uxx->uxx_Succ != preduxx)
        {
            lastuseduxx = uxx->uxx_Succ;
        }
        uxx->uxx_Link = lastuseduxx->uxx_Self;
        preduxx = uxx;
    }
}
/* \\\ */

/* /// "uhciCheckPortStatusChange()" */
void uhciCheckPortStatusChange(struct PCIController *hc)
{
    struct PCIUnit *unit = hc->hc_Unit;
    UWORD oldval;
    UWORD hciport;

    // check for port status change for UHCI and frame rollovers

    for(hciport = 0; hciport < 2; hciport++)
    {
        UWORD portreg;
        UWORD idx = hc->hc_PortNum20[hciport];
        // don't pay attention to UHCI port changes when pwned by EHCI
        if(!unit->hu_EhciOwned[idx])
        {
            portreg = hciport ? UHCI_PORT2STSCTRL : UHCI_PORT1STSCTRL;
            oldval = READREG16_LE(hc->hc_RegBase, portreg);
            if(oldval & UHPF_ENABLECHANGE)
            {
                KPRINTF(10, ("Port %ld (%ld) Enable changed\n", idx, hciport));
                hc->hc_PortChangeMap[hciport] |= UPSF_PORT_ENABLE;
            }
            if(oldval & UHPF_CONNECTCHANGE)
            {
                KPRINTF(10, ("Port %ld (%ld) Connect changed\n", idx, hciport));
                hc->hc_PortChangeMap[hciport] |= UPSF_PORT_CONNECTION;
                if(!(oldval & UHPF_PORTCONNECTED))
                {
                    if(unit->hu_PortMap20[idx])
                    {
                        KPRINTF(20, ("Transferring Port %ld back to EHCI\n", idx));
                        unit->hu_EhciOwned[idx] = TRUE;
                    }
                }
            }
            if(oldval & UHPF_RESUMEDTX)
            {
                KPRINTF(10, ("Port %ld (%ld) Resume changed\n", idx, hciport));
                hc->hc_PortChangeMap[hciport] |= UPSF_PORT_SUSPEND|UPSF_PORT_ENABLE;
                oldval &= ~UHPF_RESUMEDTX;
            }
            if(hc->hc_PortChangeMap[hciport])
            {
                unit->hu_RootPortChanges |= 1UL<<(idx+1);
                /*KPRINTF(10, ("Port %ld (%ld) contributes %04lx to portmap %04lx\n",
                             idx, hciport, hc->hc_PortChangeMap[hciport], unit->hu_RootPortChanges));*/
            }
            WRITEREG16_LE(hc->hc_RegBase, portreg, oldval);
        }
    }
}
/* \\\ */

/* /// "uhciHandleFinishedTDs()" */
void uhciHandleFinishedTDs(struct PCIController *hc)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    struct IOUsbHWReq *nextioreq;
    struct UhciQH *uqh;
    struct UhciTD *utd;
    UWORD devadrep;
    ULONG len;
    ULONG linkelem;
    UWORD inspect;
    BOOL shortpkt;
    ULONG ctrlstatus;
    ULONG token = 0;
    ULONG actual;
    BOOL updatetree = FALSE;

    KPRINTF(1, ("Checking for work done...\n"));
    ioreq = (struct IOUsbHWReq *) hc->hc_TDQueue.lh_Head;
    while((nextioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ))
    {
        uqh = (struct UhciQH *) ioreq->iouh_DriverPrivate1;
        if(uqh)
        {
            KPRINTF(1, ("Examining IOReq=%08lx with UQH=%08lx\n", ioreq, uqh));
            linkelem = READMEM32_LE(&uqh->uqh_Element);
            inspect = 0;
            devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
            if(linkelem & UHCI_TERMINATE)
            {
                KPRINTF(1, ("UQH terminated %08lx\n", linkelem));
                inspect = 2;
            } else {
                utd = (struct UhciTD *) ((linkelem & UHCI_PTRMASK) - hc->hc_PCIVirtualAdjust - 16); // struct UhciTD starts 16 bytes before physical TD
                ctrlstatus = READMEM32_LE(&utd->utd_CtrlStatus);
                if(!(ctrlstatus & UTCF_ACTIVE))
                {
                    KPRINTF(1, ("CtrlStatus inactive %08lx\n", ctrlstatus));
                    inspect = 1;
                }
                else if(unit->hu_NakTimeoutFrame[devadrep] && (hc->hc_FrameCounter > unit->hu_NakTimeoutFrame[devadrep]))
                {
                    ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                    inspect = 1;
                }
            }
            if(inspect)
            {
                shortpkt = FALSE;
                if(inspect < 2) // if all went okay, don't traverse list, assume all bytes successfully transferred
                {
                    utd = uqh->uqh_FirstTD;
                    actual = 0;
                    do
                    {
                        ctrlstatus = READMEM32_LE(&utd->utd_CtrlStatus);
                        if(ctrlstatus & UTCF_ACTIVE)
                        {
                            KPRINTF(20, ("Internal error! Still active?!\n"));
                            if(ctrlstatus & UTSF_BABBLE)
                            {
                                KPRINTF(200, ("HOST CONTROLLER IS DEAD!!!\n"));
                                ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                                CONSTWRITEREG16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_HCRESET|UHCF_MAXPACKET64|UHCF_CONFIGURE|UHCF_RUNSTOP);
                                inspect = 0;
                                break;
                            }
                            break;
                        }
                        token = READMEM32_LE(&utd->utd_Token);
                        KPRINTF(1, ("TD=%08lx CS=%08lx Token=%08lx\n", utd, ctrlstatus, token));
                        if(ctrlstatus & (UTSF_BABBLE|UTSF_STALLED|UTSF_CRCTIMEOUT|UTSF_DATABUFFERERR|UTSF_BITSTUFFERR))
                        {
                            if(ctrlstatus & UTSF_BABBLE)
                            {
                                KPRINTF(20, ("Babble error %08lx/%08lx\n", ctrlstatus, token));
                                ioreq->iouh_Req.io_Error = UHIOERR_OVERFLOW;
#if 0
                                // VIA chipset seems to die on babble!?!
                                KPRINTF(10, ("HW Regs USBCMD=%04lx\n", READREG16_LE(hc->hc_RegBase, UHCI_USBCMD)));
                                CONSTWRITEREG16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_MAXPACKET64|UHCF_CONFIGURE|UHCF_RUNSTOP);
                                SYNC;
                                EIEIO;
#endif
                                //retry
                                //ctrlstatus &= ~(UTSF_BABBLE|UTSF_STALLED|UTSF_CRCTIMEOUT|UTSF_DATABUFFERERR|UTSF_BITSTUFFERR|UTSF_NAK);
                                ctrlstatus |= UTCF_ACTIVE;
                                WRITEMEM32_LE(&utd->utd_CtrlStatus, ctrlstatus);
                                SYNC;
                                EIEIO;
                                inspect = 3;
                                break;
                            }
                            else if(ctrlstatus & UTSF_CRCTIMEOUT)
                            {
                                KPRINTF(20, ("CRC/Timeout error IOReq=%08lx DIR=%ld\n", ioreq, ioreq->iouh_Dir));
                                if(ctrlstatus & UTSF_STALLED)
                                {
                                    ioreq->iouh_Req.io_Error = UHIOERR_TIMEOUT;
                                } else {
                                    ioreq->iouh_Req.io_Error = (ioreq->iouh_Dir == UHDIR_IN) ? UHIOERR_CRCERROR : UHIOERR_TIMEOUT;
                                }
                            }
                            else if(ctrlstatus & UTSF_STALLED)
                            {
                                KPRINTF(20, ("STALLED!\n"));
                                ioreq->iouh_Req.io_Error = UHIOERR_STALL;
                            }
                            else if(ctrlstatus & UTSF_BITSTUFFERR)
                            {
                                KPRINTF(20, ("Bitstuff error\n"));
                                ioreq->iouh_Req.io_Error = UHIOERR_CRCERROR;
                            }
                            else if(ctrlstatus & UTSF_DATABUFFERERR)
                            {
                                KPRINTF(20, ("Databuffer error\n"));
                                ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                            }
                            inspect = 0;
                            break;
                        }
                        if(unit->hu_NakTimeoutFrame[devadrep] && (hc->hc_FrameCounter > unit->hu_NakTimeoutFrame[devadrep]) && (ctrlstatus & UTSF_NAK))
                        {
                            ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                            inspect = 0;
                        }

                        len = (ctrlstatus & UTSM_ACTUALLENGTH)>>UTSS_ACTUALLENGTH;
                        if((len != (token & UTTM_TRANSLENGTH)>>UTTS_TRANSLENGTH))
                        {
                            shortpkt = TRUE;
                        }
                        len = (len+1) & 0x7ff; // get real length
                        if((token & UTTM_PID)>>UTTS_PID != PID_SETUP) // don't count setup packet
                        {
                            actual += len;
                        }
                        if(shortpkt)
                        {
                            break;
                        }
                    } while((utd = (struct UhciTD *) utd->utd_Succ));
                    if(inspect == 3)
                    {
                        // bail out from babble
                        ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
                        continue;
                    }
                    if((actual < uqh->uqh_Actual) && (!ioreq->iouh_Req.io_Error) && (!(ioreq->iouh_Flags & UHFF_ALLOWRUNTPKTS)))
                    {
                        KPRINTF(10, ("Short packet: %ld < %ld\n", actual, ioreq->iouh_Length));
                        ioreq->iouh_Req.io_Error = UHIOERR_RUNTPACKET;
                    }
                    ioreq->iouh_Actual += actual;
                } else {
                    KPRINTF(10, ("all %ld bytes transferred\n", uqh->uqh_Actual));
                    ioreq->iouh_Actual += uqh->uqh_Actual;
                }
                // this is actually no short packet but result of the VIA babble fix
                if(shortpkt && (ioreq->iouh_Actual == ioreq->iouh_Length))
                {
                    shortpkt = FALSE;
                }
                unit->hu_DevBusyReq[devadrep] = NULL;
                Remove(&ioreq->iouh_Req.io_Message.mn_Node);
                uhciFreeQContext(hc, uqh);
                if(ioreq->iouh_Req.io_Command == UHCMD_INTXFER)
                {
                    updatetree = TRUE;
                }
                if(inspect)
                {
                    if(inspect < 2) // otherwise, toggle will be right already
                    {
                        // use next data toggle bit based on last successful transaction
                        unit->hu_DevDataToggle[devadrep] = (token & UTTF_DATA1) ? FALSE : TRUE;
                    }
                    if(!shortpkt && (ioreq->iouh_Actual < ioreq->iouh_Length))
                    {
                        // fragmented, do some more work
                        switch(ioreq->iouh_Req.io_Command)
                        {
                            case UHCMD_CONTROLXFER:
                                KPRINTF(10, ("Rescheduling CtrlTransfer at %ld of %ld\n", ioreq->iouh_Actual, ioreq->iouh_Length));
                                AddHead(&hc->hc_CtrlXFerQueue, (struct Node *) ioreq);
                                break;

                            case UHCMD_INTXFER:
                                KPRINTF(10, ("Rescheduling IntTransfer at %ld of %ld\n", ioreq->iouh_Actual, ioreq->iouh_Length));
                                AddHead(&hc->hc_IntXFerQueue, (struct Node *) ioreq);
                                break;

                            case UHCMD_BULKXFER:
                                KPRINTF(10, ("Rescheduling BulkTransfer at %ld of %ld\n", ioreq->iouh_Actual, ioreq->iouh_Length));
                                AddHead(&hc->hc_BulkXFerQueue, (struct Node *) ioreq);
                                break;

                            default:
                                KPRINTF(10, ("Uhm, internal error, dunno where to queue this req\n"));
                                ReplyMsg(&ioreq->iouh_Req.io_Message);
                        }
                    } else {
                        // check for sucessful clear feature and set address ctrl transfers
                        if(ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER)
                        {
                            uhwCheckSpecialCtrlTransfers(hc, ioreq);
                        }
                        ReplyMsg(&ioreq->iouh_Req.io_Message);
                    }
                } else {
                    // be sure to save the data toggle bit where the error occurred
                    unit->hu_DevDataToggle[devadrep] = (token & UTTF_DATA1) ? TRUE : FALSE;
                    ReplyMsg(&ioreq->iouh_Req.io_Message);
                }
            }
        } else {
            KPRINTF(20, ("IOReq=%08lx has no UQH!\n", ioreq));
        }
        ioreq = nextioreq;
    }
    if(updatetree)
    {
        KPRINTF(10, ("Updating Tree\n"));
        uhciUpdateIntTree(hc);
    }
}
/* \\\ */

/* /// "uhciScheduleCtrlTDs()" */
void uhciScheduleCtrlTDs(struct PCIController *hc)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    UWORD devadrep;
    struct UhciQH *uqh;
    struct UhciTD *setuputd;
    struct UhciTD *datautd;
    struct UhciTD *termutd;
    struct UhciTD *predutd;
    ULONG actual;
    ULONG ctrlstatus;
    ULONG token;
    ULONG len;
    ULONG phyaddr;
    BOOL cont;

    /* *** CTRL Transfers *** */
    KPRINTF(1, ("Scheduling new CTRL transfers...\n"));
    ioreq = (struct IOUsbHWReq *) hc->hc_CtrlXFerQueue.lh_Head;
    while(((struct Node *) ioreq)->ln_Succ)
    {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint;
        KPRINTF(10, ("New CTRL transfer to %ld.%ld: %ld bytes\n", ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length));
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep])
        {
            KPRINTF(5, ("Endpoint %02lx in use!\n", devadrep));
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }

        uqh = uhciAllocQH(hc);
        if(!uqh)
        {
            break;
        }

        setuputd = uhciAllocTD(hc);
        if(!setuputd)
        {
            uhciFreeQH(hc, uqh);
            break;
        }
        termutd = uhciAllocTD(hc);
        if(!termutd)
        {
            uhciFreeTD(hc, setuputd);
            uhciFreeQH(hc, uqh);
            break;
        }
        uqh->uqh_IOReq = ioreq;

        //termutd->utd_QueueHead = setuputd->utd_QueueHead = uqh;

        KPRINTF(1, ("SetupTD=%08lx, TermTD=%08lx\n", setuputd, termutd));

        // fill setup td
        ctrlstatus = UTCF_ACTIVE|UTCF_3ERRORSLIMIT;
        if(ioreq->iouh_Flags & UHFF_LOWSPEED)
        {
            KPRINTF(5, ("*** LOW SPEED ***\n"));
            ctrlstatus |= UTCF_LOWSPEED;
        }
        token = (ioreq->iouh_DevAddr<<UTTS_DEVADDR)|(ioreq->iouh_Endpoint<<UTTS_ENDPOINT);
        //setuputd->utd_Pred = NULL;
        if(ioreq->iouh_Actual)
        {
            // this is a continuation of a fragmented ctrl transfer!
            KPRINTF(1, ("Continuing FRAGMENT at %ld of %ld\n", ioreq->iouh_Actual, ioreq->iouh_Length));
            cont = TRUE;
        } else {
            cont = FALSE;
            uqh->uqh_FirstTD = setuputd;
            uqh->uqh_Element = setuputd->utd_Self; // start of queue
            WRITEMEM32_LE(&setuputd->utd_CtrlStatus, ctrlstatus);
            WRITEMEM32_LE(&setuputd->utd_Token, (PID_SETUP<<UTTS_PID)|token|(7<<UTTS_TRANSLENGTH)|UTTF_DATA0);
            WRITEMEM32_LE(&setuputd->utd_BufferPtr, (ULONG) pciGetPhysical(hc, &ioreq->iouh_SetupData));
        }

        token |= (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? PID_IN : PID_OUT;
        predutd = setuputd;
        actual = ioreq->iouh_Actual;
        if(ioreq->iouh_Length)
        {
            ctrlstatus |= UTCF_SHORTPACKET;
            if(cont)
            {
                phyaddr = (ULONG) pciGetPhysical(hc, &(((UBYTE *) ioreq->iouh_Data)[ioreq->iouh_Actual]));
                if(!unit->hu_DevDataToggle[devadrep])
                {
                    // continue with data toggle 0
                    token |= UTTF_DATA1;
                }
            } else {
                phyaddr = (ULONG) pciGetPhysical(hc, ioreq->iouh_Data);
            }
            do
            {
                datautd = uhciAllocTD(hc);
                if(!datautd)
                {
                    break;
                }
                token ^= UTTF_DATA1; // toggle bit
                predutd->utd_Link = datautd->utd_Self;
                predutd->utd_Succ = (struct UhciXX *) datautd;
                //datautd->utd_Pred = (struct UhciXX *) predutd;
                //datautd->utd_QueueHead = uqh;
                len = ioreq->iouh_Length - actual;
                if(len > ioreq->iouh_MaxPktSize)
                {
                    len = ioreq->iouh_MaxPktSize;
                }
                WRITEMEM32_LE(&datautd->utd_CtrlStatus, ctrlstatus);
#if 1
#warning "this workaround for a VIA babble bug will potentially overwrite innocent memory (very rarely), but will avoid the host controller dropping dead completely."
                if((len < ioreq->iouh_MaxPktSize) && (ioreq->iouh_SetupData.bmRequestType & URTF_IN))
                {
                    WRITEMEM32_LE(&datautd->utd_Token, token|((ioreq->iouh_MaxPktSize-1)<<UTTS_TRANSLENGTH)); // no masking need here as len is always >= 1
                } else {
                    WRITEMEM32_LE(&datautd->utd_Token, token|((len-1)<<UTTS_TRANSLENGTH)); // no masking need here as len is always >= 1
                }
#else
                WRITEMEM32_LE(&datautd->utd_Token, token|((len-1)<<UTTS_TRANSLENGTH)); // no masking need here as len is always >= 1
#endif
                WRITEMEM32_LE(&datautd->utd_BufferPtr, phyaddr);
                phyaddr += len;
                actual += len;
                predutd = datautd;
            } while((actual < ioreq->iouh_Length) && (actual - ioreq->iouh_Actual < UHCI_TD_CTRL_LIMIT));
            if(actual == ioreq->iouh_Actual)
            {
                // not at least one data TD? try again later
                uhciFreeTD(hc, setuputd);
                uhciFreeTD(hc, termutd);
                uhciFreeQH(hc, uqh);
                break;
            }
            if(cont)
            {
                // free Setup packet
                KPRINTF(1, ("Freeing setup\n"));
                uqh->uqh_FirstTD = (struct UhciTD *) setuputd->utd_Succ;
                //uqh->uqh_FirstTD->utd_Pred = NULL;
                uqh->uqh_Element = setuputd->utd_Succ->uxx_Self; // start of queue after setup packet
                uhciFreeTD(hc, setuputd);
                // set toggle for next batch
                unit->hu_DevDataToggle[devadrep] = (token & UTTF_DATA1) ? FALSE : TRUE;
            }
        }
        uqh->uqh_Actual = actual - ioreq->iouh_Actual;
        ctrlstatus |= UTCF_READYINTEN;
        if(actual == ioreq->iouh_Length)
        {
            // TERM packet
            KPRINTF(1, ("Activating TERM\n"));
            token |= UTTF_DATA1;
            token ^= (PID_IN^PID_OUT)<<UTTS_PID;

            predutd->utd_Link = termutd->utd_Self;
            predutd->utd_Succ = (struct UhciXX *) termutd;
            //termutd->utd_Pred = (struct UhciXX *) predutd;
            WRITEMEM32_LE(&termutd->utd_CtrlStatus, ctrlstatus);
            WRITEMEM32_LE(&termutd->utd_Token, token|(0x7ff<<UTTS_TRANSLENGTH));
            CONSTWRITEMEM32_LE(&termutd->utd_Link, UHCI_TERMINATE);
            termutd->utd_Succ = NULL;
            //uqh->uqh_LastTD = termutd;
        } else {
            KPRINTF(1, ("Setup data phase fragmented\n"));
            // don't create TERM, we don't know the final data toggle bit
            // but mark the last data TD for interrupt generation
            WRITEMEM32_LE(&predutd->utd_CtrlStatus, ctrlstatus);
            uhciFreeTD(hc, termutd);
            CONSTWRITEMEM32_LE(&predutd->utd_Link, UHCI_TERMINATE);
            predutd->utd_Succ = NULL;
            //uqh->uqh_LastTD = predutd;
        }

        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        ioreq->iouh_DriverPrivate1 = uqh;

        // manage endpoint going busy
        unit->hu_DevBusyReq[devadrep] = ioreq;
        unit->hu_NakTimeoutFrame[devadrep] = (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + ioreq->iouh_NakTimeout : 0;

        Disable();
        AddTail(&hc->hc_TDQueue, (struct Node *) ioreq);

        // looks good to me, now enqueue this entry (just behind the CtrlQH)
        uqh->uqh_Succ = hc->hc_UhciCtrlQH->uqh_Succ;
        uqh->uqh_Link = uqh->uqh_Succ->uxx_Self;
        SYNC;
        EIEIO;
        uqh->uqh_Pred = (struct UhciXX *) hc->hc_UhciCtrlQH;
        uqh->uqh_Succ->uxx_Pred = (struct UhciXX *) uqh;
        hc->hc_UhciCtrlQH->uqh_Succ = (struct UhciXX *) uqh;
        hc->hc_UhciCtrlQH->uqh_Link = uqh->uqh_Self;
        SYNC;
        EIEIO;
        Enable();

        ioreq = (struct IOUsbHWReq *) hc->hc_CtrlXFerQueue.lh_Head;
    }
}
/* \\\ */

/* /// "uhciScheduleIntTDs()" */
void uhciScheduleIntTDs(struct PCIController *hc)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    UWORD cnt;
    UWORD devadrep;
    struct UhciQH *uqh;
    struct UhciQH *intuqh;
    struct UhciTD *utd;
    struct UhciTD *predutd;
    ULONG actual;
    ULONG ctrlstatus;
    ULONG token;
    ULONG len;
    ULONG phyaddr;

    /* *** INT Transfers *** */
    KPRINTF(1, ("Scheduling new INT transfers...\n"));
    ioreq = (struct IOUsbHWReq *) hc->hc_IntXFerQueue.lh_Head;
    while(((struct Node *) ioreq)->ln_Succ)
    {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
        KPRINTF(10, ("New INT transfer to %ld.%ld: %ld bytes\n", ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length));
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep])
        {
            KPRINTF(5, ("Endpoint %02lx in use!\n", devadrep));
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }

        uqh = uhciAllocQH(hc);
        if(!uqh)
        {
            break;
        }

        uqh->uqh_IOReq = ioreq;

        ctrlstatus = UTCF_ACTIVE|UTCF_1ERRORLIMIT;
        if(ioreq->iouh_Flags & UHFF_LOWSPEED)
        {
            KPRINTF(5, ("*** LOW SPEED ***\n"));
            ctrlstatus |= UTCF_LOWSPEED;
        }
        token = (ioreq->iouh_DevAddr<<UTTS_DEVADDR)|(ioreq->iouh_Endpoint<<UTTS_ENDPOINT);
        token |= (ioreq->iouh_Dir == UHDIR_IN) ? PID_IN : PID_OUT;
        predutd = NULL;
        actual = ioreq->iouh_Actual;
        ctrlstatus |= UTCF_SHORTPACKET;
        phyaddr = (ULONG) pciGetPhysical(hc, &(((UBYTE *) ioreq->iouh_Data)[ioreq->iouh_Actual]));
        if(unit->hu_DevDataToggle[devadrep])
        {
            // continue with data toggle 1
            KPRINTF(1, ("Data1\n"));
            token |= UTTF_DATA1;
        } else {
            KPRINTF(1, ("Data0\n"));
        }
        do
        {
            utd = uhciAllocTD(hc);
            if(!utd)
            {
                break;
            }
            if(predutd)
            {
                WRITEMEM32_LE(&predutd->utd_Link, READMEM32_LE(utd->utd_Self)|UHCI_DFS);
                predutd->utd_Succ = (struct UhciXX *) utd;
                //utd->utd_Pred = (struct UhciXX *) predutd;
            } else {
                uqh->uqh_FirstTD = utd;
                uqh->uqh_Element = utd->utd_Self;
                //utd->utd_Pred = NULL;
            }
            //utd->utd_QueueHead = uqh;
            len = ioreq->iouh_Length - actual;
            if(len > ioreq->iouh_MaxPktSize)
            {
                len = ioreq->iouh_MaxPktSize;
            }

            WRITEMEM32_LE(&utd->utd_CtrlStatus, ctrlstatus);
            WRITEMEM32_LE(&utd->utd_Token, token|(((len-1) & 0x7ff)<<UTTS_TRANSLENGTH));
            WRITEMEM32_LE(&utd->utd_BufferPtr, phyaddr);
            phyaddr += len;
            actual += len;
            predutd = utd;
            token ^= UTTF_DATA1; // toggle bit
        } while((actual < ioreq->iouh_Length) && (actual - ioreq->iouh_Actual < UHCI_TD_INT_LIMIT));

        if(!utd)
        {
            // not at least one data TD? try again later
            uhciFreeQH(hc, uqh);
            break;
        }

        uqh->uqh_Actual = actual - ioreq->iouh_Actual;
        // set toggle for next batch / succesful transfer
        unit->hu_DevDataToggle[devadrep] = (token & UTTF_DATA1) ? TRUE : FALSE;
        if(unit->hu_DevDataToggle[devadrep])
        {
            // continue with data toggle 1
            KPRINTF(1, ("NewData1\n"));
        } else {
            KPRINTF(1, ("NewData0\n"));
        }
        ctrlstatus |= UTCF_READYINTEN;
        WRITEMEM32_LE(&predutd->utd_CtrlStatus, ctrlstatus);
        CONSTWRITEMEM32_LE(&utd->utd_Link, UHCI_TERMINATE);
        utd->utd_Succ = NULL;
        //uqh->uqh_LastTD = utd;

        if(ioreq->iouh_Interval >= 255)
        {
            intuqh = hc->hc_UhciIntQH[8]; // 256ms interval
        } else {
            cnt = 0;
            do
            {
                intuqh = hc->hc_UhciIntQH[cnt++];
            } while(ioreq->iouh_Interval > (1<<cnt));
            KPRINTF(1, ("Scheduled at level %ld\n", cnt));
        }

        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        ioreq->iouh_DriverPrivate1 = uqh;

        // manage endpoint going busy
        unit->hu_DevBusyReq[devadrep] = ioreq;
        unit->hu_NakTimeoutFrame[devadrep] = (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + ioreq->iouh_NakTimeout : 0;

        Disable();
        AddTail(&hc->hc_TDQueue, (struct Node *) ioreq);

        // looks good to me, now enqueue this entry (just behind the right IntQH)
        uqh->uqh_Succ = intuqh->uqh_Succ;
        uqh->uqh_Link = intuqh->uqh_Self;
        SYNC;
        EIEIO;
        uqh->uqh_Pred = (struct UhciXX *) intuqh;
        uqh->uqh_Succ->uxx_Pred = (struct UhciXX *) uqh;
        intuqh->uqh_Succ = (struct UhciXX *) uqh;
        intuqh->uqh_Link = uqh->uqh_Self;
        SYNC;
        EIEIO;
        Enable();

        uhciUpdateIntTree(hc);

        ioreq = (struct IOUsbHWReq *) hc->hc_IntXFerQueue.lh_Head;
    }
}
/* \\\ */

/* /// "uhciScheduleBulkTDs()" */
void uhciScheduleBulkTDs(struct PCIController *hc)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    UWORD devadrep;
    struct UhciQH *uqh;
    struct UhciTD *utd;
    struct UhciTD *predutd;
    ULONG actual;
    ULONG ctrlstatus;
    ULONG token;
    ULONG len;
    ULONG phyaddr;
    BOOL forcezero;

    /* *** BULK Transfers *** */
    KPRINTF(1, ("Scheduling new BULK transfers...\n"));
    ioreq = (struct IOUsbHWReq *) hc->hc_BulkXFerQueue.lh_Head;
    while(((struct Node *) ioreq)->ln_Succ)
    {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
        KPRINTF(10, ("New BULK transfer to %ld.%ld: %ld bytes\n", ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length));
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep])
        {
            KPRINTF(5, ("Endpoint %02lx in use!\n", devadrep));
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }

        uqh = uhciAllocQH(hc);
        if(!uqh)
        {
            break;
        }

        uqh->uqh_IOReq = ioreq;

        // fill setup td
        ctrlstatus = UTCF_ACTIVE|UTCF_1ERRORLIMIT;
        token = (ioreq->iouh_DevAddr<<UTTS_DEVADDR)|(ioreq->iouh_Endpoint<<UTTS_ENDPOINT);
        token |= (ioreq->iouh_Dir == UHDIR_IN) ? PID_IN : PID_OUT;
        predutd = NULL;
        actual = ioreq->iouh_Actual;
        ctrlstatus |= UTCF_SHORTPACKET;
        phyaddr = (ULONG) pciGetPhysical(hc, &(((UBYTE *) ioreq->iouh_Data)[ioreq->iouh_Actual]));
        if(unit->hu_DevDataToggle[devadrep])
        {
            // continue with data toggle 1
            token |= UTTF_DATA1;
        }
        do
        {
            utd = uhciAllocTD(hc);
            if(!utd)
            {
                break;
            }
            forcezero = FALSE;
            if(predutd)
            {
                WRITEMEM32_LE(&predutd->utd_Link, READMEM32_LE(utd->utd_Self)|UHCI_DFS);
                predutd->utd_Succ = (struct UhciXX *) utd;
                //utd->utd_Pred = (struct UhciXX *) predutd;
            } else {
                uqh->uqh_FirstTD = utd;
                uqh->uqh_Element = utd->utd_Self;
                //utd->utd_Pred = NULL;
            }
            //utd->utd_QueueHead = uqh;
            len = ioreq->iouh_Length - actual;
            if(len > ioreq->iouh_MaxPktSize)
            {
                len = ioreq->iouh_MaxPktSize;
            }

            WRITEMEM32_LE(&utd->utd_CtrlStatus, ctrlstatus);
            WRITEMEM32_LE(&utd->utd_Token, token|(((len-1) & 0x7ff)<<UTTS_TRANSLENGTH));
            WRITEMEM32_LE(&utd->utd_BufferPtr, phyaddr);
            phyaddr += len;
            actual += len;
            predutd = utd;
            token ^= UTTF_DATA1; // toggle bit
            if((actual == ioreq->iouh_Length) && len)
            {
                if((ioreq->iouh_Flags & UHFF_NOSHORTPKT) || (ioreq->iouh_Dir == UHDIR_IN) || (actual % ioreq->iouh_MaxPktSize))
                {
                    // no last zero byte packet
                    break;
                } else {
                    // avoid rare case that the zero byte packet is reached on TD_BULK_LIMIT
                    forcezero = TRUE;
                }
            }
        } while(forcezero || (len && (actual <= ioreq->iouh_Length) && (actual - ioreq->iouh_Actual < UHCI_TD_BULK_LIMIT)));

        if(!utd)
        {
            // not at least one data TD? try again later
            uhciFreeQH(hc, uqh);
            break;
        }
        uqh->uqh_Actual = actual - ioreq->iouh_Actual;
        // set toggle for next batch / succesful transfer
        unit->hu_DevDataToggle[devadrep] = (token & UTTF_DATA1) ? TRUE : FALSE;

        ctrlstatus |= UTCF_READYINTEN;
        WRITEMEM32_LE(&predutd->utd_CtrlStatus, ctrlstatus);
        CONSTWRITEMEM32_LE(&utd->utd_Link, UHCI_TERMINATE);
        utd->utd_Succ = NULL;
        //uqh->uqh_LastTD = utd;

        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        ioreq->iouh_DriverPrivate1 = uqh;

        // manage endpoint going busy
        unit->hu_DevBusyReq[devadrep] = ioreq;
        unit->hu_NakTimeoutFrame[devadrep] = (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + ioreq->iouh_NakTimeout : 0;

        Disable();
        AddTail(&hc->hc_TDQueue, (struct Node *) ioreq);

        // looks good to me, now enqueue this entry (just behind the BulkQH)
        uqh->uqh_Succ = hc->hc_UhciBulkQH->uqh_Succ;
        uqh->uqh_Link = uqh->uqh_Succ->uxx_Self;
        SYNC;
        EIEIO;
        uqh->uqh_Pred = (struct UhciXX *) hc->hc_UhciBulkQH;
        uqh->uqh_Succ->uxx_Pred = (struct UhciXX *) uqh;
        hc->hc_UhciBulkQH->uqh_Succ = (struct UhciXX *) uqh;
        hc->hc_UhciBulkQH->uqh_Link = uqh->uqh_Self;
        SYNC;
        EIEIO;
        Enable();

        ioreq = (struct IOUsbHWReq *) hc->hc_BulkXFerQueue.lh_Head;
    }
}
/* \\\ */

/* /// "uhciCompleteInt()" */
void uhciCompleteInt(struct PCIController *hc)
{
    ULONG framecnt = READREG16_LE(hc->hc_RegBase, UHCI_FRAMECOUNT);

    KPRINTF(1, ("CompleteInt!\n"));
    if(framecnt < (hc->hc_FrameCounter & 0xffff))
    {
        hc->hc_FrameCounter |= 0xffff;
        hc->hc_FrameCounter++;
        hc->hc_FrameCounter += framecnt;
        KPRINTF(10, ("Frame Counter Rollover %ld\n", hc->hc_FrameCounter));
    }

    /* **************** PROCESS DONE TRANSFERS **************** */

    uhciCheckPortStatusChange(hc);
    uhwCheckRootHubChanges(hc->hc_Unit);

    uhciHandleFinishedTDs(hc);

    if(hc->hc_CtrlXFerQueue.lh_Head->ln_Succ)
    {
        uhciScheduleCtrlTDs(hc);
    }

    if(hc->hc_IntXFerQueue.lh_Head->ln_Succ)
    {
        uhciScheduleIntTDs(hc);
    }

    if(hc->hc_BulkXFerQueue.lh_Head->ln_Succ)
    {
        uhciScheduleBulkTDs(hc);
    }

    KPRINTF(1, ("CompleteDone\n"));
}
/* \\\ */

/* /// "uhciIntCode()" */
void uhciIntCode(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
    struct PCIController *hc = (struct PCIController *) irq->h_Data;
    struct PCIDevice *base = hc->hc_Device;
    UWORD intr;

    //KPRINTF(10, ("pciUhciInt()\n"));
    intr = READREG16_LE(hc->hc_RegBase, UHCI_USBSTATUS);
    if(intr & (UHSF_USBINT|UHSF_USBERRORINT|UHSF_RESUMEDTX|UHSF_HCSYSERROR|UHSF_HCPROCERROR|UHSF_HCHALTED))
    {
        WRITEREG16_LE(hc->hc_RegBase, UHCI_USBSTATUS, intr);
        KPRINTF(1, ("INT=%04lx\n", intr));
        if(intr & (UHSF_HCSYSERROR|UHSF_HCPROCERROR|UHSF_HCHALTED))
        {
            KPRINTF(200, ("Host ERROR!\n"));
            CONSTWRITEREG16_LE(hc->hc_RegBase, UHCI_USBCMD, UHCF_HCRESET|UHCF_GLOBALRESET|UHCF_MAXPACKET64|UHCF_CONFIGURE);
            //CONSTWRITEREG16_LE(hc->hc_RegBase, UHCI_USBINTEN, 0);
        }
        if(!hc->hc_Online)
        {
            return;
        }
        if(intr & (UHSF_USBINT|UHSF_USBERRORINT))
        {
            SureCause(base, &hc->hc_CompleteInt);
        }
    }
}
/* \\\ */

/* ---------------------------------------------------------------------- *
 *                    OHCI Specific Stuff                                 *
 * ---------------------------------------------------------------------- */

/* /// "ohciDebugSchedule()" */
void ohciDebugSchedule(struct PCIController *hc)
{
    ULONG ctrlhead;
    ULONG hced;
    ULONG epcaps;
    ULONG headptr;
    ULONG headptrbits;
    ULONG tailptr;
    ULONG nexted;
    ULONG ctrl;
    ULONG currptr;
    ULONG nexttd;
    ULONG buffend;
    KPRINTF(10, ("*** Schedule debug!!! ***\n"));
    ctrlhead = READREG32_LE(hc->hc_RegBase, OHCI_CTRL_HEAD_ED) - hc->hc_PCIVirtualAdjust;
    KPRINTF(10, ("CtrlHead = %08lx, should be %08lx\n", ctrlhead, &hc->hc_OhciCtrlHeadED->oed_EPCaps));
    hced = ctrlhead;
    do
    {
        epcaps = READMEM32_LE(hced);
        tailptr = READMEM32_LE(hced+4);
        headptr = headptrbits = READMEM32_LE(hced+8);
        headptr &= OHCI_PTRMASK;
        nexted = READMEM32_LE(hced+12);
        KPRINTF(10, ("ED %08lx: EPCaps=%08lx, HeadP=%08lx, TailP=%08lx, NextED=%08lx\n",
                     hced, epcaps, headptrbits, tailptr, nexted));
        if((!(epcaps & OECF_SKIP)) && (tailptr != headptr) && (!(headptrbits & OEHF_HALTED)))
        {
            while(tailptr != headptr)
            {
                headptr -= hc->hc_PCIVirtualAdjust;
                ctrl = READMEM32_LE(headptr);
                currptr = READMEM32_LE(headptr+4);
                nexttd = READMEM32_LE(headptr+8);
                buffend = READMEM32_LE(headptr+12);

                KPRINTF(5, ("  TD %08lx: Ctrl=%08lx, CurrPtr=%08lx, NextTD=%08lx, BuffEnd=%08lx\n",
                            headptr, ctrl, currptr, nexttd, buffend));
                headptr = nexttd;
            }
        }
        if(!nexted)
        {
            break;
        }
        hced = nexted - hc->hc_PCIVirtualAdjust;
    } while(TRUE);
}
/* \\\ */

/* /// "ohciFreeEDContext()" */
void ohciFreeEDContext(struct PCIController *hc, struct OhciED *oed)
{
    struct OhciTD *otd;
    struct OhciTD *nextotd;

    KPRINTF(5, ("Unlinking EDContext %08lx\n", oed));

    // unlink from schedule
    oed->oed_Succ->oed_Pred = oed->oed_Pred;
    oed->oed_Pred->oed_Succ = oed->oed_Succ;
    oed->oed_Pred->oed_NextED = oed->oed_Succ->oed_Self;
    SYNC
    EIEIO;

#if 0
    // need to make sure that the endpoint is no longer
    Disable();
    oed->oed_Succ = hc->hc_OhciAsyncFreeED;
    hc->hc_OhciAsyncFreeED = oed;
    Enable();
#else
    Disable();
    nextotd = oed->oed_FirstTD;
    while(nextotd)
    {
        KPRINTF(1, ("FreeTD %08lx\n", nextotd));
        otd = nextotd;
        nextotd = (struct OhciTD *) otd->otd_Succ;
        ohciFreeTD(hc, otd);
    }

    ohciFreeED(hc, oed);
    Enable();
#endif
}
/* \\\ */

/* /// "ohciAllocED()" */
inline struct OhciED * ohciAllocED(struct PCIController *hc)
{
    struct OhciED *oed = hc->hc_OhciEDPool;

    if(!oed)
    {
        // out of QHs!
        KPRINTF(20, ("Out of EDs!\n"));
        return NULL;
    }

    hc->hc_OhciEDPool = oed->oed_Succ;
    return(oed);
}
/* \\\ */

/* /// "ohciFreeED()" */
inline void ohciFreeED(struct PCIController *hc, struct OhciED *oed)
{
    oed->oed_Succ = hc->hc_OhciEDPool;
    hc->hc_OhciEDPool = oed;
}
/* \\\ */

/* /// "ohciAllocTD()" */
inline struct OhciTD * ohciAllocTD(struct PCIController *hc)
{
    struct OhciTD *otd = hc->hc_OhciTDPool;

    if(!otd)
    {
        // out of TDs!
        KPRINTF(20, ("Out of TDs!\n"));
        return NULL;
    }

    hc->hc_OhciTDPool = otd->otd_Succ;
    return(otd);
}
/* \\\ */

/* /// "ohciFreeTD()" */
inline void ohciFreeTD(struct PCIController *hc, struct OhciTD *otd)
{
    otd->otd_Succ = hc->hc_OhciTDPool;
    hc->hc_OhciTDPool = otd;
}
/* \\\ */

/* /// "ohciUpdateIntTree()" */
void ohciUpdateIntTree(struct PCIController *hc)
{
    struct OhciED *oed;
    struct OhciED *predoed;
    struct OhciED *lastusedoed;
    UWORD cnt;

    // optimize linkage between queue heads
    predoed = lastusedoed = hc->hc_OhciTermED;
    for(cnt = 0; cnt < 5; cnt++)
    {
        oed = hc->hc_OhciIntED[cnt];
        if(oed->oed_Succ != predoed)
        {
            lastusedoed = oed->oed_Succ;
        }
        oed->oed_NextED = lastusedoed->oed_Self;
        predoed = oed;
    }
}
/* \\\ */

/* /// "ohciHandleFinishedTDs()" */
void ohciHandleFinishedTDs(struct PCIController *hc)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    struct IOUsbHWReq *nextioreq;
    struct OhciED *oed;
    struct OhciTD *otd;
    UWORD devadrep;
    ULONG len;
    ULONG ctrlstatus;
    BOOL updatetree = FALSE;
    ULONG donehead;
    BOOL retire;

    KPRINTF(1, ("Checking for work done...\n"));
    Disable();
    donehead = hc->hc_OhciDoneQueue;
    hc->hc_OhciDoneQueue = 0UL;
    Enable();
    if(!donehead)
    {
        KPRINTF(1, ("Nothing to do!\n"));
        return;
    }
    otd = (struct OhciTD *) (donehead - hc->hc_PCIVirtualAdjust - 16);
    KPRINTF(10, ("DoneHead=%08lx, OTD=%08lx, Frame=%ld\n", donehead, otd, READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT)));
    do
    {
        oed = otd->otd_ED;
        ctrlstatus = READMEM32_LE(&otd->otd_Ctrl);
        if(otd->otd_BufferPtr)
        {
            // FIXME this will blow up if physical memory is ever going to be discontinuous
            len = READMEM32_LE(&otd->otd_BufferEnd) - READMEM32_LE(&otd->otd_BufferPtr) + 1;
        } else {
            len = otd->otd_Length;
        }
        ioreq = oed->oed_IOReq;
        KPRINTF(1, ("Examining TD %08lx for ED %08lx (IOReq=%08lx), Status %08lx, len=%ld\n", otd, oed, ioreq, ctrlstatus, len));
        ioreq->iouh_Actual += len;
        retire = (ioreq->iouh_Actual == ioreq->iouh_Length);
        if((ctrlstatus & OTCM_DELAYINT) != OTCF_NOINT)
        {
            retire = TRUE;
        }
        switch((ctrlstatus & OTCM_COMPLETIONCODE)>>OTCS_COMPLETIONCODE)
        {
            case (OTCF_CC_NOERROR>>OTCS_COMPLETIONCODE):
                break;

            case (OTCF_CC_CRCERROR>>OTCS_COMPLETIONCODE):
                KPRINTF(200, ("CRC Error!\n"));
                ioreq->iouh_Req.io_Error = UHIOERR_CRCERROR;
                retire = TRUE;
                break;

            case (OTCF_CC_BABBLE>>OTCS_COMPLETIONCODE):
                KPRINTF(200, ("Babble/Bitstuffing Error!\n"));
                ioreq->iouh_Req.io_Error = UHIOERR_CRCERROR;
                retire = TRUE;
                break;

            case (OTCF_CC_WRONGTOGGLE>>OTCS_COMPLETIONCODE):
                KPRINTF(200, ("Data toggle mismatch length = %ld\n", len));
                break;

            case (OTCF_CC_STALL>>OTCS_COMPLETIONCODE):
                KPRINTF(200, ("STALLED!\n"));
                ioreq->iouh_Req.io_Error = UHIOERR_STALL;
                retire = TRUE;
                break;

            case (OTCF_CC_TIMEOUT>>OTCS_COMPLETIONCODE):
                KPRINTF(200, ("TIMEOUT!\n"));
                ioreq->iouh_Req.io_Error = UHIOERR_TIMEOUT;
                retire = TRUE;
                break;

            case (OTCF_CC_PIDCORRUPT>>OTCS_COMPLETIONCODE):
                KPRINTF(200, ("PID Error!\n"));
                ioreq->iouh_Req.io_Error = UHIOERR_CRCERROR;
                retire = TRUE;
                break;

            case (OTCF_CC_WRONGPID>>OTCS_COMPLETIONCODE):
                KPRINTF(200, ("Illegal PID!\n"));
                ioreq->iouh_Req.io_Error = UHIOERR_CRCERROR;
                retire = TRUE;
                break;

            case (OTCF_CC_OVERFLOW>>OTCS_COMPLETIONCODE):
                KPRINTF(200, ("Overflow Error!\n"));
                ioreq->iouh_Req.io_Error = UHIOERR_OVERFLOW;
                retire = TRUE;
                break;

            case (OTCF_CC_SHORTPKT>>OTCS_COMPLETIONCODE):
                KPRINTF(10, ("Short packet %ld < %ld\n", len, otd->otd_Length));
                if((!ioreq->iouh_Req.io_Error) && (!(ioreq->iouh_Flags & UHFF_ALLOWRUNTPKTS)))
                {
                    ioreq->iouh_Req.io_Error = UHIOERR_RUNTPACKET;
                }
                retire = TRUE;
                break;

            case (OTCF_CC_OVERRUN>>OTCS_COMPLETIONCODE):
                KPRINTF(200, ("Data Overrun Error!\n"));
                ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                retire = TRUE;
                break;

            case (OTCF_CC_UNDERRUN>>OTCS_COMPLETIONCODE):
                KPRINTF(200, ("Data Underrun Error!\n"));
                ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                retire = TRUE;
                break;

            case (OTCF_CC_INVALID>>OTCS_COMPLETIONCODE):
                KPRINTF(200, ("Not touched?!?\n"));
                break;
        }
        if(READMEM32_LE(&oed->oed_HeadPtr) & OEHF_HALTED)
        {
            KPRINTF(100, ("OED halted!\n"));
            retire = TRUE;
        }

        if(retire)
        {
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            AddHead(&hc->hc_OhciRetireQueue, &ioreq->iouh_Req.io_Message.mn_Node);
        }

        if(!otd->otd_NextTD)
        {
            break;
        }
        KPRINTF(1, ("NextTD=%08lx\n", otd->otd_NextTD));
        otd = (struct OhciTD *) (READMEM32_LE(&otd->otd_NextTD) - hc->hc_PCIVirtualAdjust - 16);
        KPRINTF(1, ("NextOTD = %08lx\n", otd));
    } while(TRUE);

    ioreq = (struct IOUsbHWReq *) hc->hc_OhciRetireQueue.lh_Head;
    while((nextioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ))
    {
        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        oed = (struct OhciED *) ioreq->iouh_DriverPrivate1;
        if(oed)
        {
            KPRINTF(10, ("Retiring IOReq=%08lx ED=%08lx, Frame=%ld\n", ioreq, oed, READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT)));

            if(oed->oed_Continue)
            {
                ULONG actual = ioreq->iouh_Actual;
                ULONG oldenables;
                ULONG phyaddr;
                struct OhciTD *predotd = NULL;

                KPRINTF(10, ("Reloading Bulk transfer at %ld of %ld\n", ioreq->iouh_Actual, ioreq->iouh_Length));
                otd = oed->oed_FirstTD;
                phyaddr = (ULONG) pciGetPhysical(hc, &(((UBYTE *) ioreq->iouh_Data)[actual]));
                do
                {
                    len = ioreq->iouh_Length - actual;
                    if(len > OHCI_PAGE_SIZE)
                    {
                        len = OHCI_PAGE_SIZE;
                    }
                    if((!otd->otd_Succ) && (actual + len == ioreq->iouh_Length) && (!ioreq->iouh_Flags & UHFF_NOSHORTPKT) && ((actual % ioreq->iouh_MaxPktSize) == 0))
                    {
                        // special case -- zero padding would not fit in this run,
                        // and next time, we would forget about it. So rather abort
                        // reload now, so the zero padding goes with the next reload
                        break;
                    }
                    predotd = otd;
                    otd->otd_Length = len;
                    KPRINTF(1, ("TD with %ld bytes\n", len));
                    CONSTWRITEMEM32_LE(&otd->otd_Ctrl, OTCF_CC_INVALID|OTCF_NOINT);
                    if(otd->otd_Succ)
                    {
                        otd->otd_NextTD = otd->otd_Succ->otd_Self;
                    }
                    if(len)
                    {
                        WRITEMEM32_LE(&otd->otd_BufferPtr, phyaddr);
                        phyaddr += len - 1;
                        WRITEMEM32_LE(&otd->otd_BufferEnd, phyaddr);
                        phyaddr++;
                    } else {
                        CONSTWRITEMEM32_LE(&otd->otd_BufferPtr, 0);
                        CONSTWRITEMEM32_LE(&otd->otd_BufferEnd, 0);
                    }
                    actual += len;
                    otd = otd->otd_Succ;
                } while(otd && ((actual < ioreq->iouh_Length) || (len && (ioreq->iouh_Dir == UHDIR_OUT) && (actual == ioreq->iouh_Length) && (!ioreq->iouh_Flags & UHFF_NOSHORTPKT) && ((actual % ioreq->iouh_MaxPktSize) == 0))));
                oed->oed_Continue = (actual < ioreq->iouh_Length);
                predotd->otd_NextTD = hc->hc_OhciTermTD->otd_Self;

                CONSTWRITEMEM32_LE(&predotd->otd_Ctrl, OTCF_CC_INVALID);

                Disable();
                AddTail(&hc->hc_TDQueue, &ioreq->iouh_Req.io_Message.mn_Node);

                // keep toggle bit
                ctrlstatus = READMEM32_LE(oed->oed_HeadPtr) & OEHF_DATA1;
                WRITEMEM32_LE(&oed->oed_HeadPtr, READMEM32_LE(oed->oed_FirstTD->otd_Self)|ctrlstatus);

                oldenables = READREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS);
                oldenables |= OCSF_BULKENABLE;
                WRITEREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS, oldenables);
                SYNC;
                EIEIO;
                Enable();
            } else {
                // disable ED
                ctrlstatus = READMEM32_LE(&oed->oed_HeadPtr);
                ctrlstatus |= OEHF_HALTED;
                WRITEMEM32_LE(&oed->oed_HeadPtr, ctrlstatus);

                devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
                unit->hu_DevBusyReq[devadrep] = NULL;
                unit->hu_DevDataToggle[devadrep] = (ctrlstatus & OEHF_DATA1) ? TRUE : FALSE;

                ohciFreeEDContext(hc, oed);
                if(ioreq->iouh_Req.io_Command == UHCMD_INTXFER)
                {
                    updatetree = TRUE;
                }
                // check for sucessful clear feature and set address ctrl transfers
                if((!ioreq->iouh_Req.io_Error) && (ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER))
                {
                    uhwCheckSpecialCtrlTransfers(hc, ioreq);
                }
                ReplyMsg(&ioreq->iouh_Req.io_Message);
            }
        } else {
            KPRINTF(20, ("IOReq=%08lx has no OED!\n", ioreq));
        }
        ioreq = nextioreq;
    }
    if(updatetree)
    {
        ohciUpdateIntTree(hc);
    }
}
/* \\\ */

/* /// "ohciScheduleCtrlTDs()" */
void ohciScheduleCtrlTDs(struct PCIController *hc)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    UWORD devadrep;
    struct OhciED *oed;
    struct OhciTD *setupotd;
    struct OhciTD *dataotd;
    struct OhciTD *termotd;
    struct OhciTD *predotd;
    ULONG actual;
    ULONG epcaps;
    ULONG ctrl;
    ULONG len;
    ULONG phyaddr;
    ULONG oldenables;

    /* *** CTRL Transfers *** */
    KPRINTF(1, ("Scheduling new CTRL transfers...\n"));
    ioreq = (struct IOUsbHWReq *) hc->hc_CtrlXFerQueue.lh_Head;
    while(((struct Node *) ioreq)->ln_Succ)
    {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint;
        KPRINTF(10, ("New CTRL transfer to %ld.%ld: %ld bytes\n", ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length));
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep])
        {
            KPRINTF(5, ("Endpoint %02lx in use!\n", devadrep));
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }

        oed = ohciAllocED(hc);
        if(!oed)
        {
            break;
        }

        setupotd = ohciAllocTD(hc);
        if(!setupotd)
        {
            ohciFreeED(hc, oed);
            break;
        }
        termotd = ohciAllocTD(hc);
        if(!termotd)
        {
            ohciFreeTD(hc, setupotd);
            ohciFreeED(hc, oed);
            break;
        }
        oed->oed_IOReq = ioreq;

        KPRINTF(1, ("SetupTD=%08lx, TermTD=%08lx\n", setupotd, termotd));

        // fill setup td
        epcaps = (ioreq->iouh_DevAddr<<OECS_DEVADDR)|(ioreq->iouh_Endpoint<<OECS_ENDPOINT)|(ioreq->iouh_MaxPktSize<<OECS_MAXPKTLEN)|OECF_DIRECTION_TD;

        if(ioreq->iouh_Flags & UHFF_LOWSPEED)
        {
            KPRINTF(5, ("*** LOW SPEED ***\n"));
            epcaps |= OECF_LOWSPEED;
        }

        WRITEMEM32_LE(&oed->oed_EPCaps, epcaps);

        oed->oed_TailPtr = hc->hc_OhciTermTD->otd_Self;
        oed->oed_HeadPtr = setupotd->otd_Self;
        oed->oed_FirstTD = setupotd;

        setupotd->otd_ED = oed;
        setupotd->otd_Length = 0; // don't increase io_Actual for that transfer
        CONSTWRITEMEM32_LE(&setupotd->otd_Ctrl, OTCF_PIDCODE_SETUP|OTCF_CC_INVALID|OTCF_NOINT);
        WRITEMEM32_LE(&setupotd->otd_BufferPtr, (ULONG) pciGetPhysical(hc, &ioreq->iouh_SetupData));
        WRITEMEM32_LE(&setupotd->otd_BufferEnd, (ULONG) pciGetPhysical(hc, ((UBYTE *) (&ioreq->iouh_SetupData)) + 7));

        ctrl = (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? (OTCF_PIDCODE_IN|OTCF_CC_INVALID|OTCF_NOINT) : (OTCF_PIDCODE_OUT|OTCF_CC_INVALID|OTCF_NOINT);

        predotd = setupotd;
        if(ioreq->iouh_Length)
        {
            phyaddr = (ULONG) pciGetPhysical(hc, ioreq->iouh_Data);
            actual = 0;
            do
            {
                dataotd = ohciAllocTD(hc);
                if(!dataotd)
                {
                    predotd->otd_Succ = NULL;
                    break;
                }
                dataotd->otd_ED = oed;
                predotd->otd_Succ = dataotd;
                predotd->otd_NextTD = dataotd->otd_Self;
                len = ioreq->iouh_Length - actual;
                if(len > OHCI_PAGE_SIZE)
                {
                    len = OHCI_PAGE_SIZE;
                }
                dataotd->otd_Length = len;
                KPRINTF(1, ("TD with %ld bytes\n", len));
                WRITEMEM32_LE(&dataotd->otd_Ctrl, ctrl);
                WRITEMEM32_LE(&dataotd->otd_BufferPtr, phyaddr);
                phyaddr += len - 1;
                WRITEMEM32_LE(&dataotd->otd_BufferEnd, phyaddr);
                phyaddr++;
                actual += len;
                predotd = dataotd;
            } while(actual < ioreq->iouh_Length);

            if(actual != ioreq->iouh_Length)
            {
                // out of TDs
                KPRINTF(200, ("Out of TDs for Ctrl Transfer!\n"));
                dataotd = setupotd->otd_Succ;
                ohciFreeTD(hc, setupotd);
                while(dataotd)
                {
                    predotd = dataotd;
                    dataotd = dataotd->otd_Succ;
                    ohciFreeTD(hc, predotd);
                }
                ohciFreeTD(hc, termotd);
                ohciFreeED(hc, oed);
                break;
            }
            predotd->otd_Succ = termotd;
            predotd->otd_NextTD = termotd->otd_Self;
        } else {
            setupotd->otd_Succ = termotd;
            setupotd->otd_NextTD = termotd->otd_Self;
        }

        ctrl ^= (OTCF_PIDCODE_IN^OTCF_PIDCODE_OUT)|OTCF_NOINT|OTCF_DATA1|OTCF_TOGGLEFROMTD;

        termotd->otd_Length = 0;
        termotd->otd_ED = oed;
        termotd->otd_Succ = NULL;
        termotd->otd_NextTD = hc->hc_OhciTermTD->otd_Self;
        CONSTWRITEMEM32_LE(&termotd->otd_Ctrl, ctrl);
        CONSTWRITEMEM32_LE(&termotd->otd_BufferPtr, 0);
        CONSTWRITEMEM32_LE(&termotd->otd_BufferEnd, 0);

        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        ioreq->iouh_DriverPrivate1 = oed;

        // manage endpoint going busy
        unit->hu_DevBusyReq[devadrep] = ioreq;
        unit->hu_NakTimeoutFrame[devadrep] = (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + ioreq->iouh_NakTimeout : 0;

        Disable();
        AddTail(&hc->hc_TDQueue, (struct Node *) ioreq);

        // looks good to me, now enqueue this entry
        oed->oed_Succ = hc->hc_OhciCtrlTailED;
        oed->oed_NextED = oed->oed_Succ->oed_Self;
        oed->oed_Pred = hc->hc_OhciCtrlTailED->oed_Pred;
        oed->oed_Pred->oed_Succ = oed;
        oed->oed_Pred->oed_NextED = oed->oed_Self;
        oed->oed_Succ->oed_Pred = oed;
        SYNC;
        EIEIO;

        KPRINTF(5, ("ED: EPCaps=%08lx, HeadPtr=%08lx, TailPtr=%08lx, NextED=%08lx\n",
                     READMEM32_LE(&oed->oed_EPCaps),
                     READMEM32_LE(&oed->oed_HeadPtr),
                     READMEM32_LE(&oed->oed_TailPtr),
                     READMEM32_LE(&oed->oed_NextED)));

        oldenables = READREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS);
        if(!(oldenables & OCSF_CTRLENABLE))
        {
            CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CTRL_ED, 0);
        }
        oldenables |= OCSF_CTRLENABLE;
        WRITEREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS, oldenables);
        SYNC;
        EIEIO;
        Enable();

        ioreq = (struct IOUsbHWReq *) hc->hc_CtrlXFerQueue.lh_Head;
    }
}
/* \\\ */

/* /// "ohciScheduleIntTDs()" */
void ohciScheduleIntTDs(struct PCIController *hc)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    UWORD devadrep;
    struct OhciED *intoed;
    struct OhciED *oed;
    struct OhciTD *otd;
    struct OhciTD *predotd;
    ULONG actual;
    ULONG epcaps;
    ULONG len;
    ULONG phyaddr;

    /* *** INT Transfers *** */
    KPRINTF(1, ("Scheduling new INT transfers...\n"));
    ioreq = (struct IOUsbHWReq *) hc->hc_IntXFerQueue.lh_Head;
    while(((struct Node *) ioreq)->ln_Succ)
    {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
        KPRINTF(10, ("New INT transfer to %ld.%ld: %ld bytes\n", ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length));
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep])
        {
            KPRINTF(5, ("Endpoint %02lx in use!\n", devadrep));
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }

        oed = ohciAllocED(hc);
        if(!oed)
        {
            break;
        }

        oed->oed_IOReq = ioreq;

        epcaps = (ioreq->iouh_DevAddr<<OECS_DEVADDR)|(ioreq->iouh_Endpoint<<OECS_ENDPOINT)|(ioreq->iouh_MaxPktSize<<OECS_MAXPKTLEN);
        epcaps |= (ioreq->iouh_Dir == UHDIR_IN) ? OECF_DIRECTION_IN : OECF_DIRECTION_OUT;

        if(ioreq->iouh_Flags & UHFF_LOWSPEED)
        {
            KPRINTF(5, ("*** LOW SPEED ***\n"));
            epcaps |= OECF_LOWSPEED;
        }

        WRITEMEM32_LE(&oed->oed_EPCaps, epcaps);
        oed->oed_TailPtr = hc->hc_OhciTermTD->otd_Self;

        predotd = NULL;
        phyaddr = (ULONG) pciGetPhysical(hc, ioreq->iouh_Data);
        actual = 0;
        do
        {
            otd = ohciAllocTD(hc);
            if(!otd)
            {
                predotd->otd_Succ = NULL;
                break;
            }
            otd->otd_ED = oed;
            if(predotd)
            {
                predotd->otd_Succ = otd;
                predotd->otd_NextTD = otd->otd_Self;
            } else {
                WRITEMEM32_LE(&oed->oed_HeadPtr, READMEM32_LE(otd->otd_Self)|(unit->hu_DevDataToggle[devadrep] ? OEHF_DATA1 : 0));
                oed->oed_FirstTD = otd;
            }
            len = ioreq->iouh_Length - actual;
            if(len > OHCI_PAGE_SIZE)
            {
                len = OHCI_PAGE_SIZE;
            }
            otd->otd_Length = len;
            KPRINTF(1, ("TD with %ld bytes\n", len));
            CONSTWRITEMEM32_LE(&otd->otd_Ctrl, OTCF_CC_INVALID|OTCF_NOINT);
            if(len)
            {
                WRITEMEM32_LE(&otd->otd_BufferPtr, phyaddr);
                phyaddr += len - 1;
                WRITEMEM32_LE(&otd->otd_BufferEnd, phyaddr);
                phyaddr++;
            } else {
                CONSTWRITEMEM32_LE(&otd->otd_BufferPtr, 0);
                CONSTWRITEMEM32_LE(&otd->otd_BufferEnd, 0);
            }
            actual += len;
            predotd = otd;
        } while(actual < ioreq->iouh_Length);

        if(actual != ioreq->iouh_Length)
        {
            // out of TDs
            KPRINTF(200, ("Out of TDs for Int Transfer!\n"));
            otd = oed->oed_FirstTD;
            while(otd)
            {
                predotd = otd;
                otd = otd->otd_Succ;
                ohciFreeTD(hc, predotd);
            }
            ohciFreeED(hc, oed);
            break;
        }
        predotd->otd_Succ = NULL;
        predotd->otd_NextTD = hc->hc_OhciTermTD->otd_Self;

        CONSTWRITEMEM32_LE(&predotd->otd_Ctrl, OTCF_CC_INVALID);

        if(ioreq->iouh_Interval >= 31)
        {
            intoed = hc->hc_OhciIntED[4]; // 32ms interval
        } else {
            UWORD cnt = 0;
            do
            {
                intoed = hc->hc_OhciIntED[cnt++];
            } while(ioreq->iouh_Interval > (1<<cnt));
        }

        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        ioreq->iouh_DriverPrivate1 = oed;

        // manage endpoint going busy
        unit->hu_DevBusyReq[devadrep] = ioreq;
        unit->hu_NakTimeoutFrame[devadrep] = (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + ioreq->iouh_NakTimeout : 0;

        Disable();
        AddTail(&hc->hc_TDQueue, (struct Node *) ioreq);

        // looks good to me, now enqueue this entry (behind Int head)
        oed->oed_Succ = intoed->oed_Succ;
        oed->oed_NextED = intoed->oed_Succ->oed_Self;
        oed->oed_Pred = intoed;
        intoed->oed_Succ = oed;
        intoed->oed_NextED = oed->oed_Self;
        oed->oed_Succ->oed_Pred = oed;
        SYNC;
        EIEIO;

        KPRINTF(5, ("ED: EPCaps=%08lx, HeadPtr=%08lx, TailPtr=%08lx, NextED=%08lx\n",
                     READMEM32_LE(&oed->oed_EPCaps),
                     READMEM32_LE(&oed->oed_HeadPtr),
                     READMEM32_LE(&oed->oed_TailPtr),
                     READMEM32_LE(&oed->oed_NextED)));
        Enable();

        ioreq = (struct IOUsbHWReq *) hc->hc_IntXFerQueue.lh_Head;
    }
}
/* \\\ */

/* /// "ohciScheduleBulkTDs()" */
void ohciScheduleBulkTDs(struct PCIController *hc)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    UWORD devadrep;
    struct OhciED *oed;
    struct OhciTD *otd;
    struct OhciTD *predotd;
    ULONG actual;
    ULONG epcaps;
    ULONG len;
    ULONG phyaddr;
    ULONG oldenables;

    /* *** BULK Transfers *** */
    KPRINTF(1, ("Scheduling new BULK transfers...\n"));
    ioreq = (struct IOUsbHWReq *) hc->hc_BulkXFerQueue.lh_Head;
    while(((struct Node *) ioreq)->ln_Succ)
    {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
        KPRINTF(10, ("New BULK transfer to %ld.%ld: %ld bytes\n", ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length));
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep])
        {
            KPRINTF(5, ("Endpoint %02lx in use!\n", devadrep));
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }

        oed = ohciAllocED(hc);
        if(!oed)
        {
            break;
        }

        oed->oed_IOReq = ioreq;

        epcaps = (ioreq->iouh_DevAddr<<OECS_DEVADDR)|(ioreq->iouh_Endpoint<<OECS_ENDPOINT)|(ioreq->iouh_MaxPktSize<<OECS_MAXPKTLEN);
        epcaps |= (ioreq->iouh_Dir == UHDIR_IN) ? OECF_DIRECTION_IN : OECF_DIRECTION_OUT;

        if(ioreq->iouh_Flags & UHFF_LOWSPEED)
        {
            KPRINTF(5, ("*** LOW SPEED ***\n"));
            epcaps |= OECF_LOWSPEED;
        }

        WRITEMEM32_LE(&oed->oed_EPCaps, epcaps);
        oed->oed_TailPtr = hc->hc_OhciTermTD->otd_Self;

        predotd = NULL;
        phyaddr = (ULONG) pciGetPhysical(hc, ioreq->iouh_Data);
        actual = 0;
        do
        {
            if((actual >= OHCI_TD_BULK_LIMIT) && (actual < ioreq->iouh_Length))
            {
                KPRINTF(10, ("Bulk too large, splitting...\n"));
                break;
            }
            otd = ohciAllocTD(hc);
            if(!otd)
            {
                predotd->otd_Succ = NULL;
                break;
            }
            otd->otd_ED = oed;
            if(predotd)
            {
                predotd->otd_Succ = otd;
                predotd->otd_NextTD = otd->otd_Self;
            } else {
                WRITEMEM32_LE(&oed->oed_HeadPtr, READMEM32_LE(otd->otd_Self)|(unit->hu_DevDataToggle[devadrep] ? OEHF_DATA1 : 0));
                oed->oed_FirstTD = otd;
            }
            len = ioreq->iouh_Length - actual;
            if(len > OHCI_PAGE_SIZE)
            {
                len = OHCI_PAGE_SIZE;
            }
            otd->otd_Length = len;
            KPRINTF(1, ("TD with %ld bytes\n", len));
            CONSTWRITEMEM32_LE(&otd->otd_Ctrl, OTCF_CC_INVALID|OTCF_NOINT);
            if(len)
            {
                WRITEMEM32_LE(&otd->otd_BufferPtr, phyaddr);
                phyaddr += len - 1;
                WRITEMEM32_LE(&otd->otd_BufferEnd, phyaddr);
                phyaddr++;
            } else {
                CONSTWRITEMEM32_LE(&otd->otd_BufferPtr, 0);
                CONSTWRITEMEM32_LE(&otd->otd_BufferEnd, 0);
            }
            actual += len;

            predotd = otd;
        } while((actual < ioreq->iouh_Length) || (len && (ioreq->iouh_Dir == UHDIR_OUT) && (actual == ioreq->iouh_Length) && (!ioreq->iouh_Flags & UHFF_NOSHORTPKT) && ((actual % ioreq->iouh_MaxPktSize) == 0)));

        if(!actual)
        {
            // out of TDs
            KPRINTF(200, ("Out of TDs for Bulk Transfer!\n"));
            otd = oed->oed_FirstTD;
            while(otd)
            {
                predotd = otd;
                otd = otd->otd_Succ;
                ohciFreeTD(hc, predotd);
            }
            ohciFreeED(hc, oed);
            break;
        }
        oed->oed_Continue = (actual < ioreq->iouh_Length);
        predotd->otd_Succ = NULL;
        predotd->otd_NextTD = hc->hc_OhciTermTD->otd_Self;

        CONSTWRITEMEM32_LE(&predotd->otd_Ctrl, OTCF_CC_INVALID);

        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        ioreq->iouh_DriverPrivate1 = oed;

        // manage endpoint going busy
        unit->hu_DevBusyReq[devadrep] = ioreq;
        unit->hu_NakTimeoutFrame[devadrep] = (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + ioreq->iouh_NakTimeout : 0;

        Disable();
        AddTail(&hc->hc_TDQueue, (struct Node *) ioreq);

        // looks good to me, now enqueue this entry
        oed->oed_Succ = hc->hc_OhciBulkTailED;
        oed->oed_NextED = oed->oed_Succ->oed_Self;
        oed->oed_Pred = hc->hc_OhciBulkTailED->oed_Pred;
        oed->oed_Pred->oed_Succ = oed;
        oed->oed_Pred->oed_NextED = oed->oed_Self;
        oed->oed_Succ->oed_Pred = oed;
        SYNC;
        EIEIO;

        KPRINTF(10, ("Activating BULK at %ld\n", READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT)));

        KPRINTF(5, ("ED: EPCaps=%08lx, HeadPtr=%08lx, TailPtr=%08lx, NextED=%08lx\n",
                     READMEM32_LE(&oed->oed_EPCaps),
                     READMEM32_LE(&oed->oed_HeadPtr),
                     READMEM32_LE(&oed->oed_TailPtr),
                     READMEM32_LE(&oed->oed_NextED)));

        oldenables = READREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS);
        if(!(oldenables & OCSF_BULKENABLE))
        {
            CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_BULK_ED, 0);
        }
        oldenables |= OCSF_BULKENABLE;
        WRITEREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS, oldenables);
        SYNC;
        EIEIO;
        Enable();
        ioreq = (struct IOUsbHWReq *) hc->hc_BulkXFerQueue.lh_Head;
    }
}
/* \\\ */

/* /// "ohciCompleteInt()" */
void ohciCompleteInt(struct PCIController *hc)
{
    ULONG framecnt = READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT);

    KPRINTF(1, ("CompleteInt!\n"));
    if(framecnt < (hc->hc_FrameCounter & 0xffff))
    {
        hc->hc_FrameCounter |= 0xffff;
        hc->hc_FrameCounter++;
        hc->hc_FrameCounter += framecnt;
        KPRINTF(10, ("Frame Counter Rollover %ld\n", hc->hc_FrameCounter));
    } else {
        hc->hc_FrameCounter = (hc->hc_FrameCounter & 0xffff0000)|framecnt;
    }

    /* **************** PROCESS DONE TRANSFERS **************** */

    if(hc->hc_OhciDoneQueue)
    {
        ohciHandleFinishedTDs(hc);
    }

    if(hc->hc_CtrlXFerQueue.lh_Head->ln_Succ)
    {
        ohciScheduleCtrlTDs(hc);
    }

    if(hc->hc_IntXFerQueue.lh_Head->ln_Succ)
    {
        ohciScheduleIntTDs(hc);
    }

    if(hc->hc_BulkXFerQueue.lh_Head->ln_Succ)
    {
        ohciScheduleBulkTDs(hc);
    }

    KPRINTF(1, ("CompleteDone\n"));
}
/* \\\ */

/* /// "ohciIntCode()" */
void ohciIntCode(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
    struct PCIController *hc = (struct PCIController *) irq->h_Data;
    struct PCIDevice *base = hc->hc_Device;
    struct PCIUnit *unit = hc->hc_Unit;
    ULONG intr = 0;
    ULONG donehead = READMEM32_LE(&hc->hc_OhciHCCA->oha_DoneHead);

    if(donehead)
    {
        intr = OISF_DONEHEAD;
        if(donehead & 1)
        {
            intr |= READREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS);
        }
        donehead &= OHCI_PTRMASK;
        KPRINTF(5, ("New Donehead %08lx for old %08lx\n", donehead, hc->hc_OhciDoneQueue));
        if(hc->hc_OhciDoneQueue)
        {
            struct OhciTD *donetd = (struct OhciTD *) (donehead - hc->hc_PCIVirtualAdjust - 16);
            while(donetd->otd_NextTD)
            {
				donetd = (struct OhciTD *) (donetd->otd_NextTD - hc->hc_PCIVirtualAdjust - 16);
			}
            WRITEMEM32_LE(&donetd->otd_NextTD, hc->hc_OhciDoneQueue);
        }
        hc->hc_OhciDoneQueue = donehead;
        CONSTWRITEMEM32_LE(&hc->hc_OhciHCCA->oha_DoneHead, 0);
    } else {
        intr = READREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS);
    }
    if(intr & hc->hc_PCIIntEnMask)
    {
        WRITEREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS, intr);
        KPRINTF(1, ("INT=%02lx\n", intr));
        if(intr & OISF_HOSTERROR)
        {
            KPRINTF(200, ("Host ERROR!\n"));
        }
        if(intr & OISF_SCHEDOVERRUN)
        {
            KPRINTF(200, ("Schedule overrun!\n"));
        }
        if(!hc->hc_Online)
        {
            return;
        }
        if(intr & OISF_FRAMECOUNTOVER)
        {
            ULONG framecnt = READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT);
            hc->hc_FrameCounter |= 0x7fff;
            hc->hc_FrameCounter++;
            hc->hc_FrameCounter |= framecnt;
            KPRINTF(10, ("Frame Counter Rollover %ld\n", hc->hc_FrameCounter));
        }
        if(intr & OISF_HUBCHANGE)
        {
            UWORD hciport;
            ULONG oldval;
            UWORD portreg = OHCI_PORTSTATUS;
            for(hciport = 0; hciport < hc->hc_NumPorts; hciport++, portreg += 4)
            {
                oldval = READREG32_LE(hc->hc_RegBase, portreg);
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
                KPRINTF(20, ("PCI Int Port %ld (glob %ld) Change %08lx\n", hciport, hc->hc_PortNum20[hciport] + 1, oldval));
                if(hc->hc_PortChangeMap[hciport])
                {
                    unit->hu_RootPortChanges |= 1UL<<(hc->hc_PortNum20[hciport] + 1);
                }
            }
            uhwCheckRootHubChanges(unit);
        }
        if(intr & OISF_DONEHEAD)
        {
            KPRINTF(10, ("DoneHead %ld\n", READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT)));
            SureCause(base, &hc->hc_CompleteInt);
        }
    }
}
/* \\\ */

/* ---------------------------------------------------------------------- *
 *                    EHCI Specific Stuff                                 *
 * ---------------------------------------------------------------------- */

/* /// "ehciFreeAsyncContext()" */
void ehciFreeAsyncContext(struct PCIController *hc, struct EhciQH *eqh)
{
    KPRINTF(5, ("Unlinking AsyncContext %08lx\n", eqh));
    // unlink from schedule
    eqh->eqh_Pred->eqh_NextQH = eqh->eqh_Succ->eqh_Self;
    SYNC;
    EIEIO;
    eqh->eqh_Succ->eqh_Pred = eqh->eqh_Pred;
    eqh->eqh_Pred->eqh_Succ = eqh->eqh_Succ;
    SYNC;
    EIEIO;

    // need to wait until an async schedule rollover before freeing these
    Disable();
    eqh->eqh_Succ = hc->hc_EhciAsyncFreeQH;
    hc->hc_EhciAsyncFreeQH = eqh;
    // activate doorbell
    WRITEREG32_LE(hc->hc_RegBase, EHCI_USBCMD, hc->hc_EhciUsbCmd|EHUF_ASYNCDOORBELL);
    Enable();
}
/* \\\ */

/* /// "ehciFreePeriodicContext()" */
void ehciFreePeriodicContext(struct PCIController *hc, struct EhciQH *eqh)
{
    struct EhciTD *etd;
    struct EhciTD *nextetd;

    KPRINTF(5, ("Unlinking PeriodicContext %08lx\n", eqh));
    // unlink from schedule
    eqh->eqh_Pred->eqh_NextQH = eqh->eqh_Succ->eqh_Self;
    SYNC;
    EIEIO;
    eqh->eqh_Succ->eqh_Pred = eqh->eqh_Pred;
    eqh->eqh_Pred->eqh_Succ = eqh->eqh_Succ;
    SYNC;
    EIEIO;

    Disable(); // avoid race condition with interrupt
    nextetd = eqh->eqh_FirstTD;
    while((etd = nextetd))
    {
        KPRINTF(1, ("FreeTD %08lx\n", nextetd));
        nextetd = etd->etd_Succ;
        ehciFreeTD(hc, etd);
    }
    ehciFreeQH(hc, eqh);
    Enable();
}
/* \\\ */

/* /// "ehciFreeQHandTDs()" */
void ehciFreeQHandTDs(struct PCIController *hc, struct EhciQH *eqh)
{
    struct EhciTD *etd = NULL;
    struct EhciTD *nextetd;

    KPRINTF(5, ("Unlinking QContext %08lx\n", eqh));
    nextetd = eqh->eqh_FirstTD;
    while(nextetd)
    {
        KPRINTF(1, ("FreeTD %08lx\n", nextetd));
        etd = nextetd;
        nextetd = (struct EhciTD *) etd->etd_Succ;
        ehciFreeTD(hc, etd);
    }

    ehciFreeQH(hc, eqh);
}
/* \\\ */

/* /// "ehciAllocQH()" */
inline struct EhciQH * ehciAllocQH(struct PCIController *hc)
{
    struct EhciQH *eqh = hc->hc_EhciQHPool;

    if(!eqh)
    {
        // out of QHs!
        KPRINTF(20, ("Out of QHs!\n"));
        return NULL;
    }

    hc->hc_EhciQHPool = (struct EhciQH *) eqh->eqh_Succ;
    return(eqh);
}
/* \\\ */

/* /// "ehciFreeQH()" */
inline void ehciFreeQH(struct PCIController *hc, struct EhciQH *eqh)
{
    eqh->eqh_Succ = hc->hc_EhciQHPool;
    hc->hc_EhciQHPool = eqh;
}
/* \\\ */

/* /// "ehciAllocTD()" */
inline struct EhciTD * ehciAllocTD(struct PCIController *hc)
{
    struct EhciTD *etd = hc->hc_EhciTDPool;

    if(!etd)
    {
        // out of TDs!
        KPRINTF(20, ("Out of TDs!\n"));
        return NULL;
    }

    hc->hc_EhciTDPool = (struct EhciTD *) etd->etd_Succ;
    return(etd);
}
/* \\\ */

/* /// "ehciFreeTD()" */
inline void ehciFreeTD(struct PCIController *hc, struct EhciTD *etd)
{
    etd->etd_Succ = hc->hc_EhciTDPool;
    hc->hc_EhciTDPool = etd;
}
/* \\\ */

/* /// "ehciUpdateIntTree()" */
void ehciUpdateIntTree(struct PCIController *hc)
{
    struct EhciQH *eqh;
    struct EhciQH *predeqh;
    struct EhciQH *lastusedeqh;
    UWORD cnt;

    // optimize linkage between queue heads
    predeqh = lastusedeqh = hc->hc_EhciTermQH;
    for(cnt = 0; cnt < 11; cnt++)
    {
        eqh = hc->hc_EhciIntQH[cnt];
        if(eqh->eqh_Succ != predeqh)
        {
            lastusedeqh = eqh->eqh_Succ;
        }
        eqh->eqh_NextQH = lastusedeqh->eqh_Self;
        predeqh = eqh;
    }
}
/* \\\ */

/* /// "ehciHandleFinishedTDs()" */
void ehciHandleFinishedTDs(struct PCIController *hc)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    struct IOUsbHWReq *nextioreq;
    struct EhciQH *eqh;
    struct EhciTD *etd;
    struct EhciTD *predetd;
    UWORD devadrep;
    ULONG len;
    UWORD inspect;
    ULONG nexttd;
    BOOL shortpkt;
    ULONG ctrlstatus;
    ULONG epctrlstatus;
    ULONG actual;
    BOOL halted;
    BOOL updatetree = FALSE;
    BOOL zeroterm;
    ULONG phyaddr;

    KPRINTF(1, ("Checking for Async work done...\n"));
    ioreq = (struct IOUsbHWReq *) hc->hc_TDQueue.lh_Head;
    while((nextioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ))
    {
        eqh = (struct EhciQH *) ioreq->iouh_DriverPrivate1;
        if(eqh)
        {
            KPRINTF(1, ("Examining IOReq=%08lx with EQH=%08lx\n", ioreq, eqh));
            SYNC;
            EIEIO;
            epctrlstatus = READMEM32_LE(&eqh->eqh_CtrlStatus);
            nexttd = READMEM32_LE(&eqh->eqh_NextTD);
            devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
            halted = ((epctrlstatus & (ETCF_ACTIVE|ETSF_HALTED)) == ETSF_HALTED);
            if(halted || (!(epctrlstatus & ETCF_ACTIVE) && (nexttd & EHCI_TERMINATE)))
            {
                KPRINTF(1, ("AS: CS=%08lx CP=%08lx NX=%08lx\n", epctrlstatus, READMEM32_LE(&eqh->eqh_CurrTD), nexttd));
                shortpkt = FALSE;
                actual = 0;
                inspect = 1;
                etd = eqh->eqh_FirstTD;
                do
                {
                    ctrlstatus = READMEM32_LE(&etd->etd_CtrlStatus);
                    KPRINTF(1, ("AS: CS=%08lx SL=%08lx TD=%08lx\n", ctrlstatus, READMEM32_LE(&etd->etd_Self), etd));
                    if(ctrlstatus & ETCF_ACTIVE)
                    {
                        if(halted)
                        {
                            KPRINTF(20, ("Async: Halted before TD\n"));
                            //ctrlstatus = eqh->eqh_CtrlStatus;
                            inspect = 0;
                            if(unit->hu_NakTimeoutFrame[devadrep] && (hc->hc_FrameCounter > unit->hu_NakTimeoutFrame[devadrep]))
                            {
                                KPRINTF(20, ("NAK timeout\n"));
                                ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                            }
                            break;
                        } else {
                            // what happened here? The host controller was just updating the fields and has not finished yet
                            ctrlstatus = epctrlstatus;

                            /*KPRINTF(20, ("AS: CS=%08lx CP=%08lx NX=%08lx\n", epctrlstatus, READMEM32_LE(&eqh->eqh_CurrTD), nexttd));
                            KPRINTF(20, ("AS: CS=%08lx CP=%08lx NX=%08lx\n", READMEM32_LE(&eqh->eqh_CtrlStatus), READMEM32_LE(&eqh->eqh_CurrTD), READMEM32_LE(&eqh->eqh_NextTD)));
                            KPRINTF(20, ("AS: CS=%08lx SL=%08lx TD=%08lx\n", ctrlstatus, READMEM32_LE(&etd->etd_Self), etd));
                            etd = eqh->eqh_FirstTD;
                            do
                            {
                                KPRINTF(20, ("XX: CS=%08lx SL=%08lx TD=%08lx\n", READMEM32_LE(&etd->etd_CtrlStatus), READMEM32_LE(&etd->etd_Self), etd));
                            } while(etd = etd->etd_Succ);
                            KPRINTF(20, ("Async: Internal error! Still active?!\n"));
                            inspect = 2;
                            break;*/
                        }
                    }

                    if(ctrlstatus & (ETSF_HALTED|ETSF_TRANSERR|ETSF_BABBLE|ETSF_DATABUFFERERR))
                    {
                        if(ctrlstatus & ETSF_BABBLE)
                        {
                            KPRINTF(20, ("Babble error %08lx\n", ctrlstatus));
                            ioreq->iouh_Req.io_Error = UHIOERR_OVERFLOW;
                        }
                        else if(ctrlstatus & ETSF_DATABUFFERERR)
                        {
                            KPRINTF(20, ("Databuffer error\n"));
                            ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                        }
                        else if(ctrlstatus & ETSF_TRANSERR)
                        {
                            if((ctrlstatus & ETCM_ERRORLIMIT)>>ETCS_ERRORLIMIT)
                            {
                                KPRINTF(20, ("STALLED!\n"));
                                ioreq->iouh_Req.io_Error = UHIOERR_STALL;
                            } else {
                                KPRINTF(20, ("TIMEOUT!\n"));
                                ioreq->iouh_Req.io_Error = UHIOERR_TIMEOUT;
                            }
                        }
                        inspect = 0;
                        break;
                    }

                    len = etd->etd_Length - ((ctrlstatus & ETSM_TRANSLENGTH)>>ETSS_TRANSLENGTH);
                    if((ctrlstatus & ETCM_PIDCODE) != ETCF_PIDCODE_SETUP) // don't count setup packet
                    {
                        actual += len;
                    }
                    if(ctrlstatus & ETSM_TRANSLENGTH)
                    {
                        KPRINTF(10, ("Short packet: %ld < %ld\n", len, etd->etd_Length));
                        shortpkt = TRUE;
                        break;
                    }
                    etd = etd->etd_Succ;
                } while(etd && (!(ctrlstatus & ETCF_READYINTEN)));
                /*if(inspect == 2)
                {
                    // phantom halted
                    ioreq = nextioreq;
                    continue;
                }*/

                if(((actual + ioreq->iouh_Actual) < eqh->eqh_Actual) && (!ioreq->iouh_Req.io_Error) && (!(ioreq->iouh_Flags & UHFF_ALLOWRUNTPKTS)))
                {
                    ioreq->iouh_Req.io_Error = UHIOERR_RUNTPACKET;
                }
                ioreq->iouh_Actual += actual;
                if(inspect && (!shortpkt) && (eqh->eqh_Actual < ioreq->iouh_Length))
                {
                    KPRINTF(10, ("Reloading BULK at %ld/%ld\n", eqh->eqh_Actual, ioreq->iouh_Length));
                    // reload
                    ctrlstatus = (ioreq->iouh_Dir == UHDIR_IN) ? (ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_IN) : (ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_OUT);
                    phyaddr = (ULONG) pciGetPhysical(hc, &(((UBYTE *) ioreq->iouh_Data)[ioreq->iouh_Actual]));
                    predetd = etd = eqh->eqh_FirstTD;

                    CONSTWRITEMEM32_LE(&eqh->eqh_CurrTD, EHCI_TERMINATE);
                    CONSTWRITEMEM32_LE(&eqh->eqh_NextTD, EHCI_TERMINATE);
                    CONSTWRITEMEM32_LE(&eqh->eqh_AltNextTD, EHCI_TERMINATE);
                    do
                    {
                        len = ioreq->iouh_Length - eqh->eqh_Actual;
                        if(len > 4*EHCI_PAGE_SIZE)
                        {
                            len = 4*EHCI_PAGE_SIZE;
                        }
                        etd->etd_Length = len;
                        KPRINTF(1, ("Reload Bulk TD %08lx len %ld (%ld/%ld) phy=%08lx\n",
                                    etd, len, eqh->eqh_Actual, ioreq->iouh_Length, phyaddr));
                        WRITEMEM32_LE(&etd->etd_CtrlStatus, ctrlstatus|(len<<ETSS_TRANSLENGTH));
                        // FIXME need quark scatter gather mechanism here
                        WRITEMEM32_LE(&etd->etd_BufferPtr[0], phyaddr);
                        WRITEMEM32_LE(&etd->etd_BufferPtr[1], (phyaddr & EHCI_PAGE_MASK) + (1*EHCI_PAGE_SIZE));
                        WRITEMEM32_LE(&etd->etd_BufferPtr[2], (phyaddr & EHCI_PAGE_MASK) + (2*EHCI_PAGE_SIZE));
                        WRITEMEM32_LE(&etd->etd_BufferPtr[3], (phyaddr & EHCI_PAGE_MASK) + (3*EHCI_PAGE_SIZE));
                        WRITEMEM32_LE(&etd->etd_BufferPtr[4], (phyaddr & EHCI_PAGE_MASK) + (4*EHCI_PAGE_SIZE));
                        phyaddr += len;
                        eqh->eqh_Actual += len;
                        zeroterm = (len && (ioreq->iouh_Dir == UHDIR_OUT) && (eqh->eqh_Actual == ioreq->iouh_Length) && (!ioreq->iouh_Flags & UHFF_NOSHORTPKT) && ((eqh->eqh_Actual % ioreq->iouh_MaxPktSize) == 0));
                        predetd = etd;
                        etd = etd->etd_Succ;
                        if((!etd) && zeroterm)
                        {
                            // rare case where the zero packet would be lost, allocate etd and append zero packet.
                            etd = ehciAllocTD(hc);
                            if(!etd)
                            {
                                KPRINTF(200, ("INTERNAL ERROR! This should not happen! Could not allocate zero packet TD\n"));
                                break;
                            }
                            predetd->etd_Succ = etd;
                            predetd->etd_NextTD = etd->etd_Self;
                            predetd->etd_AltNextTD = hc->hc_ShortPktEndTD->etd_Self;
                            etd->etd_Succ = NULL;
                            CONSTWRITEMEM32_LE(&etd->etd_NextTD, EHCI_TERMINATE);
                            CONSTWRITEMEM32_LE(&etd->etd_AltNextTD, EHCI_TERMINATE);
                        }
                    } while(etd && ((eqh->eqh_Actual < ioreq->iouh_Length) || zeroterm));
                    ctrlstatus |= ETCF_READYINTEN|(predetd->etd_Length<<ETSS_TRANSLENGTH);
                    WRITEMEM32_LE(&predetd->etd_CtrlStatus, ctrlstatus);
                    CONSTWRITEMEM32_LE(&predetd->etd_NextTD, EHCI_TERMINATE);
                    CONSTWRITEMEM32_LE(&predetd->etd_AltNextTD, EHCI_TERMINATE);
                    SYNC;
                    EIEIO;
                    etd = eqh->eqh_FirstTD;
                    eqh->eqh_NextTD = etd->etd_Self;
                    SYNC;
                    EIEIO;
                    unit->hu_NakTimeoutFrame[devadrep] = (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + (ioreq->iouh_NakTimeout<<3) : 0;
                } else {
                    unit->hu_DevBusyReq[devadrep] = NULL;
                    Remove(&ioreq->iouh_Req.io_Message.mn_Node);
                    ehciFreeAsyncContext(hc, eqh);
                    // use next data toggle bit based on last successful transaction
                    KPRINTF(1, ("Old Toggle %04lx:%ld\n", devadrep, unit->hu_DevDataToggle[devadrep]));
                    unit->hu_DevDataToggle[devadrep] = (ctrlstatus & ETCF_DATA1) ? TRUE : FALSE;
                    KPRINTF(1, ("Toggle now %04lx:%ld\n", devadrep, unit->hu_DevDataToggle[devadrep]));
                    if(inspect)
                    {
                        if(ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER)
                        {
                            // check for sucessful clear feature and set address ctrl transfers
                            uhwCheckSpecialCtrlTransfers(hc, ioreq);
                        }
                    }
                    ReplyMsg(&ioreq->iouh_Req.io_Message);
                }
            }
        } else {
            KPRINTF(20, ("IOReq=%08lx has no UQH!\n", ioreq));
        }
        ioreq = nextioreq;
    }

    KPRINTF(1, ("Checking for Periodic work done...\n"));
    ioreq = (struct IOUsbHWReq *) hc->hc_PeriodicTDQueue.lh_Head;
    while((nextioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ))
    {
        eqh = (struct EhciQH *) ioreq->iouh_DriverPrivate1;
        if(eqh)
        {
            KPRINTF(1, ("Examining IOReq=%08lx with EQH=%08lx\n", ioreq, eqh));
            nexttd = READMEM32_LE(&eqh->eqh_NextTD);
            etd = eqh->eqh_FirstTD;
            ctrlstatus = READMEM32_LE(&eqh->eqh_CtrlStatus);
            devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
            halted = ((ctrlstatus & (ETCF_ACTIVE|ETSF_HALTED)) == ETSF_HALTED);
            if(halted || (!(ctrlstatus & ETCF_ACTIVE) && (nexttd & EHCI_TERMINATE)))
            {
                KPRINTF(1, ("EQH not active %08lx\n", ctrlstatus));
                shortpkt = FALSE;
                actual = 0;
                inspect = 1;
                do
                {
                    ctrlstatus = READMEM32_LE(&etd->etd_CtrlStatus);
                    KPRINTF(1, ("Periodic: TD=%08lx CS=%08lx\n", etd, ctrlstatus));
                    if(ctrlstatus & ETCF_ACTIVE)
                    {
                        if(halted)
                        {
                            KPRINTF(20, ("Periodic: Halted before TD\n"));
                            //ctrlstatus = eqh->eqh_CtrlStatus;
                            inspect = 0;
                            if(unit->hu_NakTimeoutFrame[devadrep] && (hc->hc_FrameCounter > unit->hu_NakTimeoutFrame[devadrep]))
                            {
                                KPRINTF(20, ("NAK timeout\n"));
                                ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                            }
                            break;
                        } else {
                            KPRINTF(20, ("Periodic: Internal error! Still active?!\n"));
                            break;
                        }
                    }

                    if(ctrlstatus & (ETSF_HALTED|ETSF_TRANSERR|ETSF_BABBLE|ETSF_DATABUFFERERR|ETSF_MISSEDCSPLIT))
                    {
                        if(ctrlstatus & ETSF_BABBLE)
                        {
                            KPRINTF(20, ("Babble error %08lx\n", ctrlstatus));
                            ioreq->iouh_Req.io_Error = UHIOERR_OVERFLOW;
                        }
                        else if(ctrlstatus & ETSF_MISSEDCSPLIT)
                        {
                            KPRINTF(20, ("Missed CSplit %08lx\n", ctrlstatus));
                            ioreq->iouh_Req.io_Error = UHIOERR_STALL;
                        }
                        else if(ctrlstatus & ETSF_DATABUFFERERR)
                        {
                            KPRINTF(20, ("Databuffer error\n"));
                            ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                        }
                        else if(ctrlstatus & ETSF_TRANSERR)
                        {
                            if((ctrlstatus & ETCM_ERRORLIMIT)>>ETCS_ERRORLIMIT)
                            {
                                KPRINTF(20, ("STALLED!\n"));
                                ioreq->iouh_Req.io_Error = UHIOERR_STALL;
                            } else {
                                KPRINTF(20, ("TIMEOUT!\n"));
                                ioreq->iouh_Req.io_Error = UHIOERR_TIMEOUT;
                            }
                        }
                        else if(unit->hu_NakTimeoutFrame[devadrep] && (hc->hc_FrameCounter > unit->hu_NakTimeoutFrame[devadrep]))
                        {
                            ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                        }
                        inspect = 0;
                        break;
                    }

                    len = etd->etd_Length - ((ctrlstatus & ETSM_TRANSLENGTH)>>ETSS_TRANSLENGTH);
                    actual += len;
                    if(ctrlstatus & ETSM_TRANSLENGTH)
                    {
                        KPRINTF(10, ("Short packet: %ld < %ld\n", len, etd->etd_Length));
                        shortpkt = TRUE;
                        break;
                    }
                    etd = etd->etd_Succ;
                } while(etd);
                if((actual < eqh->eqh_Actual) && (!ioreq->iouh_Req.io_Error) && (!(ioreq->iouh_Flags & UHFF_ALLOWRUNTPKTS)))
                {
                    ioreq->iouh_Req.io_Error = UHIOERR_RUNTPACKET;
                }
                ioreq->iouh_Actual += actual;
                unit->hu_DevBusyReq[devadrep] = NULL;
                Remove(&ioreq->iouh_Req.io_Message.mn_Node);
                ehciFreePeriodicContext(hc, eqh);
                updatetree = TRUE;
                // use next data toggle bit based on last successful transaction
                KPRINTF(1, ("Old Toggle %04lx:%ld\n", devadrep, unit->hu_DevDataToggle[devadrep]));
                unit->hu_DevDataToggle[devadrep] = (ctrlstatus & ETCF_DATA1) ? TRUE : FALSE;
                KPRINTF(1, ("Toggle now %04lx:%ld\n", devadrep, unit->hu_DevDataToggle[devadrep]));
                ReplyMsg(&ioreq->iouh_Req.io_Message);
            }
        } else {
            KPRINTF(20, ("IOReq=%08lx has no UQH!\n", ioreq));
        }
        ioreq = nextioreq;
    }
    if(updatetree)
    {
        ehciUpdateIntTree(hc);
    }
}
/* \\\ */

/* /// "ehciScheduleCtrlTDs()" */
void ehciScheduleCtrlTDs(struct PCIController *hc)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    UWORD devadrep;
    struct EhciQH *eqh;
    struct EhciTD *setupetd;
    struct EhciTD *dataetd;
    struct EhciTD *termetd;
    struct EhciTD *predetd;
    ULONG epcaps;
    ULONG ctrlstatus;
    ULONG len;
    ULONG phyaddr;

    /* *** CTRL Transfers *** */
    KPRINTF(1, ("Scheduling new CTRL transfers...\n"));
    ioreq = (struct IOUsbHWReq *) hc->hc_CtrlXFerQueue.lh_Head;
    while(((struct Node *) ioreq)->ln_Succ)
    {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint;
        KPRINTF(10, ("New CTRL transfer to %ld.%ld: %ld bytes\n", ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length));
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep])
        {
            KPRINTF(5, ("Endpoint %02lx in use!\n", devadrep));
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }

        eqh = ehciAllocQH(hc);
        if(!eqh)
        {
            break;
        }

        setupetd = ehciAllocTD(hc);
        if(!setupetd)
        {
            ehciFreeQH(hc, eqh);
            break;
        }
        termetd = ehciAllocTD(hc);
        if(!termetd)
        {
            ehciFreeTD(hc, setupetd);
            ehciFreeQH(hc, eqh);
            break;
        }
        eqh->eqh_IOReq = ioreq;
        eqh->eqh_FirstTD = setupetd;
        eqh->eqh_Actual = 0;

        epcaps = ((0<<EQES_RELOAD)|EQEF_TOGGLEFROMTD)|(ioreq->iouh_MaxPktSize<<EQES_MAXPKTLEN)|(ioreq->iouh_DevAddr<<EQES_DEVADDR)|(ioreq->iouh_Endpoint<<EQES_ENDPOINT);
        if(ioreq->iouh_Flags & UHFF_SPLITTRANS)
        {
            KPRINTF(10, ("*** SPLIT TRANSACTION to HubPort %ld at Addr %ld\n", ioreq->iouh_SplitHubPort, ioreq->iouh_SplitHubAddr));
            // full speed and low speed handling
            WRITEMEM32_LE(&eqh->eqh_SplitCtrl, EQSF_MULTI_1|(ioreq->iouh_SplitHubPort<<EQSS_PORTNUMBER)|(ioreq->iouh_SplitHubAddr<<EQSS_HUBADDRESS));
            epcaps |= EQEF_SPLITCTRLEP;
            if(ioreq->iouh_Flags & UHFF_LOWSPEED)
            {
                KPRINTF(10, ("*** LOW SPEED ***\n"));
                epcaps |= EQEF_LOWSPEED;
            }
        } else {
            CONSTWRITEMEM32_LE(&eqh->eqh_SplitCtrl, EQSF_MULTI_1);
            epcaps |= EQEF_HIGHSPEED;
        }
        WRITEMEM32_LE(&eqh->eqh_EPCaps, epcaps);
        //eqh->eqh_CtrlStatus = eqh->eqh_CurrTD = 0;
        //eqh->eqh_AltNextTD = eqh->eqh_NextTD = setupetd->etd_Self;

        //termetd->etd_QueueHead = setupetd->etd_QueueHead = eqh;

        KPRINTF(1, ("SetupTD=%08lx, TermTD=%08lx\n", setupetd, termetd));

        // fill setup td
        setupetd->etd_Length = 8;

        CONSTWRITEMEM32_LE(&setupetd->etd_CtrlStatus, (8<<ETSS_TRANSLENGTH)|ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_SETUP);
        phyaddr = (ULONG) pciGetPhysical(hc, &ioreq->iouh_SetupData);
        WRITEMEM32_LE(&setupetd->etd_BufferPtr[0], phyaddr);
        WRITEMEM32_LE(&setupetd->etd_BufferPtr[1], (phyaddr + 8) & EHCI_PAGE_MASK); // theoretically, setup data may cross one page
        setupetd->etd_BufferPtr[2] = 0; // clear for overlay bits

        ctrlstatus = (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? (ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_IN) : (ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_OUT);
        predetd = setupetd;
        if(ioreq->iouh_Length)
        {
            phyaddr = (ULONG) pciGetPhysical(hc, ioreq->iouh_Data);
            do
            {
                dataetd = ehciAllocTD(hc);
                if(!dataetd)
                {
                    break;
                }
                ctrlstatus ^= ETCF_DATA1; // toggle bit
                predetd->etd_Succ = dataetd;
                predetd->etd_NextTD = dataetd->etd_Self;
                dataetd->etd_AltNextTD = termetd->etd_Self;

                len = ioreq->iouh_Length - eqh->eqh_Actual;
                if(len > 4*EHCI_PAGE_SIZE)
                {
                    len = 4*EHCI_PAGE_SIZE;
                }
                dataetd->etd_Length = len;
                WRITEMEM32_LE(&dataetd->etd_CtrlStatus, ctrlstatus|(len<<ETSS_TRANSLENGTH));
                // FIXME need quark scatter gather mechanism here
                WRITEMEM32_LE(&dataetd->etd_BufferPtr[0], phyaddr);
                WRITEMEM32_LE(&dataetd->etd_BufferPtr[1], (phyaddr & EHCI_PAGE_MASK) + (1*EHCI_PAGE_SIZE));
                WRITEMEM32_LE(&dataetd->etd_BufferPtr[2], (phyaddr & EHCI_PAGE_MASK) + (2*EHCI_PAGE_SIZE));
                WRITEMEM32_LE(&dataetd->etd_BufferPtr[3], (phyaddr & EHCI_PAGE_MASK) + (3*EHCI_PAGE_SIZE));
                WRITEMEM32_LE(&dataetd->etd_BufferPtr[4], (phyaddr & EHCI_PAGE_MASK) + (4*EHCI_PAGE_SIZE));
                phyaddr += len;
                eqh->eqh_Actual += len;
                predetd = dataetd;
            } while(eqh->eqh_Actual < ioreq->iouh_Length);
            if(!dataetd)
            {
                // not enough dataetds? try again later
                ehciFreeQHandTDs(hc, eqh);
                ehciFreeTD(hc, termetd); // this one's not linked yet
                break;
            }
        }
        // TERM packet
        ctrlstatus |= ETCF_DATA1|ETCF_READYINTEN;
        ctrlstatus ^= (ETCF_PIDCODE_IN^ETCF_PIDCODE_OUT);

        predetd->etd_NextTD = termetd->etd_Self;
        predetd->etd_Succ = termetd;
        CONSTWRITEMEM32_LE(&termetd->etd_NextTD, EHCI_TERMINATE);
        CONSTWRITEMEM32_LE(&termetd->etd_AltNextTD, EHCI_TERMINATE);
        WRITEMEM32_LE(&termetd->etd_CtrlStatus, ctrlstatus);
        termetd->etd_Length = 0;
        termetd->etd_BufferPtr[0] = 0; // clear for overlay bits
        termetd->etd_BufferPtr[1] = 0; // clear for overlay bits
        termetd->etd_BufferPtr[2] = 0; // clear for overlay bits
        termetd->etd_Succ = NULL;

        // due to sillicon bugs, we fill in the first overlay ourselves.
        eqh->eqh_CurrTD = setupetd->etd_Self;
        eqh->eqh_NextTD = setupetd->etd_NextTD;
        eqh->eqh_AltNextTD = setupetd->etd_AltNextTD;
        eqh->eqh_CtrlStatus = setupetd->etd_CtrlStatus;
        eqh->eqh_BufferPtr[0] = setupetd->etd_BufferPtr[0];
        eqh->eqh_BufferPtr[1] = setupetd->etd_BufferPtr[1];
        eqh->eqh_BufferPtr[2] = 0;

        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        ioreq->iouh_DriverPrivate1 = eqh;

        // manage endpoint going busy
        unit->hu_DevBusyReq[devadrep] = ioreq;
        unit->hu_NakTimeoutFrame[devadrep] = (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + (ioreq->iouh_NakTimeout<<3) : 0;

        Disable();
        AddTail(&hc->hc_TDQueue, (struct Node *) ioreq);

        // looks good to me, now enqueue this entry (just behind the asyncQH)
        eqh->eqh_Succ = hc->hc_EhciAsyncQH->eqh_Succ;
        eqh->eqh_NextQH = eqh->eqh_Succ->eqh_Self;
        SYNC;
        EIEIO;
        eqh->eqh_Pred = hc->hc_EhciAsyncQH;
        eqh->eqh_Succ->eqh_Pred = eqh;
        hc->hc_EhciAsyncQH->eqh_Succ = eqh;
        hc->hc_EhciAsyncQH->eqh_NextQH = eqh->eqh_Self;
        SYNC;
        EIEIO;
        Enable();

        ioreq = (struct IOUsbHWReq *) hc->hc_CtrlXFerQueue.lh_Head;
    }
}
/* \\\ */

/* /// "ehciScheduleIntTDs()" */
void ehciScheduleIntTDs(struct PCIController *hc)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    UWORD devadrep;
    UWORD cnt;
    struct EhciQH *eqh;
    struct EhciQH *inteqh;
    struct EhciTD *etd;
    struct EhciTD *predetd;
    ULONG epcaps;
    ULONG ctrlstatus;
    ULONG splitctrl;
    ULONG len;
    ULONG phyaddr;

    /* *** INT Transfers *** */
    KPRINTF(1, ("Scheduling new INT transfers...\n"));
    ioreq = (struct IOUsbHWReq *) hc->hc_IntXFerQueue.lh_Head;
    while(((struct Node *) ioreq)->ln_Succ)
    {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
        KPRINTF(10, ("New INT transfer to %ld.%ld: %ld bytes\n", ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length));
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep])
        {
            KPRINTF(5, ("Endpoint %02lx in use!\n", devadrep));
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }

        eqh = ehciAllocQH(hc);
        if(!eqh)
        {
            break;
        }

        eqh->eqh_IOReq = ioreq;
        eqh->eqh_Actual = 0;

        epcaps = (0<<EQES_RELOAD)|(ioreq->iouh_MaxPktSize<<EQES_MAXPKTLEN)|(ioreq->iouh_DevAddr<<EQES_DEVADDR)|(ioreq->iouh_Endpoint<<EQES_ENDPOINT);
        if(ioreq->iouh_Flags & UHFF_SPLITTRANS)
        {
            KPRINTF(10, ("*** SPLIT TRANSACTION to HubPort %ld at Addr %ld\n", ioreq->iouh_SplitHubPort, ioreq->iouh_SplitHubAddr));
            // full speed and low speed handling
            if(ioreq->iouh_Flags & UHFF_LOWSPEED)
            {
                KPRINTF(10, ("*** LOW SPEED ***\n"));
                epcaps |= EQEF_LOWSPEED;
            }
            WRITEMEM32_LE(&eqh->eqh_SplitCtrl, (EQSF_MULTI_1|(0x01<<EQSS_MUSOFACTIVE)|(0x1c<<EQSS_MUSOFCSPLIT))|(ioreq->iouh_SplitHubPort<<EQSS_PORTNUMBER)|(ioreq->iouh_SplitHubAddr<<EQSS_HUBADDRESS));
            if(ioreq->iouh_Interval >= 255)
            {
                inteqh = hc->hc_EhciIntQH[8]; // 256ms interval
            } else {
                cnt = 0;
                do
                {
                    inteqh = hc->hc_EhciIntQH[cnt++];
                } while(ioreq->iouh_Interval > (1<<cnt));
            }
        } else {
            epcaps |= EQEF_HIGHSPEED;
            if(ioreq->iouh_Flags & UHFF_MULTI_3)
            {
                splitctrl = EQSF_MULTI_3;
            }
            else if(ioreq->iouh_Flags & UHFF_MULTI_2)
            {
                splitctrl = EQSF_MULTI_2;
            } else {
                splitctrl = EQSF_MULTI_1;
            }
            if(ioreq->iouh_Interval < 2) // 0-1 µFrames
            {
                splitctrl |= (0xff<<EQSS_MUSOFACTIVE);
            }
            else if(ioreq->iouh_Interval < 4) // 2-3 µFrames
            {
                splitctrl |= (0x55<<EQSS_MUSOFACTIVE);
            }
            else if(ioreq->iouh_Interval < 8) // 4-7 µFrames
            {
                splitctrl |= (0x22<<EQSS_MUSOFACTIVE);
            }
            else if(ioreq->iouh_Interval > 511) // 64ms and higher
            {
                splitctrl |= (0x10<<EQSS_MUSOFACTIVE);
            }
            else //if(ioreq->iouh_Interval >= 8) // 1-64ms
            {
                splitctrl |= (0x01<<EQSS_MUSOFACTIVE);
            }
            WRITEMEM32_LE(&eqh->eqh_SplitCtrl, splitctrl);
            if(ioreq->iouh_Interval >= 1024)
            {
                inteqh = hc->hc_EhciIntQH[10]; // 1024µFrames interval
            } else {
                cnt = 0;
                do
                {
                    inteqh = hc->hc_EhciIntQH[cnt++];
                } while(ioreq->iouh_Interval > (1<<cnt));
            }
        }
        WRITEMEM32_LE(&eqh->eqh_EPCaps, epcaps);
        //eqh->eqh_CtrlStatus = eqh->eqh_CurrTD = 0;
        eqh->eqh_FirstTD = NULL; // clear for ehciFreeQHandTDs()

        ctrlstatus = (ioreq->iouh_Dir == UHDIR_IN) ? (ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_IN) : (ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_OUT);
        if(unit->hu_DevDataToggle[devadrep])
        {
            // continue with data toggle 0
            ctrlstatus |= ETCF_DATA1;
        }
        predetd = NULL;
        phyaddr = (ULONG) pciGetPhysical(hc, ioreq->iouh_Data);
        do
        {
            etd = ehciAllocTD(hc);
            if(!etd)
            {
                break;
            }
            if(predetd)
            {
                predetd->etd_Succ = etd;
                predetd->etd_NextTD = etd->etd_Self;
                predetd->etd_AltNextTD = hc->hc_ShortPktEndTD->etd_Self;
            } else {
                eqh->eqh_FirstTD = etd;
                //eqh->eqh_AltNextTD = eqh->eqh_NextTD = etd->etd_Self;
            }

            len = ioreq->iouh_Length - eqh->eqh_Actual;
            if(len > 4*EHCI_PAGE_SIZE)
            {
                len = 4*EHCI_PAGE_SIZE;
            }
            etd->etd_Length = len;
            WRITEMEM32_LE(&etd->etd_CtrlStatus, ctrlstatus|(len<<ETSS_TRANSLENGTH));
            // FIXME need quark scatter gather mechanism here
            WRITEMEM32_LE(&etd->etd_BufferPtr[0], phyaddr);
            WRITEMEM32_LE(&etd->etd_BufferPtr[1], (phyaddr & EHCI_PAGE_MASK) + (1*EHCI_PAGE_SIZE));
            WRITEMEM32_LE(&etd->etd_BufferPtr[2], (phyaddr & EHCI_PAGE_MASK) + (2*EHCI_PAGE_SIZE));
            WRITEMEM32_LE(&etd->etd_BufferPtr[3], (phyaddr & EHCI_PAGE_MASK) + (3*EHCI_PAGE_SIZE));
            WRITEMEM32_LE(&etd->etd_BufferPtr[4], (phyaddr & EHCI_PAGE_MASK) + (4*EHCI_PAGE_SIZE));
            phyaddr += len;
            eqh->eqh_Actual += len;
            predetd = etd;
        } while(eqh->eqh_Actual < ioreq->iouh_Length);

        if(!etd)
        {
            // not enough etds? try again later
            ehciFreeQHandTDs(hc, eqh);
            break;
        }
        ctrlstatus |= ETCF_READYINTEN|(etd->etd_Length<<ETSS_TRANSLENGTH);
        WRITEMEM32_LE(&predetd->etd_CtrlStatus, ctrlstatus);

        CONSTWRITEMEM32_LE(&predetd->etd_NextTD, EHCI_TERMINATE);
        CONSTWRITEMEM32_LE(&predetd->etd_AltNextTD, EHCI_TERMINATE);
        predetd->etd_Succ = NULL;

        // due to sillicon bugs, we fill in the first overlay ourselves.
        etd = eqh->eqh_FirstTD;
        eqh->eqh_CurrTD = etd->etd_Self;
        eqh->eqh_NextTD = etd->etd_NextTD;
        eqh->eqh_AltNextTD = etd->etd_AltNextTD;
        eqh->eqh_CtrlStatus = etd->etd_CtrlStatus;
        eqh->eqh_BufferPtr[0] = etd->etd_BufferPtr[0];
        eqh->eqh_BufferPtr[1] = etd->etd_BufferPtr[1];
        eqh->eqh_BufferPtr[2] = etd->etd_BufferPtr[2];
        eqh->eqh_BufferPtr[3] = etd->etd_BufferPtr[3];
        eqh->eqh_BufferPtr[4] = etd->etd_BufferPtr[4];

        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        ioreq->iouh_DriverPrivate1 = eqh;

        // manage endpoint going busy
        unit->hu_DevBusyReq[devadrep] = ioreq;
        unit->hu_NakTimeoutFrame[devadrep] = (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + (ioreq->iouh_NakTimeout<<3) : 0;

        Disable();
        AddTail(&hc->hc_PeriodicTDQueue, (struct Node *) ioreq);

        // looks good to me, now enqueue this entry in the right IntQH
        eqh->eqh_Succ = inteqh->eqh_Succ;
        eqh->eqh_NextQH = eqh->eqh_Succ->eqh_Self;
        SYNC;
        EIEIO;
        eqh->eqh_Pred = inteqh;
        eqh->eqh_Succ->eqh_Pred = eqh;
        inteqh->eqh_Succ = eqh;
        inteqh->eqh_NextQH = eqh->eqh_Self;
        SYNC;
        EIEIO;
        Enable();

        ehciUpdateIntTree(hc);

        ioreq = (struct IOUsbHWReq *) hc->hc_IntXFerQueue.lh_Head;
    }
}
/* \\\ */

/* /// "ehciScheduleBulkTDs()" */
void ehciScheduleBulkTDs(struct PCIController *hc)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    UWORD devadrep;
    struct EhciQH *eqh;
    struct EhciTD *etd = NULL;
    struct EhciTD *predetd;
    ULONG epcaps;
    ULONG ctrlstatus;
    ULONG splitctrl;
    ULONG len;
    ULONG phyaddr;

    /* *** BULK Transfers *** */
    KPRINTF(1, ("Scheduling new BULK transfers...\n"));
    ioreq = (struct IOUsbHWReq *) hc->hc_BulkXFerQueue.lh_Head;
    while(((struct Node *) ioreq)->ln_Succ)
    {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
        KPRINTF(10, ("New BULK transfer to %ld.%ld: %ld bytes\n", ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length));
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep])
        {
            KPRINTF(5, ("Endpoint %02lx in use!\n", devadrep));
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }

        eqh = ehciAllocQH(hc);
        if(!eqh)
        {
            break;
        }

        eqh->eqh_IOReq = ioreq;
        eqh->eqh_Actual = 0;

        epcaps = (0<<EQES_RELOAD)|(ioreq->iouh_MaxPktSize<<EQES_MAXPKTLEN)|(ioreq->iouh_DevAddr<<EQES_DEVADDR)|(ioreq->iouh_Endpoint<<EQES_ENDPOINT);
        if(ioreq->iouh_Flags & UHFF_SPLITTRANS)
        {
            KPRINTF(10, ("*** SPLIT TRANSACTION to HubPort %ld at Addr %ld\n", ioreq->iouh_SplitHubPort, ioreq->iouh_SplitHubAddr));
            // full speed and low speed handling
            if(ioreq->iouh_Flags & UHFF_LOWSPEED)
            {
                KPRINTF(10, ("*** LOW SPEED ***\n"));
                epcaps |= EQEF_LOWSPEED;
            }
            WRITEMEM32_LE(&eqh->eqh_SplitCtrl, EQSF_MULTI_1|(ioreq->iouh_SplitHubPort<<EQSS_PORTNUMBER)|(ioreq->iouh_SplitHubAddr<<EQSS_HUBADDRESS));
        } else {
            epcaps |= EQEF_HIGHSPEED;
            if(ioreq->iouh_Flags & UHFF_MULTI_3)
            {
                splitctrl = EQSF_MULTI_3;
            }
            else if(ioreq->iouh_Flags & UHFF_MULTI_2)
            {
                splitctrl = EQSF_MULTI_2;
            } else {
                splitctrl = EQSF_MULTI_1;
            }
            WRITEMEM32_LE(&eqh->eqh_SplitCtrl, splitctrl);
        }
        WRITEMEM32_LE(&eqh->eqh_EPCaps, epcaps);
        //eqh->eqh_CtrlStatus = eqh->eqh_CurrTD = 0;
        eqh->eqh_FirstTD = NULL; // clear for ehciFreeQHandTDs()

        ctrlstatus = (ioreq->iouh_Dir == UHDIR_IN) ? (ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_IN) : (ETCF_3ERRORSLIMIT|ETCF_ACTIVE|ETCF_PIDCODE_OUT);
        if(unit->hu_DevDataToggle[devadrep])
        {
            // continue with data toggle 0
            ctrlstatus |= ETCF_DATA1;
        }
        predetd = NULL;
        phyaddr = (ULONG) pciGetPhysical(hc, ioreq->iouh_Data);
        do
        {
            if((eqh->eqh_Actual >= EHCI_TD_BULK_LIMIT) && (eqh->eqh_Actual < ioreq->iouh_Length))
            {
                KPRINTF(10, ("Bulk too large, splitting...\n"));
                break;
            }
            etd = ehciAllocTD(hc);
            if(!etd)
            {
                break;
            }
            if(predetd)
            {
                predetd->etd_Succ = etd;
                predetd->etd_NextTD = etd->etd_Self;
                predetd->etd_AltNextTD = hc->hc_ShortPktEndTD->etd_Self;
            } else {
                eqh->eqh_FirstTD = etd;
                //eqh->eqh_AltNextTD = eqh->eqh_NextTD = etd->etd_Self;
            }

            len = ioreq->iouh_Length - eqh->eqh_Actual;
            if(len > 4*EHCI_PAGE_SIZE)
            {
                len = 4*EHCI_PAGE_SIZE;
            }
            etd->etd_Length = len;
            KPRINTF(1, ("Bulk TD %08lx len %ld (%ld/%ld) phy=%08lx\n",
                         etd, len, eqh->eqh_Actual, ioreq->iouh_Length, phyaddr));
            WRITEMEM32_LE(&etd->etd_CtrlStatus, ctrlstatus|(len<<ETSS_TRANSLENGTH));
            // FIXME need quark scatter gather mechanism here
            WRITEMEM32_LE(&etd->etd_BufferPtr[0], phyaddr);
            WRITEMEM32_LE(&etd->etd_BufferPtr[1], (phyaddr & EHCI_PAGE_MASK) + (1*EHCI_PAGE_SIZE));
            WRITEMEM32_LE(&etd->etd_BufferPtr[2], (phyaddr & EHCI_PAGE_MASK) + (2*EHCI_PAGE_SIZE));
            WRITEMEM32_LE(&etd->etd_BufferPtr[3], (phyaddr & EHCI_PAGE_MASK) + (3*EHCI_PAGE_SIZE));
            WRITEMEM32_LE(&etd->etd_BufferPtr[4], (phyaddr & EHCI_PAGE_MASK) + (4*EHCI_PAGE_SIZE));
            phyaddr += len;
            eqh->eqh_Actual += len;

            predetd = etd;
        } while((eqh->eqh_Actual < ioreq->iouh_Length) || (len && (ioreq->iouh_Dir == UHDIR_OUT) && (eqh->eqh_Actual == ioreq->iouh_Length) && (!ioreq->iouh_Flags & UHFF_NOSHORTPKT) && ((eqh->eqh_Actual % ioreq->iouh_MaxPktSize) == 0)));

        if(!etd)
        {
            // not enough etds? try again later
            ehciFreeQHandTDs(hc, eqh);
            break;
        }
        ctrlstatus |= ETCF_READYINTEN|(predetd->etd_Length<<ETSS_TRANSLENGTH);
        WRITEMEM32_LE(&predetd->etd_CtrlStatus, ctrlstatus);

        predetd->etd_Succ = NULL;
        CONSTWRITEMEM32_LE(&predetd->etd_NextTD, EHCI_TERMINATE);
        CONSTWRITEMEM32_LE(&predetd->etd_AltNextTD, EHCI_TERMINATE);

        // due to sillicon bugs, we fill in the first overlay ourselves.
        etd = eqh->eqh_FirstTD;
        eqh->eqh_CurrTD = etd->etd_Self;
        eqh->eqh_NextTD = etd->etd_NextTD;
        eqh->eqh_AltNextTD = etd->etd_AltNextTD;
        eqh->eqh_CtrlStatus = etd->etd_CtrlStatus;
        eqh->eqh_BufferPtr[0] = etd->etd_BufferPtr[0];
        eqh->eqh_BufferPtr[1] = etd->etd_BufferPtr[1];
        eqh->eqh_BufferPtr[2] = etd->etd_BufferPtr[2];
        eqh->eqh_BufferPtr[3] = etd->etd_BufferPtr[3];
        eqh->eqh_BufferPtr[4] = etd->etd_BufferPtr[4];

        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        ioreq->iouh_DriverPrivate1 = eqh;

        // manage endpoint going busy
        unit->hu_DevBusyReq[devadrep] = ioreq;
        unit->hu_NakTimeoutFrame[devadrep] = (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + (ioreq->iouh_NakTimeout<<3) : 0;

        Disable();
        AddTail(&hc->hc_TDQueue, (struct Node *) ioreq);

        // looks good to me, now enqueue this entry (just behind the asyncQH)
        eqh->eqh_Succ = hc->hc_EhciAsyncQH->eqh_Succ;
        eqh->eqh_NextQH = eqh->eqh_Succ->eqh_Self;
        SYNC;
        EIEIO;
        eqh->eqh_Pred = hc->hc_EhciAsyncQH;
        eqh->eqh_Succ->eqh_Pred = eqh;
        hc->hc_EhciAsyncQH->eqh_Succ = eqh;
        hc->hc_EhciAsyncQH->eqh_NextQH = eqh->eqh_Self;
        SYNC;
        EIEIO;
        Enable();

        ioreq = (struct IOUsbHWReq *) hc->hc_BulkXFerQueue.lh_Head;
    }
}
/* \\\ */

/* /// "ehciCompleteInt()" */
void ehciCompleteInt(struct PCIController *hc)
{
    ULONG framecnt = READREG32_LE(hc->hc_RegBase, EHCI_FRAMECOUNT);

    KPRINTF(1, ("CompleteInt!\n"));
    hc->hc_FrameCounter = (hc->hc_FrameCounter & 0xffffc000) + framecnt;

    /* **************** PROCESS DONE TRANSFERS **************** */

    if(hc->hc_AsyncAdvanced)
    {
        struct EhciQH *eqh;
        struct EhciTD *etd;
        struct EhciTD *nextetd;

        hc->hc_AsyncAdvanced = FALSE;

        KPRINTF(1, ("AsyncAdvance %08lx\n", hc->hc_EhciAsyncFreeQH));

        while((eqh = hc->hc_EhciAsyncFreeQH))
        {
            KPRINTF(1, ("FreeQH %08lx\n", eqh));
            nextetd = eqh->eqh_FirstTD;
            while((etd = nextetd))
            {
                KPRINTF(1, ("FreeTD %08lx\n", nextetd));
                nextetd = etd->etd_Succ;
                ehciFreeTD(hc, etd);
            }
            hc->hc_EhciAsyncFreeQH = eqh->eqh_Succ;
            ehciFreeQH(hc, eqh);
        }
    }

    ehciHandleFinishedTDs(hc);

    if(hc->hc_CtrlXFerQueue.lh_Head->ln_Succ)
    {
        ehciScheduleCtrlTDs(hc);
    }

    if(hc->hc_IntXFerQueue.lh_Head->ln_Succ)
    {
        ehciScheduleIntTDs(hc);
    }

    if(hc->hc_BulkXFerQueue.lh_Head->ln_Succ)
    {
        ehciScheduleBulkTDs(hc);
    }

    KPRINTF(1, ("CompleteDone\n"));
}
/* \\\ */

/* /// "ehciIntCode()" */
void ehciIntCode(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
    struct PCIController *hc = (struct PCIController *) irq->h_Data;
    struct PCIDevice *base = hc->hc_Device;
    struct PCIUnit *unit = hc->hc_Unit;
    ULONG intr;

    KPRINTF(1, ("pciEhciInt()\n"));
    intr = READREG32_LE(hc->hc_RegBase, EHCI_USBSTATUS);
    if(intr & hc->hc_PCIIntEnMask)
    {
        WRITEREG32_LE(hc->hc_RegBase, EHCI_USBSTATUS, intr);
        KPRINTF(1, ("INT=%04lx\n", intr));
        if(!hc->hc_Online)
        {
            return;
        }
        if(intr & EHSF_FRAMECOUNTOVER)
        {
            ULONG framecnt = READREG32_LE(hc->hc_RegBase, EHCI_FRAMECOUNT);
            hc->hc_FrameCounter = (hc->hc_FrameCounter|0x3fff) + 1 + framecnt;
            KPRINTF(5, ("Frame Counter Rollover %ld\n", hc->hc_FrameCounter));
        }
        if(intr & EHSF_ASYNCADVANCE)
        {
            KPRINTF(1, ("AsyncAdvance\n"));
            hc->hc_AsyncAdvanced = TRUE;
        }
        if(intr & (EHSF_TDDONE|EHSF_TDERROR|EHSF_ASYNCADVANCE))
        {
            SureCause(base, &hc->hc_CompleteInt);
        }
        if(intr & EHSF_HOSTERROR)
        {
            KPRINTF(200, ("Host ERROR!\n"));
        }
        if(intr & EHSF_PORTCHANGED)
        {
            UWORD hciport;
            ULONG oldval;
            UWORD portreg = EHCI_PORTSC1;
            for(hciport = 0; hciport < hc->hc_NumPorts; hciport++, portreg += 4)
            {
                oldval = READREG32_LE(hc->hc_RegBase, portreg);
                // reflect port ownership (shortcut without hc->hc_PortNum20[hciport], as usb 2.0 maps 1:1)
                unit->hu_EhciOwned[hciport] = (oldval & EHPF_NOTPORTOWNER) ? FALSE : TRUE;
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
                WRITEREG32_LE(hc->hc_RegBase, portreg, oldval);
                KPRINTF(20, ("PCI Int Port %ld Change %08lx\n", hciport + 1, oldval));
                if(hc->hc_PortChangeMap[hciport])
                {
                    unit->hu_RootPortChanges |= 1UL<<(hciport + 1);
                }
            }
            uhwCheckRootHubChanges(unit);
        }
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
    struct OhciED *oed;
    UWORD devadrep;
    UWORD cnt;
    ULONG linkelem;
    ULONG ctrlstatus;

    //KPRINTF(10, ("NakTimeoutInt()\n"));

    // check for port status change for UHCI and frame rollovers and NAK Timeouts
    hc = (struct PCIController *) unit->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ)
    {
        if(!hc->hc_Online)
        {
            hc = (struct PCIController *) hc->hc_Node.ln_Succ;
            continue;
        }
        switch(hc->hc_HCIType)
        {
            case HCITYPE_UHCI:
            {
                ULONG framecnt = READREG16_LE(hc->hc_RegBase, UHCI_FRAMECOUNT);

                if(framecnt < (hc->hc_FrameCounter & 0xffff))
                {
                    hc->hc_FrameCounter = (hc->hc_FrameCounter|0xffff) + 1 + framecnt;
                    KPRINTF(10, ("Frame Counter Rollover %ld\n", hc->hc_FrameCounter));
                }
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
                            KPRINTF(1, ("Examining IOReq=%08lx with UQH=%08lx\n", ioreq, uqh));
                            devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
                            linkelem = READMEM32_LE(&uqh->uqh_Element);
                            if(linkelem & UHCI_TERMINATE)
                            {
                                KPRINTF(1, ("UQH terminated %08lx\n", linkelem));
                                if(framecnt > unit->hu_NakTimeoutFrame[devadrep])
                                {
                                    // give the thing the chance to exit gracefully
                                    KPRINTF(20, ("Terminated? NAK timeout %ld > %ld, IOReq=%08lx\n", framecnt, unit->hu_NakTimeoutFrame[devadrep], ioreq));
                                    SureCause(base, &hc->hc_CompleteInt);
                                }
                            } else {
                                utd = (struct UhciTD *) ((linkelem & UHCI_PTRMASK) - hc->hc_PCIVirtualAdjust - 16); // struct UhciTD starts 16 before physical TD
                                ctrlstatus = READMEM32_LE(&utd->utd_CtrlStatus);
                                if(ctrlstatus & UTCF_ACTIVE)
                                {
                                    if(framecnt > unit->hu_NakTimeoutFrame[devadrep])
                                    {
                                        // give the thing the chance to exit gracefully
                                        KPRINTF(20, ("NAK timeout %ld > %ld, IOReq=%08lx\n", framecnt, unit->hu_NakTimeoutFrame[devadrep], ioreq));
                                        ctrlstatus &= ~UTCF_ACTIVE;
                                        WRITEMEM32_LE(&utd->utd_CtrlStatus, ctrlstatus);
                                        SureCause(base, &hc->hc_CompleteInt);
                                    }
                                } else {
                                    if(framecnt > unit->hu_NakTimeoutFrame[devadrep])
                                    {
                                        // give the thing the chance to exit gracefully
                                        KPRINTF(20, ("Terminated? NAK timeout %ld > %ld, IOReq=%08lx\n", framecnt, unit->hu_NakTimeoutFrame[devadrep], ioreq));
                                        SureCause(base, &hc->hc_CompleteInt);
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
                ULONG framecnt = READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT);
                framecnt = hc->hc_FrameCounter = (hc->hc_FrameCounter & 0xffff0000) + framecnt;
                // NakTimeout
                ioreq = (struct IOUsbHWReq *) hc->hc_TDQueue.lh_Head;
                while(((struct Node *) ioreq)->ln_Succ)
                {
                    if(ioreq->iouh_Flags & UHFF_NAKTIMEOUT)
                    {
                        oed = (struct OhciED *) ioreq->iouh_DriverPrivate1;
                        if(oed)
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
                            ctrlstatus = READMEM32_LE(&oed->oed_HeadPtr);
                            KPRINTF(1, ("Examining IOReq=%08lx with OED=%08lx HeadPtr=%08lx\n", ioreq, oed, ctrlstatus));
                            if(framecnt > unit->hu_NakTimeoutFrame[devadrep])
                            {
                                //ohciDebugSchedule(hc);
                                if(ctrlstatus & OEHF_HALTED)
                                {
                                    // give the thing the chance to exit gracefully
                                    KPRINTF(20, ("Terminated? NAK timeout %ld > %ld, IOReq=%08lx\n", framecnt, unit->hu_NakTimeoutFrame[devadrep], ioreq));
                                    SureCause(base, &hc->hc_CompleteInt);
                                } else {
                                    // give the thing the chance to exit gracefully
                                    KPRINTF(20, ("NAK timeout %ld > %ld, IOReq=%08lx\n", framecnt, unit->hu_NakTimeoutFrame[devadrep], ioreq));
                                    ctrlstatus |= OEHF_HALTED;
                                    WRITEMEM32_LE(&oed->oed_HeadPtr, ctrlstatus);
                                    ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                                    unit->hu_DevBusyReq[devadrep] = NULL;
                                    Remove(&ioreq->iouh_Req.io_Message.mn_Node);
                                    ohciFreeEDContext(hc, oed);
                                    KPRINTF(1, ("Old Toggle %04lx:%ld\n", devadrep, unit->hu_DevDataToggle[devadrep]));
                                    unit->hu_DevDataToggle[devadrep] = (ctrlstatus & OEHF_DATA1) ? TRUE : FALSE;
                                    KPRINTF(1, ("Toggle now %04lx:%ld\n", devadrep, unit->hu_DevDataToggle[devadrep]));
                                    ReplyMsg(&ioreq->iouh_Req.io_Message);
                                }
                            }
                        }
                    }
                    ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
                }
                break;
            }

            case HCITYPE_EHCI:
            {
                ULONG framecnt = READREG32_LE(hc->hc_RegBase, EHCI_FRAMECOUNT);
                framecnt = hc->hc_FrameCounter = (hc->hc_FrameCounter & 0xffffc000) + framecnt;
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
                                KPRINTF(1, ("Examining IOReq=%08lx with EQH=%08lx\n", ioreq, eqh));
                                devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
                                ctrlstatus = READMEM32_LE(&eqh->eqh_CtrlStatus);
                                if(ctrlstatus & ETCF_ACTIVE)
                                {
                                    if(framecnt > unit->hu_NakTimeoutFrame[devadrep])
                                    {
                                        // give the thing the chance to exit gracefully
                                        KPRINTF(20, ("NAK timeout %ld > %ld, IOReq=%08lx\n", framecnt, unit->hu_NakTimeoutFrame[devadrep], ioreq));
                                        ctrlstatus &= ~ETCF_ACTIVE;
                                        ctrlstatus |= ETSF_HALTED;
                                        WRITEMEM32_LE(&eqh->eqh_CtrlStatus, ctrlstatus);
                                        SureCause(base, &hc->hc_CompleteInt);
                                    }
                                } else {
                                    if(ctrlstatus & ETCF_READYINTEN)
                                    {
                                        KPRINTF(10, ("INT missed?!? Manually causing it! %08lx, IOReq=%08lx\n",
                                                     ctrlstatus, ioreq));
                                        SureCause(base, &hc->hc_CompleteInt);
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
        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }

    uhwCheckRootHubChanges(unit);

    /* Update frame counter */
    unit->hu_NakTimeoutReq.tr_time.tv_micro = 150*1000;
    SendIO((APTR) &unit->hu_NakTimeoutReq);

    AROS_USERFUNC_EXIT
}
/* \\\ */

