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
    
void pcmcia_newowner(struct CardResource *CardResource, BOOL doNotify)
{
    struct CardHandle *handle = NULL, *nexthandle;
    CARDDEBUG(bug("pcmcia_newowner: n=%d owned=%x rem=%d listempty=%d\n",
	doNotify, CardResource->ownedcard, CardResource->removed, IsListEmpty(&CardResource->handles)));
    Forbid();
    ForeachNode(&CardResource->handles, nexthandle) {
    	CARDDEBUG(bug("Possible node %p flags %02x\n", nexthandle, nexthandle->cah_CardFlags));
	if (!(nexthandle->cah_CardFlags & CARDF_USED)) {
	    handle = nexthandle;
	    break;
	}
    }
    if (handle != NULL && CardResource->ownedcard == NULL && CardResource->removed == FALSE) {
	CardResource->ownedcard = handle;
	CardResource->resetberr = (CardResource->ownedcard->cah_CardFlags & CARDF_RESETREMOVE) ? GAYLE_IRQ_RESET : 0;
	CARDDEBUG(bug("pcmcia_newowner: %p\n", CardResource->ownedcard));
	CardResource->ownedcard->cah_CardFlags |= CARDF_USED;
	if (doNotify && CardResource->ownedcard->cah_CardInserted) {
	    CARDDEBUG(bug("Executing cah_CardInserted(%p, %p)\n",
		CardResource->ownedcard->cah_CardInserted->is_Data,
		CardResource->ownedcard->cah_CardInserted->is_Code));
		AROS_UFIC1(CardResource->ownedcard->cah_CardInserted->is_Code,
		           CardResource->ownedcard->cah_CardInserted->is_Data);
	}
    }
    Permit();
}

void pcmcia_removeowner(struct CardResource *CardResource)
{
    CARDDEBUG(bug("pcmcia_removeowner: owned=%p\n", CardResource->ownedcard));
    Forbid();
    if (CardResource->ownedcard != NULL) {
	if (CardResource->ownedcard->cah_CardRemoved) {
	    CARDDEBUG(bug("Executing cah_CardRemoved(%p, %p)\n",
		CardResource->ownedcard->cah_CardRemoved->is_Data,
		CardResource->ownedcard->cah_CardRemoved->is_Code));
		AROS_UFIC1(CardResource->ownedcard->cah_CardRemoved->is_Code,
		           CardResource->ownedcard->cah_CardRemoved->is_Data);
	}
    }
    Permit();
}

void CardTask(struct Task *parent, struct CardResource *CardResource)
{
    UBYTE signal;
    
    signal = AllocSignal(-1);
    CardResource->signalmask = 1 << signal;

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
 
	SetSignal(0, CardResource->signalmask);

    	CardResource->removed = status == FALSE;

	pcmcia_reset(CardResource);

	CardResource->disabled = FALSE;
	pcmcia_clear_requests(CardResource);
	pcmcia_enable_interrupts();

	/* Insert */
	Forbid();
	if (CardResource->removed == FALSE) {
	    pcmcia_cardreset(CardResource);
	    pcmcia_newowner(CardResource, TRUE);
	}
	Permit();

    }
}
