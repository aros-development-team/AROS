/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: task.c $

    Desc: 
    Lang: English
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <devices/timer.h>

#include "card_intern.h"
    
BOOL pcmcia_newowner(struct CardResource *CardResource)
{
    if (CardResource->ownedcard)
    	return FALSE;
    if (CardResource->removed)
    	return FALSE;
    if (IsListEmpty(&CardResource->handles))
    	return FALSE;
    CardResource->ownedcard = (struct CardHandle*)RemHead(&CardResource->handles);
    CARDDEBUG(bug("pcmcia_newowner: %p\n", CardResource->ownedcard));
    if (CardResource->ownedcard->cah_CardInserted) {
	AROS_UFC3(UBYTE, CardResource->ownedcard->cah_CardInserted->is_Code,
	    AROS_UFCA(APTR, CardResource->ownedcard->cah_CardInserted->is_Data, A1),
	    AROS_UFCA(APTR, CardResource->ownedcard->cah_CardInserted->is_Code, A5),
	    AROS_UFCA(struct ExecBase*, SysBase, A6));
    }
    return TRUE;
}

void pcmcia_removeowner(struct CardResource *CardResource)
{
    if (CardResource->ownedcard == NULL)
    	return;
    CARDDEBUG(bug("pcmcia_removeowner: %p\n", CardResource->ownedcard));
    if (CardResource->ownedcard->cah_CardRemoved) {
	AROS_UFC3(UBYTE, CardResource->ownedcard->cah_CardRemoved->is_Code,
	    AROS_UFCA(APTR, CardResource->ownedcard->cah_CardRemoved->is_Data, A1),
	    AROS_UFCA(APTR, CardResource->ownedcard->cah_CardRemoved->is_Code, A5),
	    AROS_UFCA(struct ExecBase*, SysBase, A6));
    }
    CardResource->ownedcard = NULL;
}

void CardTask(struct Task *parent, struct CardResource *CardResource)
{
    UBYTE signal;
    UBYTE timersignal;
    
    signal = AllocSignal(-1);
    CardResource->signalmask = 1 << signal;

    timersignal = AllocSignal(-1);
    CardResource->timermp = CreateMsgPort();
    CardResource->timerio = (struct timerequest*) CreateIORequest(CardResource->timermp, sizeof(struct timerequest));
    if (!CardResource->timermp || !CardResource->timerio)
    	Alert(AT_DeadEnd | AG_NoMemory);
    if (OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest*)CardResource->timerio, 0))
    	Alert(AT_DeadEnd | AG_OpenDev);
    
    Signal(parent, SIGBREAKF_CTRL_F);

    for (;;) {
    	UBYTE status;
    	BOOL gotstatus;

    	Wait(CardResource->signalmask);

    	CARDDEBUG(bug("CardTask woken up\n"));
    	
    	CardResource->changecount++;
    	
    	Forbid();
    	/* Removal */
    	if (CardResource->removed == FALSE) {
	    CardResource->removed = TRUE;
	    pcmcia_removeowner(CardResource);
	}
	Permit();

	gotstatus = FALSE;
	status = 0;
	/* Debounce wait, not sure if needed */
	for (;;) {
	    CardResource->timerio->tr_node.io_Command = TR_ADDREQUEST;
	    CardResource->timerio->tr_time.tv_secs = 0;
	    CardResource->timerio->tr_time.tv_micro = 100000;
	    DoIO((struct IORequest*)CardResource->timerio);
	    if (gotstatus && status == pcmcia_havecard())
		break;
	    status = pcmcia_havecard();
	    gotstatus = TRUE;
	}

    	CARDDEBUG(bug("PCMCIA changecnt=%d removed=%d\n", CardResource->changecount, status == FALSE));

	CardResource->timerio->tr_time.tv_secs = 0;
	CardResource->timerio->tr_time.tv_micro = 10000;
	DoIO((struct IORequest*)CardResource->timerio);
 
	Forbid();

	SetSignal(0, CardResource->signalmask);

    	CardResource->removed = status == FALSE;

	pcmcia_reset(CardResource);

	CardResource->disabled = FALSE;
	pcmcia_enable_interrupts();

	/* Insert */
	if (CardResource->removed == FALSE) {
	    pcmcia_cardreset(CardResource);
	    pcmcia_newowner(CardResource);
	}

	Permit();

    }
}
