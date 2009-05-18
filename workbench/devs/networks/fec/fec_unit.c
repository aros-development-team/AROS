/*
 * fec_unit.c
 *
 *  Created on: May 18, 2009
 *      Author: misc
 */

#define DEBUG 1
#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <exec/memory.h>

#include <devices/sana2.h>
#include <devices/sana2specialstats.h>

#include <proto/openfirmware.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <inttypes.h>

#include "fec.h"



static struct AddressRange *FindMulticastRange(struct FECBase *FECBase, struct FECUnit *unit,
   ULONG lower_bound_left, UWORD lower_bound_right, ULONG upper_bound_left, UWORD upper_bound_right)
{
    struct AddressRange *range, *tail;
    BOOL found = FALSE;

    range = (APTR)unit->feu_MulticastRanges.mlh_Head;
    tail = (APTR)&unit->feu_MulticastRanges.mlh_Tail;

    while((range != tail) && !found)
    {
        if((lower_bound_left == range->lower_bound_left) &&
            (lower_bound_right == range->lower_bound_right) &&
            (upper_bound_left == range->upper_bound_left) &&
            (upper_bound_right == range->upper_bound_right))
            found = TRUE;
        else
            range = (APTR)range->node.mln_Succ;
    }

    if(!found)
        range = NULL;

    return range;
}

BOOL AddMulticastRange(struct FECBase *FECBase, struct FECUnit *unit, const UBYTE *lower_bound, const UBYTE *upper_bound)
{
    struct AddressRange *range;
    ULONG lower_bound_left, upper_bound_left;
    UWORD lower_bound_right, upper_bound_right;

    lower_bound_left = AROS_BE2LONG(*((ULONG *)lower_bound));
    lower_bound_right = AROS_BE2WORD(*((UWORD *)(lower_bound + 4)));
    upper_bound_left = AROS_BE2LONG(*((ULONG *)upper_bound));
    upper_bound_right = AROS_BE2WORD(*((UWORD *)(upper_bound + 4)));

    range = FindMulticastRange(FECBase, unit, lower_bound_left, lower_bound_right,
        upper_bound_left, upper_bound_right);

    if(range != NULL)
        range->add_count++;
    else
    {
        range = AllocPooled(FECBase->feb_Pool, sizeof(struct AddressRange));
        if(range != NULL)
        {
            range->lower_bound_left = lower_bound_left;
            range->lower_bound_right = lower_bound_right;
            range->upper_bound_left = upper_bound_left;
            range->upper_bound_right = upper_bound_right;
            range->add_count = 1;

            Disable();
            AddTail((APTR)&unit->feu_MulticastRanges, (APTR)range);
            Enable();

            if (unit->feu_RangeCount++ == 0)
            {
                unit->feu_Flags |= IFF_ALLMULTI;
                unit->set_multicast(unit);
            }
        }
    }

    return range != NULL;
}

BOOL RemMulticastRange(struct FECBase *FECBase, struct FECUnit *unit, const UBYTE *lower_bound, const UBYTE *upper_bound)
{
    struct AddressRange *range;
    ULONG lower_bound_left, upper_bound_left;
    UWORD lower_bound_right, upper_bound_right;

    lower_bound_left = AROS_BE2LONG(*((ULONG *)lower_bound));
    lower_bound_right = AROS_BE2WORD(*((UWORD *)(lower_bound + 4)));
    upper_bound_left = AROS_BE2LONG(*((ULONG *)upper_bound));
    upper_bound_right = AROS_BE2WORD(*((UWORD *)(upper_bound + 4)));

    range = FindMulticastRange(FECBase, unit, lower_bound_left, lower_bound_right,
        upper_bound_left, upper_bound_right);

    if(range != NULL)
    {
        if(--range->add_count == 0)
        {
            Disable();
            Remove((APTR)range);
            Enable();
            FreePooled(FECBase->feb_Pool, range, sizeof(struct AddressRange));

            if (--unit->feu_RangeCount == 0)
            {
                unit->feu_Flags &= ~IFF_ALLMULTI;
                unit->set_multicast(unit);
            }
        }
    }
    return range != NULL;
}

/* Unit process */
static
AROS_UFH3(void, FEC_UnitProcess,
      AROS_UFHA(char *,                 argPtr,         A0),
      AROS_UFHA(ULONG,                  argSize,        D0),
      AROS_UFHA(struct ExecBase *,      SysBase,        A6))
{
    AROS_USERFUNC_INIT

    struct MsgPort *iport;
    struct FECUnit *unit = (struct FECUnit *)(FindTask(NULL)->tc_UserData);
    struct Process *parent = unit->feu_Process;
    uint32_t rcvd;

    unit->feu_Process = (struct Process *)FindTask(NULL);

    D(bug("[FEC] Hello there.\n"));
    D(bug("[FEC] Process @ %p\n", unit->feu_Process));

    unit->feu_Flags = 0;
    unit->feu_OpenCount = 0;
    unit->feu_RangeCount = 0;

    iport = CreateMsgPort();

    unit->feu_InputPort = iport;

    unit->feu_TimerPort.mp_SigBit = SIGB_SINGLE;
    unit->feu_TimerPort.mp_Flags = PA_SIGNAL;
    unit->feu_TimerPort.mp_SigTask = FindTask(NULL);
    unit->feu_TimerPort.mp_Node.ln_Type = NT_MSGPORT;
    NEWLIST(&unit->feu_TimerPort.mp_MsgList);

    unit->feu_TimerRequest.tr_node.io_Message.mn_ReplyPort = &unit->feu_TimerPort;
    unit->feu_TimerRequest.tr_node.io_Message.mn_Length = sizeof(unit->feu_TimerRequest);

    OpenDevice((STRPTR)"timer.device", UNIT_MICROHZ, (struct IORequest *)&unit->feu_TimerRequest, 0);

    /* Start the unit now... */

    Signal(parent, SIGF_SINGLE);

    do
    {
        uint32_t sigset = 1 << iport->mp_SigBit |
                          SIGBREAKF_CTRL_C;

        rcvd = Wait(sigset);

        if (rcvd & SIGBREAKF_CTRL_C)
        {
            D(bug("[FEC] CTRL_C signal. Bye now ;) \n"));
        }
        else if (rcvd & (1 << iport->mp_SigBit))
        {
        	struct IOSana2Req *io;

        	/* Handle incoming transactions */
        	while ((io = (struct IOSana2Req *)GetMsg(iport))!= NULL);
        	{
        		D(bug("[FEC] Handle incomming transaction.\n"));
        		ObtainSemaphore(&unit->feu_Lock);
        		handle_request(unit->feu_FECBase, io);
        	}
        }


    } while((rcvd & SIGBREAKF_CTRL_C) == 0);

    AROS_USERFUNC_EXIT
}

int FEC_CreateUnit(struct FECBase *FECBase, fec_t *regs)
{
	int retval = 0;
	struct FECUnit *unit = NULL;

	D(bug("[FEC] Creating FEC Unit\n"));

	unit = AllocPooled(FECBase->feb_Pool, sizeof(struct FECUnit));

	if (unit)
	{
		int i;

		D(bug("[FEC] Unit @ %08x, Registers @ %08x\n", unit, regs));

		unit->feu_regs = regs;

		InitSemaphore(&unit->feu_Lock);

		NEWLIST(&unit->feu_Openers);
		NEWLIST(&unit->feu_MulticastRanges);
		NEWLIST(&unit->feu_TypeTrackers);

		unit->feu_FECBase = FECBase;

        for (i=0; i < REQUEST_QUEUE_COUNT; i++)
        {
            struct MsgPort *port = AllocPooled(FECBase->feb_Pool, sizeof(struct MsgPort));
            unit->feu_RequestPorts[i] = port;

            if (port)
            {
                NEWLIST(&port->mp_MsgList);
                port->mp_Flags = PA_IGNORE;
                port->mp_SigTask = &unit->feu_TXInt;
            }
        }
        unit->feu_RequestPorts[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;

        /* Create the unit's process */

        /* Unit's process pointer will temporarly contain the parent */
        unit->feu_Process = FindTask(NULL);
        CreateNewProcTags(
                         NP_Entry, (IPTR)FEC_UnitProcess,
                         NP_Name, "FEC Process",
                         NP_Priority, 0,
                         NP_UserData, (IPTR)unit,
                         NP_StackSize, 40960,
                         TAG_DONE);

        /* Wait for synchronisation signal */
        Wait(SIGF_SINGLE);

        D(bug("[FEC] Unit up and running\n"));

		retval = 1;
	}

	return retval;
}
