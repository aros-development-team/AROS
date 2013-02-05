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

#define NewList         NEWLIST

#define FNAME_DEV(x)    USB2OTG__Dev__ ## x
#define	UtilityBase     USB2OTGBase->hd_UtilityBase

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
    unsigned int usbregval;

    D(bug("[USB2OTG] %s: USB2OTGBase @ 0x%p, SysBase @ 0x%p\n",
                 __PRETTY_FUNCTION__, USB2OTGBase, SysBase));

    usbregval = *((volatile unsigned int *)USB2OTG_VENDORID);

    if ((usbregval & 0xFFFFF000) != 0x4F542000)
    {
        D(bug("[USB2OTG] %s: No Supported HS OTG USB Core Found\n",
                __PRETTY_FUNCTION__));

        USB2OTGBase = NULL;
    }
    else
    {
        USB2OTGBase->hd_KernelBase = OpenResource("kernel.resource");
        D(bug("[USB2OTG] %s: kernel.resource opened @ 0x%p\n",
                __PRETTY_FUNCTION__, USB2OTGBase->hd_KernelBase));

        D(bug("[USB2OTG] %s: HS OTG Core Release: %x.%x%x%x\n",
                    __PRETTY_FUNCTION__, ((usbregval >> 12) & 0xF), ((usbregval >> 8) & 0xF), ((usbregval >> 4) & 0xF), (usbregval & 0xF)));

        if ((USB2OTGBase->hd_UtilityBase = (APTR)OpenLibrary("utility.library", 39)) != NULL)
        {
            USB2OTGBase->hd_MemPool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR | MEMF_SEM_PROTECTED, 16384, 4096);
            if (USB2OTGBase->hd_MemPool)
            {
                int chan;

                D(bug("[USB2OTG] %s: Allocated MemPool @ 0x%p\n",
                            __PRETTY_FUNCTION__, USB2OTGBase->hd_MemPool));

                for (chan = 0; chan < EPSCHANS_MAX; chan++) {
                    D(bug("[USB2OTG] %s: Host Chan #%d FIFO @ 0x%p, Characteristics: %08x\n",
                                __PRETTY_FUNCTION__,
                                chan, USB2OTG_FIFOBASE + (chan * USB2OTG_FIFOSIZE),
                                *((volatile unsigned int *)(USB2OTG_HOST_CHANBASE + (chan * USB2OTG_HOST_CHANREGSIZE) + USB2OTG_HOSTCHAN_CHARBASE))));
                }

                for (chan = 0; chan < EPSCHANS_MAX; chan++) {
                    D(bug("[USB2OTG] %s: Device Endpoint #%d IN_CTL: %08x, OUT_CTL: %08x\n",
                                __PRETTY_FUNCTION__, chan,
                                *((volatile unsigned int *)(USB2OTG_DEV_INEP_BASE + (chan * USB2OTG_DEV_EPSIZE) + USB2OTG_DEV_INEP_DIEPCTL)),
                                *((volatile unsigned int *)(USB2OTG_DEV_OUTEP_BASE + (chan * USB2OTG_DEV_EPSIZE) + USB2OTG_DEV_OUTEP_DOEPCTL))));
                }

                D(bug("[USB2OTG] %s: HWConfig1: %08x\n",
                            __PRETTY_FUNCTION__, *((volatile unsigned int *)USB2OTG_HARDWARE)));
                D(bug("[USB2OTG] %s: HWConfig2: %08x\n",
                            __PRETTY_FUNCTION__, *((volatile unsigned int *)USB2OTG_LPMCONFIG)));
                D(bug("[USB2OTG] %s: HWConfig3: %08x\n",
                            __PRETTY_FUNCTION__, *((volatile unsigned int *)USB2OTG_LPMCONFIG + 4)));
                D(bug("[USB2OTG] %s: HWConfig4: %08x\n",
                            __PRETTY_FUNCTION__, *((volatile unsigned int *)USB2OTG_LPMCONFIG + 8)));

                D(bug("[USB2OTG] %s: Host Mode Config: %08x\n",
                            __PRETTY_FUNCTION__, *((volatile unsigned int *)USB2OTG_HOSTCFG)));
                D(bug("[USB2OTG] %s: Device Mode Config: %08x\n",
                            __PRETTY_FUNCTION__, *((volatile unsigned int *)USB2OTG_DEVCFG)));

                D(bug("[USB2OTG] %s: Disabling USB Interrupts (Globaly)..\n", __PRETTY_FUNCTION__));
                usbregval = *((volatile unsigned int *)USB2OTG_AHB);
                usbregval &= ~USB2OTG_AHB_INTENABLE;
                *((volatile unsigned int *)USB2OTG_AHB) = usbregval;
                D(bug("[USB2OTG] %s: AHB : %08x\n", __PRETTY_FUNCTION__, *((volatile unsigned int *)USB2OTG_AHB)));

                USB2OTGBase->hd_Unit = AllocPooled(USB2OTGBase->hd_MemPool, sizeof(struct USB2OTGUnit));

                D(bug("[USB2OTG] %s: Unit Allocated at 0x%p\n",
                            __PRETTY_FUNCTION__, USB2OTGBase->hd_Unit));

                USB2OTGBase->hd_Unit->hu_GlobalIRQHandle = KrnAddIRQHandler(IRQ_VC_USB, GlobalIRQHandler, USB2OTGBase->hd_Unit, SysBase);

                D(bug("[USB2OTG] %s: Installed Global IRQ Handler [handle @ 0x%p]\n",
                            __PRETTY_FUNCTION__, USB2OTGBase->hd_Unit->hu_GlobalIRQHandle));

                D(bug("[USB2OTG] %s: HS OTG Driver Initialised\n",
                            __PRETTY_FUNCTION__));

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

    D(bug("[USB2OTG] %s: OpenCnt = %ld\n",
                __PRETTY_FUNCTION__, USB2OTGBase->hd_Library.lib_OpenCnt));

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
static int FNAME_DEV(Open)(LIBBASETYPEPTR USB2OTGBase, struct IOUsbHWReq *ioreq, ULONG unit, ULONG flags)
{
    D(bug("[USB2OTG] %s: IOReq @ 0x%p, unit #%ld, flags = 0x%08lx, USB2OTGBase @ 0x%p\n",
                __PRETTY_FUNCTION__, ioreq, unit, flags, USB2OTGBase));

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

//        ioreq->iouh_Req.io_Unit = Open_Unit(ioreq, unit, USB2OTGBase);
        if (!(ioreq->iouh_Req.io_Unit))
        {
            D(bug("[USB2OTG] %s: could not open unit!\n",
                        __PRETTY_FUNCTION__));

        }
        else
        {
            /* Opended ok! */
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

//    Close_Unit(USB2OTGBase, (struct USB2OTGUnit *) ioreq->iouh_Req.io_Unit, ioreq);

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

    struct USB2OTGUnit *unit = (struct USB2OTGUnit *) ioreq->iouh_Req.io_Unit;
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
//                ret = cmdReset(ioreq, unit, USB2OTGBase);
                break;

            case CMD_FLUSH:
//                ret = cmdFlush(ioreq, unit, USB2OTGBase);
                break;

            case UHCMD_QUERYDEVICE:
//                ret = cmdQueryDevice(ioreq, unit, USB2OTGBase);
                break;

            case UHCMD_USBRESET:
//                ret = cmdUsbReset(ioreq, unit, USB2OTGBase);
                break;

            case UHCMD_USBRESUME:
//                ret = cmdUsbResume(ioreq, unit, USB2OTGBase);
                break;

            case UHCMD_USBSUSPEND:
//                ret = cmdUsbSuspend(ioreq, unit, USB2OTGBase);
                break;

            case UHCMD_USBOPER:
//                ret = cmdUsbOper(ioreq, unit, USB2OTGBase);
                break;

            case UHCMD_CONTROLXFER:
//                ret = cmdControlXFer(ioreq, unit, USB2OTGBase);
                break;

            case UHCMD_BULKXFER:
//                ret = cmdBulkXFer(ioreq, unit, USB2OTGBase);
                break;

            case UHCMD_INTXFER:
//                ret = cmdIntXFer(ioreq, unit, USB2OTGBase);
                break;

            case UHCMD_ISOXFER:
//                ret = cmdIsoXFer(ioreq, unit, USB2OTGBase);
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
//                ret = cmdNSDeviceQuery((struct IOStdReq *) ioreq, unit, USB2OTGBase);
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
            /* Set error codes */
            ioreq->iouh_Req.io_Error = ret & 0xff;
        }
        /* Terminate the iorequest */
//        TermIO(ioreq, USB2OTGBase);
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
//        if (cmdAbortIO(ioreq, USB2OTGBase))
//        {
//            return(0);
//        }
    }
    return(-1);

    AROS_LIBFUNC_EXIT
}

