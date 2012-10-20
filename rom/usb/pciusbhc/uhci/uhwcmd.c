/* uhwcmd.c - pciuhci.device based on work by Chris Hodges */

#include <devices/usb_hub.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/timer.h>

#include <strings.h>

#include "uhwcmd.h"

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
            AROS_INTC1(interrupt->is_Code, interrupt->is_Data);
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
            unit->hu_NakTimeoutInt.is_Code = (VOID_FUNC)uhwNakTimeoutInt;

            CopyMem(unit->hu_TimerReq, &unit->hu_NakTimeoutReq, sizeof(struct timerequest));
            unit->hu_NakTimeoutReq.tr_node.io_Message.mn_ReplyPort = &unit->hu_NakTimeoutMsgPort;
            unit->hu_NakTimeoutMsgPort.mp_Node.ln_Type = NT_MSGPORT;
            unit->hu_NakTimeoutMsgPort.mp_Flags = PA_SOFTINT;
            unit->hu_NakTimeoutMsgPort.mp_SigTask = &unit->hu_NakTimeoutInt;
            NEWLIST(&unit->hu_NakTimeoutMsgPort.mp_MsgList);
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
        *((STRPTR *) tag->ti_Data) = "PCI UHCI USB 1.1 Host Controller";
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

    UWORD portreg;
    ULONG oldval;
    ULONG newval;

    if(ioreq->iouh_Endpoint) {
        return(UHIOERR_STALL);
    }

    if(len != ioreq->iouh_Length) {
        KPRINTF(20, ("RH: Len (%ld != %ld) mismatch!\n", len != ioreq->iouh_Length));
        return(UHIOERR_STALL);
    }

    switch(rt) {
        case (URTF_STANDARD|URTF_DEVICE):
            switch(req) {
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
            switch(req) {
                case USR_GET_DESCRIPTOR:
                    switch(val>>8) {
                        case UDT_DEVICE:
                            KPRINTF(1, ("RH: GetDeviceDescriptor (%ld)\n", len));
                            ioreq->iouh_Actual = (len > sizeof(struct UsbStdDevDesc)) ? sizeof(struct UsbStdDevDesc) : len;
                            CopyMem((APTR) &RHDevDesc, ioreq->iouh_Data, ioreq->iouh_Actual);
                            return(0);

                        case UDT_CONFIGURATION: {
                            UBYTE tmpbuf[9+9+7];
                            KPRINTF(1, ("RH: GetConfigDescriptor (%ld)\n", len));
                            CopyMem((APTR) &RHCfgDesc, tmpbuf, 9);
                            CopyMem((APTR) &RHIfDesc, &tmpbuf[9], 9);
                            CopyMem((APTR) &RHEPDesc, &tmpbuf[9+9], 7);
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
                                        if(((val & 0xff) == 2) && (source[1] == 0)) {
                                            *mptr++ = AROS_WORD2LE('0' + unit->hu_UnitNo);
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
                                KPRINTF(1, ("RH: GetLangArray %04lx (%ld)\n", val, len));
                                if(len > 1) {
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
                    if(len == 1) {
                        KPRINTF(1, ("RH: GetConfiguration\n"));
                        ((UBYTE *) ioreq->iouh_Data)[0] = 1;
                        ioreq->iouh_Actual = len;
                        return(0);
                    }
                    break;
            }
            break;

        case (URTF_CLASS|URTF_OTHER):
            switch(req) {
                case USR_SET_FEATURE:
                    if((!idx) && (idx > numports)) {
                        KPRINTF(20, ("Port %ld out of range\n", idx));
                        return(UHIOERR_STALL);
                    }
                    chc = unit->hu_PortMap11[idx - 1];

//                    if(unit->hu_EhciOwned[idx - 1])
//                    {
//                        hc = unit->hu_PortMap20[idx - 1];
//                        hciport = idx - 1;
//                    } else {
                        hc = chc;
                        hciport = unit->hu_PortNum11[idx - 1];
//                    }
//                    KPRINTF(10, ("Set Feature %ld maps from glob. Port %ld to local Port %ld (%s)\n", val, idx, hciport, unit->hu_EhciOwned[idx - 1] ? "EHCI" : "U/OHCI"));
                    cmdgood = FALSE;

                    portreg = hciport ? UHCI_PORT2STSCTRL : UHCI_PORT1STSCTRL;
                    oldval = READIO16_LE(hc->hc_RegBase, portreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE); // these are clear-on-write!
                    newval = oldval;
                    
                    switch(val) {
                     /* case UFS_PORT_CONNECTION: not possible */
                        case UFS_PORT_ENABLE:
                            KPRINTF(200, ("Enabling Port (%s)\n", newval & UHPF_PORTENABLE ? "already" : "ok"));
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
                            KPRINTF(200, ("Resetting Port (%s)\n", newval & UHPF_PORTRESET ? "already" : "ok"));

                            // this is an ugly blocking workaround to the inability of UHCI to clear reset automatically
                            newval &= ~(UHPF_PORTSUSPEND|UHPF_PORTENABLE);
                            newval |= UHPF_PORTRESET;
                            WRITEIO16_LE(hc->hc_RegBase, portreg, newval);
                            uhwDelayMS(25, unit);

                            newval = READIO16_LE(hc->hc_RegBase, portreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE|UHPF_PORTSUSPEND|UHPF_PORTENABLE);
                            KPRINTF(200, ("Reset=%s\n", newval & UHPF_PORTRESET ? "GOOD" : "BAD!"));

                            // like windows does it
                            newval &= ~UHPF_PORTRESET;
                            WRITEIO16_LE(hc->hc_RegBase, portreg, newval);
                            uhwDelayMicro(50, unit);

                            newval = READIO16_LE(hc->hc_RegBase, portreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE|UHPF_PORTSUSPEND);
                            KPRINTF(200, ("Reset=%s\n", newval & UHPF_PORTRESET ? "BAD!" : "GOOD"));
                            newval &= ~(UHPF_PORTSUSPEND|UHPF_PORTRESET);
                            newval |= UHPF_PORTENABLE;
                            WRITEIO16_LE(hc->hc_RegBase, portreg, newval);
                            hc->hc_PortChangeMap[hciport] |= UPSF_PORT_RESET|UPSF_PORT_ENABLE; // manually fake reset change

                            cnt = 100;
                            do {
                                uhwDelayMS(1, unit);
                                newval = READIO16_LE(hc->hc_RegBase, portreg);
                            } while(--cnt && (!(newval & UHPF_PORTENABLE)));

                            if(cnt) {
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

                     /* case UFS_PORT_LOW_SPEED: not possible
                        case UFS_C_PORT_CONNECTION:
                        case UFS_C_PORT_ENABLE:
                        case UFS_C_PORT_SUSPEND:
                        case UFS_C_PORT_OVER_CURRENT:
                        case UFS_C_PORT_RESET: */
                    }
                    
                    if(cmdgood) {
                        KPRINTF(5, ("Port %ld SET_FEATURE %04lx->%04lx\n", idx, oldval, newval));
                        WRITEIO16_LE(hc->hc_RegBase, portreg, newval);
                        return(0);
                    }
                    break;  /* USR_SET_FEATURE */

                case USR_CLEAR_FEATURE:
                    if((!idx) && (idx > numports)) {
                        KPRINTF(20, ("Port %ld out of range\n", idx));
                        return(UHIOERR_STALL);
                    }
//                    if(unit->hu_EhciOwned[idx - 1])
//                    {
//                        hc = unit->hu_PortMap20[idx - 1];
//                        hciport = idx - 1;
//                    } else {
                        hc = unit->hu_PortMap11[idx - 1];
                        hciport = unit->hu_PortNum11[idx - 1];
//                    }
//                    KPRINTF(10, ("Clear Feature %ld maps from glob. Port %ld to local Port %ld (%s)\n", val, idx, hciport, unit->hu_EhciOwned[idx - 1] ? "EHCI" : "U/OHCI"));
                    cmdgood = FALSE;

                    portreg = hciport ? UHCI_PORT2STSCTRL : UHCI_PORT1STSCTRL;
                    oldval = READIO16_LE(hc->hc_RegBase, portreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE); // these are clear-on-write!
                    newval = oldval;
                    switch(val) {
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

                    if(cmdgood) {
                        KPRINTF(5, ("Port %ld CLEAR_FEATURE %04lx->%04lx\n", idx, oldval, newval));
                        WRITEIO16_LE(hc->hc_RegBase, portreg, newval);
                        if(hc->hc_PortChangeMap[hciport]) {
                            unit->hu_RootPortChanges |= 1UL<<idx;
                        } else {
                            unit->hu_RootPortChanges &= ~(1UL<<idx);
                        }
                        return(0);
                    }

                    break;
            }
            break;

        case (URTF_IN|URTF_CLASS|URTF_OTHER):
            switch(req) {
                case USR_GET_STATUS: {
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
//                    if(unit->hu_EhciOwned[idx - 1])
//                    {
//                        hc = unit->hu_PortMap20[idx - 1];
//                        hciport = idx - 1;
//                    } else {
                        hc = unit->hu_PortMap11[idx - 1];
                        hciport = unit->hu_PortNum11[idx - 1];
//                    }


                    UWORD portreg = hciport ? UHCI_PORT2STSCTRL : UHCI_PORT1STSCTRL;
                    UWORD oldval = READIO16_LE(hc->hc_RegBase, portreg);
                    *mptr = AROS_WORD2LE(UPSF_PORT_POWER);
                    if(oldval & UHPF_PORTCONNECTED) *mptr |= AROS_WORD2LE(UPSF_PORT_CONNECTION);
                    if(oldval & UHPF_PORTENABLE) *mptr |= AROS_WORD2LE(UPSF_PORT_ENABLE);
                    if(oldval & UHPF_LOWSPEED) *mptr |= AROS_WORD2LE(UPSF_PORT_LOW_SPEED);
                    if(oldval & UHPF_PORTRESET) *mptr |= AROS_WORD2LE(UPSF_PORT_RESET);
                    if(oldval & UHPF_PORTSUSPEND) *mptr |= AROS_WORD2LE(UPSF_PORT_SUSPEND);

                    KPRINTF(200, ("UHCI Port %ld is %s\n", idx, oldval & UHPF_LOWSPEED ? "LOWSPEED" : "FULLSPEED"));
                    KPRINTF(200, ("UHCI Port %ld Status %08lx\n", idx, *mptr));

                    mptr++;
                    if(oldval & UHPF_ENABLECHANGE) {
                        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_ENABLE;
                    }
                    if(oldval & UHPF_CONNECTCHANGE) {
                        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_CONNECTION;
                    }
                    if(oldval & UHPF_RESUMEDTX) {
                        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_SUSPEND|UPSF_PORT_ENABLE;
                    }
                    *mptr = AROS_WORD2LE(hc->hc_PortChangeMap[hciport]);
                    WRITEIO16_LE(hc->hc_RegBase, portreg, oldval);
                    KPRINTF(5, ("UHCI Port %ld Change %08lx\n", idx, *mptr));
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
    while(((struct Node *) cmpioreq)->ln_Succ) {
        Remove(&cmpioreq->iouh_Req.io_Message.mn_Node);
        cmpioreq->iouh_Req.io_Error = IOERR_ABORTED;
        ReplyMsg(&cmpioreq->iouh_Req.io_Message);
        cmpioreq = (struct IOUsbHWReq *) unit->hu_RHIOQueue.lh_Head;
    }

    hc = (struct PCIController *) unit->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ) {
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
        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }
    Enable();
    /* Return success
    */
    return RC_OK;
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

        if(!foundit) {
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

        if(!foundit) {
            // IOReq is probably pending in some transfer structure
            devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);

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

        ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_FREEMSG;

        /* If not quick I/O, reply the message */
        if(!(ioreq->iouh_Req.io_Flags & IOF_QUICK)) {
            ReplyMsg(&ioreq->iouh_Req.io_Message);
        }
    }else{
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
AROS_INTH1(uhwNakTimeoutInt, struct PCIUnit *, unit)
{
    AROS_INTFUNC_INIT

    struct PCIDevice *base = unit->hu_Device;
    struct PCIController *hc;
    struct IOUsbHWReq *ioreq;
    struct UhciQH *uqh;
    struct UhciTD *utd;
    UWORD devadrep;
    ULONG linkelem;
    ULONG ctrlstatus;
    BOOL causeint;

    KPRINTF(1, ("Enter NakTimeoutInt(0x%p)\n", unit));

    // check for port status change for UHCI and frame rollovers and NAK Timeouts
    hc = (struct PCIController *) unit->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ) {
        if (!(hc->hc_Flags & HCF_ONLINE)) {
            hc = (struct PCIController *) hc->hc_Node.ln_Succ;
            continue;
        }
        causeint = FALSE;
        ULONG framecnt;
        uhciUpdateFrameCounter(hc);
        framecnt = hc->hc_FrameCounter;

        // NakTimeout
        ioreq = (struct IOUsbHWReq *) hc->hc_TDQueue.lh_Head;
        while(((struct Node *) ioreq)->ln_Succ) {
            if(ioreq->iouh_Flags & UHFF_NAKTIMEOUT) {
                uqh = (struct UhciQH *) ioreq->iouh_DriverPrivate1;
                if(uqh) {
                    KPRINTF(1, ("Examining IOReq=%p with UQH=%p\n", ioreq, uqh));
                    devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
                    linkelem = READMEM32_LE(&uqh->uqh_Element);
                    if(linkelem & UHCI_TERMINATE) {
                        KPRINTF(1, ("UQH terminated %08lx\n", linkelem));
                       if(framecnt > unit->hu_NakTimeoutFrame[devadrep]) {
                            // give the thing the chance to exit gracefully
                            KPRINTF(20, ("Terminated? NAK timeout %ld > %ld, IOReq=%p\n", framecnt, unit->hu_NakTimeoutFrame[devadrep], ioreq));
                            causeint = TRUE;
                        }
                    } else {
                        utd = (struct UhciTD *) (((IPTR)linkelem & UHCI_PTRMASK) - hc->hc_PCIVirtualAdjust - 16); // struct UhciTD starts 16 before physical TD
                        ctrlstatus = READMEM32_LE(&utd->utd_CtrlStatus);
                        if(ctrlstatus & UTCF_ACTIVE) {
                            if(framecnt > unit->hu_NakTimeoutFrame[devadrep]) {
                                // give the thing the chance to exit gracefully
                                KPRINTF(20, ("NAK timeout %ld > %ld, IOReq=%p\n", framecnt, unit->hu_NakTimeoutFrame[devadrep], ioreq));
                                ctrlstatus &= ~UTCF_ACTIVE;
                                WRITEMEM32_LE(&utd->utd_CtrlStatus, ctrlstatus);
                                causeint = TRUE;
                            }
                        } else {
                            if(framecnt > unit->hu_NakTimeoutFrame[devadrep]) {
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

        if(causeint) {
            SureCause(base, &hc->hc_CompleteInt);
        }

        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }

    uhwCheckRootHubChanges(unit);

    unit->hu_NakTimeoutReq.tr_time.tv_micro = 150*1000;
    SendIO((APTR) &unit->hu_NakTimeoutReq);

    KPRINTF(1, ("Exit NakTimeoutInt(0x%p)\n", unit));

    return FALSE;

    AROS_INTFUNC_EXIT
}
/* \\\ */

