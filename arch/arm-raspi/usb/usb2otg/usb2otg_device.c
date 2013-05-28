/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include <asm/bcm2835.h>
#include <hardware/usb2otg.h>

#include "usb2otg_intern.h"

#define DEVNAME         "usb2otg.device"

const char devname[]    = MOD_NAME_STRING;

static void GlobalIRQHandler(struct USB2OTGUnit *USBUnit, struct ExecBase *SysBase)
{
    D(bug("[USB2OTG] %s: Received USB interrupt for Unit @ 0x%p\n",
                 __PRETTY_FUNCTION__, USBUnit));
}

/*
 *===========================================================
 * Init(base)
 *===========================================================
 */
static int FNAME_DEV(Init)(LIBBASETYPEPTR USB2OTGBase)
{
    volatile unsigned int otg_RegVal, otg_OperatingMode = 0;

    D(bug("[USB2OTG] %s: USB2OTGBase @ 0x%p, SysBase @ 0x%p\n",
                 __PRETTY_FUNCTION__, USB2OTGBase, SysBase));

    otg_RegVal = *((volatile unsigned int *)USB2OTG_VENDORID);

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

        bug("[USB2OTG] HS OTG Core Release: %c%c%x.%x%x%x\n",
                    ((otg_RegVal >> 24) & 0xFF), ((otg_RegVal >> 16) & 0xFF),
                    ((otg_RegVal >> 12) & 0xF), ((otg_RegVal >> 8) & 0xF), ((otg_RegVal >> 4) & 0xF), (otg_RegVal & 0xF)
                    );

        otg_RegVal = *((volatile unsigned int *)USB2OTG_HARDWARE2);
        bug("[USB2OTG] Architecture: %0x\n", ((otg_RegVal & (3 << 3)) >> 3));

        if ((USB2OTGBase->hd_UtilityBase = (APTR)OpenLibrary("utility.library", 39)) != NULL)
        {
            USB2OTGBase->hd_MemPool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR | MEMF_SEM_PROTECTED, 16384, 4096);
            if (USB2OTGBase->hd_MemPool)
            {
                int ns;

                D(bug("[USB2OTG] %s: Allocated MemPool @ 0x%p\n",
                            __PRETTY_FUNCTION__, USB2OTGBase->hd_MemPool));

#if !defined(OTG_FORCEDEVICEMODE)
                otg_OperatingMode |= USB2OTG_USBHOSTMODE;
                D(
                    otg_RegVal = *((volatile unsigned int *)USB2OTG_HOSTCFG);
                    bug("[USB2OTG] %s: Host Mode Config: %08x\n",
                            __PRETTY_FUNCTION__, otg_RegVal);
                )
#endif
#if !defined(OTG_FORCEHOSTMODE)
                otg_OperatingMode |= USB2OTG_USBDEVICEMODE;
                D(
                    otg_RegVal = *((volatile unsigned int *)USB2OTG_DEVCFG);
                    bug("[USB2OTG] %s: Device Mode Config: %08x\n",
                        __PRETTY_FUNCTION__, otg_RegVal);
                )
#endif
                if (USB2OTGBase)
                {
                    D(
                        otg_RegVal = *((volatile unsigned int *)USB2OTG_HARDWARE);
                        bug("[USB2OTG] %s: HWConfig1: %08x\n",
                                __PRETTY_FUNCTION__, otg_RegVal);
                        otg_RegVal = *((volatile unsigned int *)USB2OTG_HARDWARE2);
                        bug("[USB2OTG] %s: HWConfig2: %08x\n",
                                __PRETTY_FUNCTION__, otg_RegVal);
                        otg_RegVal = *((volatile unsigned int *)USB2OTG_HARDWARE3);
                        bug("[USB2OTG] %s: HWConfig3: %08x\n",
                                __PRETTY_FUNCTION__, otg_RegVal);
                        otg_RegVal = *((volatile unsigned int *)USB2OTG_HARDWARE4);
                        bug("[USB2OTG] %s: HWConfig4: %08x\n",
                                __PRETTY_FUNCTION__, otg_RegVal);
                    )

#if (0)
                    D(bug("[USB2OTG] %s: Disabling USB Interrupts (Globaly)..\n", __PRETTY_FUNCTION__));
                    otg_RegVal = *((volatile unsigned int *)USB2OTG_AHB);
                    otg_RegVal &= ~USB2OTG_AHB_INTENABLE;
                    *((volatile unsigned int *)USB2OTG_AHB) = otg_RegVal;
                    D(bug("[USB2OTG] %s: AHB : %08x\n", __PRETTY_FUNCTION__, *((volatile unsigned int *)USB2OTG_AHB)));
#endif
                    D(bug("[USB2OTG] %s: Performing Soft Disconnect ..\n", __PRETTY_FUNCTION__));
                    *((volatile unsigned int *)USB2OTG_DEVCTRL) = (1 << 1);

                    for (ns = 0; ns < 32000; ns++) { asm volatile("mov r0, r0\n"); } // Wait 32ms

                    *((volatile unsigned int *)USB2OTG_RESET) = (1 << 0);

                    for (ns = 0; ns < 8000; ns++) { asm volatile("mov r0, r0\n"); } // Wait 8ms

                    if ((USB2OTGBase->hd_Unit = AllocPooled(USB2OTGBase->hd_MemPool, sizeof(struct USB2OTGUnit))) != NULL)
                    {
                        D(bug("[USB2OTG] %s: Unit Allocated at 0x%p\n",
                                    __PRETTY_FUNCTION__, USB2OTGBase->hd_Unit));

                        USB2OTGBase->hd_Unit->hu_OperatingMode = (otg_OperatingMode == (USB2OTG_USBHOSTMODE|USB2OTG_USBDEVICEMODE)) ? 0 : otg_OperatingMode;

                        D(bug("[USB2OTG] %s: Unit Mode %d\n",
                                    __PRETTY_FUNCTION__, USB2OTGBase->hd_Unit->hu_OperatingMode));

                        USB2OTGBase->hd_Unit->hu_GlobalIRQHandle = KrnAddIRQHandler(IRQ_VC_USB, GlobalIRQHandler, USB2OTGBase->hd_Unit, SysBase);

                        D(bug("[USB2OTG] %s: Installed Global IRQ Handler [handle @ 0x%p]\n",
                                    __PRETTY_FUNCTION__, USB2OTGBase->hd_Unit->hu_GlobalIRQHandle));

                        D(bug("[USB2OTG] %s: Preparing Controller (non HSIC mode) ..\n", __PRETTY_FUNCTION__));
                        *((volatile unsigned int *)USB2OTG_USB) = USB2OTG_USB_MODESELECT|USB2OTG_USB_USBTRDTIM(5)|(USB2OTGBase->hd_Unit->hu_OperatingMode << 29);
                        *((volatile unsigned int *)USB2OTG_OTGCTRL) = 0;

                        otg_RegVal = *((volatile unsigned int *)USB2OTG_LPMCONFIG);
                        otg_RegVal &= ~USB2OTG_LPMCONFIG_HSICCONNECT;
                        *((volatile unsigned int *)USB2OTG_LPMCONFIG) = otg_RegVal;

                        D(bug("[USB2OTG] %s: Clearing Global NAK ..\n", __PRETTY_FUNCTION__));
                        *((volatile unsigned int *)USB2OTG_DEVCTRL) = (1 << 10) | (1 << 8);

                        D(bug("[USB2OTG] %s: Disabling Port ..\n", __PRETTY_FUNCTION__));
                        *((volatile unsigned int *)USB2OTG_POWER) = 0xFFFFFFFF;

                        for (ns = 0; ns < 10000; ns++) { asm volatile("mov r0, r0\n"); } // Wait 10ms

                        D(bug("[USB2OTG] %s: Enabling Port ..\n", __PRETTY_FUNCTION__));
                        *((volatile unsigned int *)USB2OTG_POWER) = 0;

                        for (ns = 0; ns < 10000; ns++) { asm volatile("mov r0, r0\n"); } // Wait 10ms

                        D(bug("[USB2OTG] %s: Pulling-Up D+ ..\n", __PRETTY_FUNCTION__));
                        otg_RegVal = *((volatile unsigned int *)USB2OTG_DEVCTRL);
                        otg_RegVal &= ~(1 << 1);
                        *((volatile unsigned int *)USB2OTG_DEVCTRL) = otg_RegVal;

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

    D(bug("[USB2OTG] %s: IOReq @ 0x%08lx, USB2OTGBase @ 0x%08lx, cmd: %lu\n",
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
//        if (FNAME_DEV(cmdAbortIO)(ioreq, USB2OTGBase))
//        {
//            return(0);
//        }
    }
    return(-1);

    AROS_LIBFUNC_EXIT
}

