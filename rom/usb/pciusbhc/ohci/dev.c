/*
   Copyright © 2002-2009, Chris Hodges. All rights reserved.
   Copyright © 2010-2012, The AROS Development Team. All rights reserved.
   $Id$
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <clib/alib_protos.h>

#include "debug.h"

#include "cmd_protos.h"
#include "pci_protos.h"

#define	UtilityBase	base->hd_UtilityBase

const char devname[] = MOD_NAME_STRING;
CONST_STRPTR xfer_names[] = {"CONTROL", "BULK", "ISO", "INT"};

static int devInit(LIBBASETYPEPTR base)
{
    KPRINTF(10, ("devInit base: 0x%p SysBase: 0x%p\n", base, SysBase));

    base->hd_UtilityBase = (APTR) OpenLibrary("utility.library", 39);

    if (UtilityBase)
    {
        base->hd_MemPool =
            CreatePool(MEMF_PUBLIC | MEMF_CLEAR | MEMF_SEM_PROTECTED, 16384,
            4096);
        if (base->hd_MemPool)
        {
            NewList(&base->hd_Units);

            KPRINTF(10, ("devInit: Ok\n"));
        }
        else
        {
            KPRINTF(10, ("devInit: CreatePool() failed!\n"));
            CloseLibrary((struct Library *)UtilityBase);
            base = NULL;
        }
    }
    else
    {
        KPRINTF(10,
            ("devInit: OpenLibrary(\"utility.library\", 39) failed!\n"));
        base = NULL;
    }

    KPRINTF(10, ("devInit: openCnt = %ld\n", base->hd_Library.lib_OpenCnt));

    return base ? TRUE : FALSE;
}

/*
 *===========================================================
 * devOpen(ioreq, unit, flags, base)
 *===========================================================
 *
 * This is the the DEV_OPEN function.
 *
 */
static int devOpen(LIBBASETYPEPTR base, struct IOUsbHWReq *ioreq,
    ULONG unit, ULONG flags)
{
    KPRINTF(10,
        ("devOpen ioreq: 0x%p unit: %ld flags: 0x%08lx base: 0x%p\n", ioreq,
            unit, flags, base));

    KPRINTF(10, ("devOpen: openCnt = %ld\n", base->hd_Library.lib_OpenCnt));

    if (ioreq->iouh_Req.io_Message.mn_Length < sizeof(struct IOUsbHWReq))
    {
        KPRINTF(20, ("devOpen: invalid MN_LENGTH!\n"));

        ioreq->iouh_Req.io_Error = IOERR_BADLENGTH;
    }
    else
    {
        /* Default to open failure. */
        ioreq->iouh_Req.io_Error = IOERR_OPENFAIL;

        ioreq->iouh_Req.io_Unit = Open_Unit(ioreq, unit, base);
        if (!ioreq->iouh_Req.io_Unit)
        {
            KPRINTF(20, ("devOpen: could not open unit!\n"));
        }
        else
        {
            /* Opended ok! */
            ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
            ioreq->iouh_Req.io_Error = 0;

            return TRUE;
        }
    }

    return FALSE;
}


/*
 *===========================================================
 * devClose(ioreq, base)
 *===========================================================
 *
 * This is the the DEV_EXPUNGE function.
 *
 */

static int devClose(LIBBASETYPEPTR base, struct IOUsbHWReq *ioreq)
{
    KPRINTF(10, ("devClose ioreq: 0x%p base: 0x%p\n", ioreq, base));

    Close_Unit(base, (struct PCIUnit *)ioreq->iouh_Req.io_Unit, ioreq);

    ioreq->iouh_Req.io_Unit = (APTR) - 1;
    ioreq->iouh_Req.io_Device = (APTR) - 1;
    return TRUE;
}


static int devExpunge(LIBBASETYPEPTR base)
{
    pciExpunge(base);

    DeletePool(base->hd_MemPool);

    KPRINTF(5, ("devExpunge: closelibrary utilitybase 0x%p\n",
            UtilityBase));
    CloseLibrary((struct Library *)UtilityBase);
    return TRUE;
}

ADD2INITLIB(devInit, 0)
ADD2OPENDEV(devOpen, 0)
ADD2CLOSEDEV(devClose, 0) ADD2EXPUNGELIB(devExpunge, 0)
    /*
     *===========================================================
     * devBeginIO(ioreq, base)
     *===========================================================
     *
     * This is the DEV_BEGINIO vector of the device.
     *
     */
    AROS_LH1(void, devBeginIO,
    AROS_LHA(struct IOUsbHWReq *, ioreq, A1), LIBBASETYPEPTR, base, 5, ohci)
{
    AROS_LIBFUNC_INIT
        struct PCIUnit *unit = (struct PCIUnit *)ioreq->iouh_Req.io_Unit;
    WORD ret;

    ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    ioreq->iouh_Req.io_Error = UHIOERR_NO_ERROR;

    if (ioreq->iouh_Req.io_Command < NSCMD_DEVICEQUERY)
    {
        switch (ioreq->iouh_Req.io_Command)
        {
        case CMD_RESET:
            ret = cmdReset(ioreq, unit, base);
            break;

        case CMD_FLUSH:
            ret = cmdFlush(ioreq, unit, base);
            break;

        case UHCMD_QUERYDEVICE:
            ret = cmdQueryDevice(ioreq, unit, base);
            break;

        case UHCMD_USBRESET:
            ret = cmdUsbReset(ioreq, unit, base);
            break;

        case UHCMD_USBRESUME:
            ret = cmdUsbResume(ioreq, unit, base);
            break;

        case UHCMD_USBSUSPEND:
            ret = cmdUsbSuspend(ioreq, unit, base);
            break;

        case UHCMD_USBOPER:
            ret = cmdUsbOper(ioreq, unit, base);
            break;

        case UHCMD_CONTROLXFER:
            ret = cmdXFer(ioreq, unit, base);
            break;

        case UHCMD_BULKXFER:
            ret = cmdXFer(ioreq, unit, base);
            break;

        case UHCMD_INTXFER:
            ret = cmdXFer(ioreq, unit, base);
            break;

        case UHCMD_ISOXFER:
            ret = cmdXFer(ioreq, unit, base);
            break;

        default:
            ret = IOERR_NOCMD;
            break;
        }
    }
    else
    {
        switch (ioreq->iouh_Req.io_Command)
        {
        case NSCMD_DEVICEQUERY:
            ret = cmdNSDeviceQuery((struct IOStdReq *)ioreq, unit, base);
            break;

        default:
            ret = IOERR_NOCMD;
            break;
        }
    }

    if (ret != RC_DONTREPLY)
    {
        KPRINTF(1, ("TermIO\n"));
        if (ret != RC_OK)
        {
            /* Set error codes */
            ioreq->iouh_Req.io_Error = ret & 0xff;
        }
        /* Terminate the iorequest */
        TermIO(ioreq, base);
    }

AROS_LIBFUNC_EXIT}

/*
 *===========================================================
 * devAbortIO(ioreq, base)
 *===========================================================
 *
 * This is the DEV_ABORTIO vector of the device. It abort
 * the given iorequest, and set
 *
 */
AROS_LH1(LONG, devAbortIO,
    AROS_LHA(struct IOUsbHWReq *, ioreq, A1), LIBBASETYPEPTR, base, 6, ohci)
{
    AROS_LIBFUNC_INIT
        KPRINTF(50, ("devAbortIO ioreq: 0x%p, command %ld, status %ld\n",
            ioreq, ioreq->iouh_Req.io_Command,
            ioreq->iouh_Req.io_Message.mn_Node.ln_Type));

    /* Is it pending? */
    if (ioreq->iouh_Req.io_Message.mn_Node.ln_Type == NT_MESSAGE)
    {
        if (cmdAbortIO(ioreq, base))
        {
            return 0;
        }
    }
    return -1;

AROS_LIBFUNC_EXIT}
