/*
    Copyright � 2013-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include <proto/alib.h>
#include <proto/mbox.h>
#include <hardware/videocore.h>

#include "usb2otg_intern.h"

#define DEVNAME         "usb2otg.device"

const char devname[]    = MOD_NAME_STRING;

AROS_INTP(FNAME_DEV(PendingInt));
AROS_INTP(FNAME_DEV(NakTimeoutInt));

IPTR	__arm_periiobase __attribute__((used)) = 0 ;

static void GlobalIRQHandler(struct USB2OTGUnit *USBUnit, struct ExecBase *SysBase)
{
    volatile unsigned int otg_RegVal;

//    D(bug("[USB2OTG] %s: Received USB interrupt for Unit @ 0x%p\n",
//                 __PRETTY_FUNCTION__, USBUnit));

    otg_RegVal = rd32le(USB2OTG_INTR);

    if (otg_RegVal & USB2OTG_INTRCORE_DMASTARTOFFRAME)
    {
        wr32le(USB2OTG_INTR, USB2OTG_INTRCORE_DMASTARTOFFRAME);
    }

    if (otg_RegVal & USB2OTG_INTRCORE_HOSTCHANNEL)
    {
        volatile unsigned int otg_ChanVal;
        otg_ChanVal = rd32le(USB2OTG_HOSTINTR);
        wr32le(USB2OTG_HOSTINTR, otg_ChanVal);
#if (0)
              for (chan = 0; chan < ... ; chan ++)
              {
                      if (otg_ChanVal & (1 << chan))
                      {
                              *((volatile unsigned int *)USB2OTG_HOST_CHANBASE + (chan * USB2OTG_HOST_CHANREGSIZE) + 0x0c) = 0;
                              D(bug("[USB2OTG] %s: Host Channel #%d Interrupt\n",
                                      __PRETTY_FUNCTION__, chan));
                      }
              }
#endif
    }

    wr32le(USB2OTG_INTR, otg_RegVal);
}

/*
 *===========================================================
 * Init(base)
 *===========================================================
 */
static int FNAME_DEV(Init)(LIBBASETYPEPTR USB2OTGBase)
{
    void *MBoxBase = NULL;
    volatile unsigned int otg_RegVal;
    unsigned int otg_OperatingMode = 0;
    ULONG *PwrOnMsg, *pwron = NULL;

    KernelBase = OpenResource("kernel.resource");
    MBoxBase = OpenResource("mbox.resource");

    __arm_periiobase = KrnGetSystemAttr(KATTR_PeripheralBase);

    D(bug("[USB2OTG] %s: USB2OTGBase @ 0x%p, SysBase @ 0x%p\n",
                 __PRETTY_FUNCTION__, USB2OTGBase, SysBase));

    otg_RegVal = rd32le(USB2OTG_VENDORID);

    if ((otg_RegVal & 0xFFFFF000) != 0x4F542000)
    {
        bug("[USB2OTG] Unsupported HS OTG USB Core Found\n");
        bug("[USB2OTG] Hardware: %c%c%x.%x%x%x\n",
                    ((otg_RegVal >> 24) & 0xFF), ((otg_RegVal >> 16) & 0xFF),
                    ((otg_RegVal >> 12) & 0xF), ((otg_RegVal >> 8) & 0xF), ((otg_RegVal >> 4) & 0xF), (otg_RegVal & 0xF)
                    );

        USB2OTGBase = NULL;
    }
    else
    {
        USB2OTGBase->hd_KernelBase = OpenResource("kernel.resource");
        D(bug("[USB2OTG] %s: kernel.resource opened @ 0x%p\n",
                __PRETTY_FUNCTION__, USB2OTGBase->hd_KernelBase));

        if((USB2OTGBase->hd_MsgPort = CreateMsgPort()))
        {
            if((USB2OTGBase->hd_TimerReq = (struct timerequest *) CreateIORequest(USB2OTGBase->hd_MsgPort, sizeof(struct timerequest))))
            {
                if(!OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *) USB2OTGBase->hd_TimerReq, 0))
                {
                    USB2OTGBase->hd_TimerReq->tr_node.io_Message.mn_Node.ln_Name = "USB2OTG Timer";
                    USB2OTGBase->hd_TimerReq->tr_node.io_Command = TR_ADDREQUEST;
                    D(bug("[USB2OTG] %s: timer.device opened\n",
                            __PRETTY_FUNCTION__));

                    bug("[USB2OTG] HS OTG Core Release: %c%c%x.%x%x%x\n",
                                ((otg_RegVal >> 24) & 0xFF), ((otg_RegVal >> 16) & 0xFF),
                                ((otg_RegVal >> 12) & 0xF), ((otg_RegVal >> 8) & 0xF), ((otg_RegVal >> 4) & 0xF), (otg_RegVal & 0xF)
                                );

                    otg_RegVal = rd32le(USB2OTG_HARDWARE2);
                    bug("[USB2OTG] Architecture: %d - ", ((otg_RegVal & (3 << 3)) >> 3));
                    switch (((otg_RegVal & (3 << 3)) >> 3))
                    {
                        case 2:
                            bug("Internal DMA\n");
                            break;
                        case 1:
                            bug("External DMA\n");
                            break;
                        default:
                            bug("Slave Only\n");
                            break;
                    }

                    D(bug("[USB2OTG] %s: Disabling USB Interrupts (Globaly)..\n", __PRETTY_FUNCTION__));
                    otg_RegVal = rd32le(USB2OTG_AHB);
                    otg_RegVal &= ~USB2OTG_AHB_INTENABLE;
                    wr32le(USB2OTG_INTRMASK, 0);
                    wr32le(USB2OTG_AHB, otg_RegVal);

                    D(bug("[USB2OTG] Powering on USB controller\n"));
                    pwron = AllocVec(9*sizeof(ULONG), MEMF_CLEAR);
                    PwrOnMsg = (ULONG*)(((IPTR)pwron + 15) & ~15);

                    D(bug("[USB2OTG] pwron=%p, PwrOnMsg=%p\n", pwron, PwrOnMsg));

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
                        D(bug("[USB2OTG] Power on state: %08x\n", AROS_LE2LONG(PwrOnMsg[6])));
                        if ((AROS_LE2LONG(PwrOnMsg[6]) & 1) == 0)
                        {
                            bug("[USB2OTG] Failed to power on USB controller\n");
                            USB2OTGBase = NULL;
                        }

                        if ((AROS_LE2LONG(PwrOnMsg[6]) & 2) != 0)
                        {
                            bug("[USB2OTG] USB HCD does not exist\n");
                            USB2OTGBase = NULL;
                        }
                    }
                    FreeVec(pwron);
                    PwrOnMsg = NULL;

                    if (!USB2OTGBase)
                    {
                        return FALSE;
                    }

                    if ((USB2OTGBase->hd_UtilityBase = (APTR)OpenLibrary("utility.library", 39)) != NULL)
                    {
                        USB2OTGBase->hd_MemPool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR | MEMF_SEM_PROTECTED, 16384, 4096);
                        if (USB2OTGBase->hd_MemPool)
                        {
                            int ns;

                            D(bug("[USB2OTG] %s: Allocated MemPool @ 0x%p\n",
                                        __PRETTY_FUNCTION__, USB2OTGBase->hd_MemPool));

                            if (USB2OTGBase)
                            {
                                D(
                                    otg_RegVal = rd32le(USB2OTG_HARDWARE);
                                    bug("[USB2OTG] %s: HWConfig: %08x-", __PRETTY_FUNCTION__, otg_RegVal);
                                    otg_RegVal = rd32le(USB2OTG_HARDWARE2);
                                    bug("%08x-", otg_RegVal);
                                    otg_RegVal = rd32le(USB2OTG_HARDWARE3);
                                    bug("%08x-", otg_RegVal);
                                    otg_RegVal = rd32le(USB2OTG_HARDWARE4);
                                    bug("%08x\n", otg_RegVal);
                                )

                                if ((USB2OTGBase->hd_Unit = AllocPooled(USB2OTGBase->hd_MemPool, sizeof(struct USB2OTGUnit))) != NULL)
                                {
                                    D(bug("[USB2OTG] %s: Unit Allocated at 0x%p\n",
                                                __PRETTY_FUNCTION__, USB2OTGBase->hd_Unit));

                                    NewList(&USB2OTGBase->hd_Unit->hu_IOPendingQueue);

                                    NewList(&USB2OTGBase->hd_Unit->hu_CtrlXFerQueue);
                                    NewList(&USB2OTGBase->hd_Unit->hu_IntXFerQueue);
                                    NewList(&USB2OTGBase->hd_Unit->hu_IsoXFerQueue);
                                    NewList(&USB2OTGBase->hd_Unit->hu_BulkXFerQueue);
                                    NewList(&USB2OTGBase->hd_Unit->hu_TDQueue);
                                    NewList(&USB2OTGBase->hd_Unit->hu_AbortQueue);
                                    NewList(&USB2OTGBase->hd_Unit->hu_PeriodicTDQueue);

                                    USB2OTGBase->hd_Unit->hu_PendingInt.is_Node.ln_Type = NT_INTERRUPT;
                                    USB2OTGBase->hd_Unit->hu_PendingInt.is_Node.ln_Name = "OTG2USB Pending Work Interrupt";
                                    USB2OTGBase->hd_Unit->hu_PendingInt.is_Node.ln_Pri  = 0;
                                    USB2OTGBase->hd_Unit->hu_PendingInt.is_Data = USB2OTGBase->hd_Unit;
                                    USB2OTGBase->hd_Unit->hu_PendingInt.is_Code = (VOID_FUNC)FNAME_DEV(PendingInt);

                                    USB2OTGBase->hd_Unit->hu_NakTimeoutInt.is_Node.ln_Type = NT_INTERRUPT;
                                    USB2OTGBase->hd_Unit->hu_NakTimeoutInt.is_Node.ln_Name = "OTG2USB NakTimeout Interrupt";
                                    USB2OTGBase->hd_Unit->hu_NakTimeoutInt.is_Node.ln_Pri  = -16;
                                    USB2OTGBase->hd_Unit->hu_NakTimeoutInt.is_Data = USB2OTGBase->hd_Unit;
                                    USB2OTGBase->hd_Unit->hu_NakTimeoutInt.is_Code = (VOID_FUNC)FNAME_DEV(NakTimeoutInt);

                                    USB2OTGBase->hd_Unit->hu_NakTimeoutMsgPort.mp_Node.ln_Type = NT_MSGPORT;
                                    USB2OTGBase->hd_Unit->hu_NakTimeoutMsgPort.mp_Flags = PA_SOFTINT;
                                    USB2OTGBase->hd_Unit->hu_NakTimeoutMsgPort.mp_SigTask = &USB2OTGBase->hd_Unit->hu_NakTimeoutInt;
                                    NewList(&USB2OTGBase->hd_Unit->hu_NakTimeoutMsgPort.mp_MsgList);

                                    CopyMem(USB2OTGBase->hd_TimerReq, &USB2OTGBase->hd_Unit->hu_NakTimeoutReq, sizeof(struct timerequest));
                                    USB2OTGBase->hd_Unit->hu_NakTimeoutReq.tr_node.io_Message.mn_ReplyPort = &USB2OTGBase->hd_Unit->hu_NakTimeoutMsgPort;

                                    USB2OTGBase->hd_Unit->hu_HubPortChanged = FALSE;

                                    USB2OTGBase->hd_Unit->hu_OperatingMode = (otg_OperatingMode == (USB2OTG_USBHOSTMODE|USB2OTG_USBDEVICEMODE)) ? 0 : otg_OperatingMode;

#if (0)
                                    D(bug("[USB2OTG] %s: Unit Mode %d\n",
                                                __PRETTY_FUNCTION__, USB2OTGBase->hd_Unit->hu_OperatingMode));
#endif
                                    USB2OTGBase->hd_Unit->hu_GlobalIRQHandle = KrnAddIRQHandler(IRQ_VC_USB, GlobalIRQHandler, USB2OTGBase->hd_Unit, SysBase);

                                    USB2OTGBase->hd_Unit->hu_USB2OTGBase = USB2OTGBase;

                                    D(bug("[USB2OTG] %s: Installed Global IRQ Handler [handle @ 0x%p] for IRQ #%ld\n",
                                                __PRETTY_FUNCTION__, USB2OTGBase->hd_Unit->hu_GlobalIRQHandle, IRQ_HOSTPORT));

                                    otg_RegVal = rd32le(USB2OTG_USB);
                                    otg_RegVal &= ~(USB2OTG_USB_ULPIDRIVEEXTERNALVBUS|USB2OTG_USB_TSDLINEPULSEENABLE);
                                    wr32le(USB2OTG_USB, otg_RegVal);

                                    D(bug("[USB2OTG] %s: Reseting Controller ..\n", __PRETTY_FUNCTION__));
                                    wr32le(USB2OTG_RESET, USB2OTG_RESET_CORESOFT);
                                    for (ns = 0; ns < 10000; ns++) { asm volatile("mov r0, r0\n"); } // Wait 10ms
                                    if ((rd32le(USB2OTG_RESET) & USB2OTG_RESET_CORESOFT) != 0)
                                        bug("[USB2OTG] %s: Reset Timed-Out!\n", __PRETTY_FUNCTION__);

                                    D(bug("[USB2OTG] %s: Initialising PHY ..\n", __PRETTY_FUNCTION__));
                                    otg_RegVal = rd32le(USB2OTG_USB);
                                    otg_RegVal &= ~USB2OTG_USB_PHYINTERFACE;
                                    otg_RegVal &= ~USB2OTG_USB_MODESELECT_UTMI;
                                    wr32le(USB2OTG_USB, otg_RegVal);

#if (0)
                                    D(bug("[USB2OTG] %s: Reseting Controller ..\n", __PRETTY_FUNCTION__));
                                    wr32le(USB2OTG_RESET, USB2OTG_RESET_CORESOFT);
                                    for (ns = 0; ns < 10000; ns++) { asm volatile("mov r0, r0\n"); } // Wait 10ms
                                    if ((rd32le(USB2OTG_RESET) & USB2OTG_RESET_CORESOFT) != 0)
                                        bug("[USB2OTG] %s: Reset Timed-Out!\n", __PRETTY_FUNCTION__);
#endif

                                    otg_RegVal = rd32le(USB2OTG_HARDWARE2);
                                    if (((otg_RegVal & (3 << 6) >> 6) == 2) && ((otg_RegVal & (3 << 8) >> 8) == 1))
                                    {
                                        D(bug("[USB2OTG] %s: ULPI FSLS configuration: enabled.\n", __PRETTY_FUNCTION__));
                                        otg_RegVal = rd32le(USB2OTG_USB);
                                        otg_RegVal |= (USB2OTG_USB_ULPIFSLS|USB2OTG_USB_ULPI_CLK_SUS_M);
                                        wr32le(USB2OTG_USB, otg_RegVal);
                                    } else {
                                        D(bug("[USB2OTG] %s: ULPI FSLS configuration: disabled.\n", __PRETTY_FUNCTION__));
                                        otg_RegVal = rd32le(USB2OTG_USB);
                                        otg_RegVal &= ~(USB2OTG_USB_ULPIFSLS|USB2OTG_USB_ULPI_CLK_SUS_M);
                                        wr32le(USB2OTG_USB, otg_RegVal);
                                    }

                                    D(bug("[USB2OTG] %s: Enabling DMA configuration..\n", __PRETTY_FUNCTION__));
                                    otg_RegVal = rd32le(USB2OTG_AHB);
                                    otg_RegVal &= ~(1 << USB2OTG_AHB_DMAREMAINDERMODE);
                                    otg_RegVal |= (1 << 4) | (USB2OTG_AHB_DMAENABLE|USB2OTG_AHB_DMAREMAINDERMODE_INCR);
                                    D(bug("[USB2OTG] %s: AHB reg: %08x..\n", __PRETTY_FUNCTION__, otg_RegVal));
                                    wr32le(USB2OTG_AHB, otg_RegVal);

#if (0)
                                    D(bug("[USB2OTG] %s: Operating Mode: ", __PRETTY_FUNCTION__));
                                    otg_RegVal = rd32le(USB2OTG_HARDWARE2);
                                    switch (otg_RegVal & 7)
                                    {
                                        case 0:
                                            D(bug("HNP/SRP\n"));
                                            otg_RegVal = rd32le(USB2OTG_USB);
                                            otg_RegVal |= (USB2OTG_USB_HNPCAPABLE|USB2OTG_USB_SRPCAPABLE);
                                            wr32le(USB2OTG_USB, otg_RegVal);
                                            break;
                                        case 1:
                                        case 3:
                                        case 5:
                                            D(bug("SRP\n"));
                                            otg_RegVal = rd32le(USB2OTG_USB);
                                            otg_RegVal &= ~USB2OTG_USB_HNPCAPABLE;
                                            otg_RegVal |= USB2OTG_USB_SRPCAPABLE;
                                            wr32le(USB2OTG_USB, otg_RegVal);
                                            break;
                                        case 2:
                                        case 4:
                                        case 6:
                                            D(bug("No HNP or SRP\n"));
                                            otg_RegVal = rd32le(USB2OTG_USB);
                                            otg_RegVal &= ~(USB2OTG_USB_HNPCAPABLE|USB2OTG_USB_SRPCAPABLE);
                                            wr32le(USB2OTG_USB, otg_RegVal);
                                            break;
                                    }
#else
                                    D(bug("[USB2OTG] %s: Disable HNP/SRP\n", __PRETTY_FUNCTION__));
                                    otg_RegVal = rd32le(USB2OTG_USB);
                                    otg_RegVal &= ~(USB2OTG_USB_HNPCAPABLE|USB2OTG_USB_SRPCAPABLE);
                                    wr32le(USB2OTG_USB, otg_RegVal);
#endif

                                    D(bug("[USB2OTG] %s: Enabling Global Interrupts ...\n", __PRETTY_FUNCTION__));
                                    otg_RegVal = rd32le(USB2OTG_INTR);
                                    otg_RegVal = ~0UL;
                                    wr32le(USB2OTG_INTR, otg_RegVal);

                                    otg_RegVal = rd32le(USB2OTG_INTRMASK);
                                    otg_RegVal |= USB2OTG_INTRCORE_DMASTARTOFFRAME;
                                    wr32le(USB2OTG_INTRMASK, otg_RegVal);

                                    otg_RegVal = rd32le(USB2OTG_AHB);
                                    otg_RegVal |= USB2OTG_AHB_INTENABLE;
                                    wr32le(USB2OTG_AHB, otg_RegVal);

                                    bug("[USB2OTG] HS OTG USB Driver Initialised\n");
                                }
                            }
                        }
                        else
                        {
                            D(bug("[USB2OTG] %s: Failed to Create MemPool\n",
                                        __PRETTY_FUNCTION__));

                            CloseLibrary((struct Library *) UtilityBase);
                            USB2OTGBase = NULL;
                        }
                    }
                    else
                    {
                        D(bug("[USB2OTG] %s: OpenLibrary(\"utility.library\", 39) failed!\n",
                                    __PRETTY_FUNCTION__));

                        USB2OTGBase = NULL;
                    }
                }
                else
                {
                    D(bug("[USB2OTG] %s: OpenDevice(\"timer.device\") failed!\n",
                                __PRETTY_FUNCTION__));

                    USB2OTGBase = NULL;
                }
            }
            else
            {
                D(bug("[USB2OTG] %s: Failed to allocate timer IORequest\n",
                            __PRETTY_FUNCTION__));

                USB2OTGBase = NULL;
            }
        }
        else
        {
            D(bug("[USB2OTG] %s: Failed to create MsgPort\n",
                        __PRETTY_FUNCTION__));

            USB2OTGBase = NULL;
        }
    }

    return USB2OTGBase ? TRUE : FALSE;
}

/*
 *===========================================================
 * Open(ioreq, unit, flags, base)
 *===========================================================
 *
 * This is the the DEV_OPEN function.
 *
 */
static int FNAME_DEV(Open)(LIBBASETYPEPTR USB2OTGBase, struct IOUsbHWReq *ioreq, ULONG otg_Unit, ULONG flags)
{
    D(bug("[USB2OTG] %s: IOReq @ 0x%p, unit #%ld, flags = 0x%08lx, USB2OTGBase @ 0x%p\n",
                __PRETTY_FUNCTION__, ioreq, otg_Unit, flags, USB2OTGBase));

    D(bug("[USB2OTG] %s: openCnt = %ld\n",
                __PRETTY_FUNCTION__, USB2OTGBase->hd_Library.lib_OpenCnt));

    if (ioreq->iouh_Req.io_Message.mn_Length < sizeof(struct IOUsbHWReq))
    {
        D(bug("[USB2OTG] %s: invalid MN_LENGTH!\n",
                    __PRETTY_FUNCTION__));

        ioreq->iouh_Req.io_Error = IOERR_BADLENGTH;
    }
    else
    {
        ioreq->iouh_Req.io_Error = IOERR_OPENFAIL;

        ioreq->iouh_Req.io_Unit = FNAME_DEV(OpenUnit)(ioreq, otg_Unit, USB2OTGBase);
        if (!(ioreq->iouh_Req.io_Unit))
        {
            D(bug("[USB2OTG] %s: could not open unit!\n",
                        __PRETTY_FUNCTION__));

        }
        else
        {
            ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
            ioreq->iouh_Req.io_Error                   = 0;

            return TRUE;
        }
    }

    return FALSE;
}


/*
 *===========================================================
 * Close(ioreq, base)
 *===========================================================
 *
 * This is the the DEV_EXPUNGE function.
 *
 */
static int FNAME_DEV(Close)(LIBBASETYPEPTR USB2OTGBase, struct IOUsbHWReq *ioreq)
{
    D(bug("[USB2OTG] %s: IOReq @ 0x%p, USB2OTGBase @ 0x%p\n",
                __PRETTY_FUNCTION__, ioreq, USB2OTGBase));

    FNAME_DEV(CloseUnit)(ioreq, (struct USB2OTGUnit *) ioreq->iouh_Req.io_Unit, USB2OTGBase);

    ioreq->iouh_Req.io_Unit   = (APTR) -1;
    ioreq->iouh_Req.io_Device = (APTR) -1;
    return TRUE;
}

static int FNAME_DEV(Expunge)(LIBBASETYPEPTR USB2OTGBase)
{
    DeletePool(USB2OTGBase->hd_MemPool);

    D(bug("[USB2OTG] %s: closing utility.library @ 0x%p\n",
                __PRETTY_FUNCTION__, UtilityBase));

    CloseLibrary((struct Library *) UtilityBase);
    return TRUE;
}

ADD2INITLIB(FNAME_DEV(Init), 0)
ADD2OPENDEV(FNAME_DEV(Open), 0)
ADD2CLOSEDEV(FNAME_DEV(Close), 0)
ADD2EXPUNGELIB(FNAME_DEV(Expunge), 0)

/*
 *===========================================================
 * BeginIO(ioreq, base)
 *===========================================================
 *
 * This is the DEV_BEGINIO vector of the device.
 *
 */
AROS_LH1(void, FNAME_DEV(BeginIO),
         AROS_LHA(struct IOUsbHWReq *, ioreq, A1),
	     LIBBASETYPEPTR, USB2OTGBase, 5, usb2otg)
{
    AROS_LIBFUNC_INIT

    struct USB2OTGUnit *otg_Unit = (struct USB2OTGUnit *) ioreq->iouh_Req.io_Unit;
    WORD ret;

    D(bug("[USB2OTG] %s: IOReq @ 0x%08lx, USB2OTGBase @ 0x%08lx [cmd:%lu]\n",
                __PRETTY_FUNCTION__, ioreq, USB2OTGBase, ioreq->iouh_Req.io_Command));

    ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    ioreq->iouh_Req.io_Error                   = UHIOERR_NO_ERROR;

    if (ioreq->iouh_Req.io_Command < NSCMD_DEVICEQUERY)
    {
        switch (ioreq->iouh_Req.io_Command)
        {
            case CMD_RESET:
                ret = FNAME_DEV(cmdReset)(ioreq, otg_Unit, USB2OTGBase);
                break;

            case CMD_FLUSH:
                ret = FNAME_DEV(cmdFlush)(ioreq, otg_Unit, USB2OTGBase);
                break;

            case UHCMD_QUERYDEVICE:
                ret = FNAME_DEV(cmdQueryDevice)(ioreq, otg_Unit, USB2OTGBase);
                break;

            case UHCMD_USBRESET:
                ret = FNAME_DEV(cmdUsbReset)(ioreq, otg_Unit, USB2OTGBase);
                break;

            case UHCMD_USBRESUME:
                ret = FNAME_DEV(cmdUsbResume)(ioreq, otg_Unit, USB2OTGBase);
                break;

            case UHCMD_USBSUSPEND:
                ret = FNAME_DEV(cmdUsbSuspend)(ioreq, otg_Unit, USB2OTGBase);
                break;

            case UHCMD_USBOPER:
                ret = FNAME_DEV(cmdUsbOper)(ioreq, otg_Unit, USB2OTGBase);
                break;

            case UHCMD_CONTROLXFER:
                ret = FNAME_DEV(cmdControlXFer)(ioreq, otg_Unit, USB2OTGBase);
                break;

            case UHCMD_BULKXFER:
                ret = FNAME_DEV(cmdBulkXFer)(ioreq, otg_Unit, USB2OTGBase);
                break;

            case UHCMD_INTXFER:
                ret = FNAME_DEV(cmdIntXFer)(ioreq, otg_Unit, USB2OTGBase);
                break;

            case UHCMD_ISOXFER:
                ret = FNAME_DEV(cmdIsoXFer)(ioreq, otg_Unit, USB2OTGBase);
                break;

            default:
                ret = IOERR_NOCMD;
                break;
        }
    }
    else
    {
        switch(ioreq->iouh_Req.io_Command)
        {
            case NSCMD_DEVICEQUERY:
                ret = FNAME_DEV(cmdNSDeviceQuery)((struct IOStdReq *) ioreq, otg_Unit, USB2OTGBase);
                break;

            default:
                ret = IOERR_NOCMD;
                break;
        }
    }

    if (ret != RC_DONTREPLY)
    {
        D(bug("[USB2OTG] %s: Terminating I/O..\n",
                    __PRETTY_FUNCTION__));

        if (ret != RC_OK)
        {
            ioreq->iouh_Req.io_Error = ret & 0xff;
        }
        FNAME_DEV(TermIO)(ioreq, USB2OTGBase);
    }

    AROS_LIBFUNC_EXIT
}

/*
 *===========================================================
 * AbortIO(ioreq, base)
 *===========================================================
 *
 * This is the DEV_ABORTIO vector of the device. It abort
 * the given iorequest, and set
 *
 */
AROS_LH1(LONG, FNAME_DEV(AbortIO),
         AROS_LHA(struct IOUsbHWReq *, ioreq, A1),
	     LIBBASETYPEPTR, USB2OTGBase, 6, usb2otg)
{
    AROS_LIBFUNC_INIT

    D(bug("[USB2OTG] %s: IOReq @ 0x%p, command %ld, status %ld\n",
                __PRETTY_FUNCTION__, ioreq, ioreq->iouh_Req.io_Command, ioreq->iouh_Req.io_Message.mn_Node.ln_Type));

    /* Is it pending? */
    if (ioreq->iouh_Req.io_Message.mn_Node.ln_Type == NT_MESSAGE)
    {
#if (0)
        if (FNAME_DEV(cmdAbortIO)(ioreq, USB2OTGBase))
        {
            return(0);
        }
#endif
    }
    return(-1);

    AROS_LIBFUNC_EXIT
}

void FNAME_DEV(Cause)(LIBBASETYPEPTR USB2OTGBase, struct Interrupt *interrupt)
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
