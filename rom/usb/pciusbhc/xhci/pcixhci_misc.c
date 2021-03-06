/*
    Copyright (C) 2014, The AROS Development Team. All rights reserved.

    Desc: PCI XHCI USB host controller
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 1

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

void FreeVecOnBoundary(APTR onboundary) {
}

APTR AllocVecOnBoundary(ULONG size, ULONG boundary, STRPTR description) {

    UBYTE *allocation;
    UBYTE *ret;

    IPTR   newsize;
    IPTR   alignement;
    IPTR   description_size;

    STRPTR default_description = "(something?!?)";

    mybug(-1, ("[ALLOCVECONBOUNDARY] size %d(0x%x), boundary %d(0x%x), %s\n", size, size, boundary, boundary, (description ? description : default_description)));

    if(!size) {
        mybug(-1, (" - Allocation called with zero size\n"));
        return NULL;
    }

    /* Sanity check, size can never exceed boundary if boundary is set */
    if((boundary<size) && (boundary != 0)) {
        mybug(-1, (" - Allocation called with size exceeding boundary (%d<%d)\n", boundary, size));
        return NULL;
    }

    if(boundary) {
        alignement = boundary;    // Align allocation to start at boundary
    } else {
        alignement = 64;          // Else allocation is aligned to 64 bytes
    }

    description_size = AROS_ROUNDUP2(strlen((description ? description : default_description)) + 1, sizeof(IPTR));

    newsize = size + 2*(sizeof(IPTR)) + description_size + alignement;

    mybug(-1, (" - Allocating size %d->%d\n", size, newsize));

    allocation = AllocMem(newsize, (MEMF_ANY|MEMF_CLEAR));

    mybug(-1, (" - Allocated space from %p to %p with boundary %d\n", allocation, allocation+newsize-1, boundary));

    /*
        Allocation:
            size (IPTR)
            description string rounded up to IPTR
            padding to 64 byte alignement or to boundary alignement
            return address minus one IPTR = address of original allocation
    */
    if(allocation) {
        ret = allocation;

        *(IPTR *)ret=newsize;
        ret +=sizeof(IPTR);

        strcpy(ret, (description ? description : default_description));
        ret += description_size;

        ret = (UBYTE *)AROS_ROUNDUP2((IPTR)ret, alignement) - sizeof(IPTR);
        *(IPTR *)ret = (IPTR)allocation;

        ret += sizeof(IPTR);

        mybug(-1, (" - Return allocation space from %p to %p\n", ret, ret+size-1));

        return ret;
    }

    mybug(-1, ("Allocation for %s failed!\n", (description ? description : default_description)));
    return NULL;
}
