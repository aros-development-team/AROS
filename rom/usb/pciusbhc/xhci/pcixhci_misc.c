/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI XHCI USB host controller
    Lang: English
*/

#ifdef DEBUG
#undef DEBUG
#endif
//#define DEBUG 1

#include <aros/io.h>
#include <aros/debug.h>
#include <aros/macros.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/stdc.h>
#include <proto/arossupport.h>

#include <devices/usb.h>
#include <devices/usb_hub.h>
#include <devices/newstyle.h>
#include <devices/usbhardware.h>
#include <devices/timer.h>

#include <asm/io.h>
#include <inttypes.h>

#include <hidd/pci.h>
#include <hidd/hidd.h>

#include "pcixhci_intern.h"

#include LC_LIBDEFS_FILE

void PCIXHCI_Delay(struct PCIXHCIUnit *unit, ULONG msec) {
    /* Allocate a signal within this task context */
    unit->tr->tr_node.io_Message.mn_ReplyPort->mp_SigBit = SIGB_SINGLE;
    unit->tr->tr_node.io_Message.mn_ReplyPort->mp_SigTask = FindTask(NULL);

    /* Specify the request */
    unit->tr->tr_node.io_Command = TR_ADDREQUEST;
    unit->tr->tr_time.tv_secs = msec / 1000;
    unit->tr->tr_time.tv_micro = 1000 * (msec % 1000);

    /* Wait */
    DoIO((struct IORequest *)unit->tr);

    unit->tr->tr_node.io_Message.mn_ReplyPort->mp_SigTask = NULL;
}

BOOL PCIXHCI_CreateTimer(struct PCIXHCIUnit *unit) {
    mybug_unit(0, ("Entering function\n"));

    struct MsgPort *mp = NULL;

    mp = CreateMsgPort();
    if (mp) {
        unit->tr = (struct timerequest *)CreateIORequest(mp, sizeof(struct timerequest));
        if (unit->tr) {
            FreeSignal(mp->mp_SigBit);
            if (!OpenDevice((STRPTR)"timer.device", UNIT_MICROHZ, (struct IORequest *)unit->tr, 0)) {
                return TRUE;
            }
            DeleteIORequest((struct IORequest *)unit->tr);
            mp->mp_SigBit = AllocSignal(-1);
        }
        DeleteMsgPort(mp);
    }

    return FALSE;
}

void PCIXHCI_DeleteTimer(struct PCIXHCIUnit *unit) {
    mybug_unit(0, ("Entering function\n"));

    if (unit->tr) {
        unit->tr->tr_node.io_Message.mn_ReplyPort->mp_SigBit = AllocSignal(-1);
        CloseDevice((struct IORequest *)unit->tr);
        DeleteMsgPort(unit->tr->tr_node.io_Message.mn_ReplyPort);
        DeleteIORequest((struct IORequest *)unit->tr);
    }
}
