/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/arossupport.h>
#include <proto/alib.h>

#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/ports.h>

#include <dos/dos.h>

#include "macros.h"

// #include <oop/oop.h>

/* Hack: prevent hidd/graphics.h from beeing included */
// #include "graphics_internal.h"


VOID activescreen_taskentry();

struct activescreen_msg {
    struct Message message;
    OOP_Object *bmobj;
};


static struct Task *activescreen_task = NULL;
static struct Task *parent_task = NULL;
static struct MsgPort *activescreen_msgport = NULL;

static ULONG oksig	= SIGBREAKF_CTRL_E;
static ULONG failsig	= SIGBREAKF_CTRL_D;
static ULONG killsig	= SIGBREAKF_CTRL_C;

static struct ExecBase *sysbase = NULL;


static BOOL send_activescreen_msg( Object *bmobj, struct ExecBase *SysBase)
{
    /* Allocate a new message */
    struct activescreen_msg *msg;
    
    msg = AllocMem(sizeof (*msg), MEMF_PUBLIC | MEMF_CLEAR);
    if (NULL == msg)
    	return FALSE;
	
    msg->bmobj = bmobj;
    PutMsg(activescreen_msgport, (struct Message *)msg);
    
    return TRUE;
}


VOID activatebm_callback(APTR data, OOP_Object *bmobj, BOOL activated)
{
    
    struct GfxBase *GfxBase;
    
    GfxBase = (struct GfxBase *)data;
    
    if (!activated)
    	return;
    
    /* We must send it to some task that can handle this.
       We should NOT use LockIBase() inside here, since we might
       have locked layers also. So we instead send it to some othe task
    */
    
    if (!send_activescreen_msg(bmobj, SysBase)) {
        kprintf("!!! activatebm_callback: Could not send activatescreen message !!!!\n");
    }
  
    return;
    
}

BOOL init_activescreen_stuff(struct GfxBase *GfxBase)
{

    BOOL ok = FALSE;
    sysbase = SysBase;
    
kprintf("init_activescreen_stuff\n");
    
    parent_task = FindTask(NULL);
    
    /* Create the task */
    activescreen_task = CreateTask("Active screen updating task"
    	, 40
	, activescreen_taskentry
	, 4096
    );
    
    if (NULL != activescreen_task) {
	ULONG sigs;
	
	kprintf("Task created\n");
	
	sigs = Wait(oksig | failsig);
	kprintf("Got sig: %d\n", sigs); 
	if (sigs & oksig) {
	     ok = TRUE;
	}
    }
    
    return ok;
    
}

VOID cleanup_activescreen_stuff(struct GfxBase *GfxBase)
{
    /* Kill the task. Note that this
       may cause race condition problems
       if some task continues to send messages to the task
     */
    Signal(activescreen_task, killsig);
    
}

#undef SysBase
#define SysBase sysbase

VOID activescreen_taskentry()
{
    /* Create the msg port */
    struct IntuitionBase *IntuitionBase = NULL;
    BOOL ok = FALSE;
    
    activescreen_msgport = CreateMsgPort();
    if (NULL != activescreen_msgport) {
    	IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 0);
	if (NULL != IntuitionBase) {
	     ok = TRUE;
	} else {
	    kprintf("activescreen_task: Could not open intuition\n");
	}
    } else {
    	kprintf("activescreen_task: Could not create msgport\n");
    }
    
    if (!ok) {
	/* Signal failure to the creating task */
	Signal(parent_task, failsig);
	    
    } else {
        ULONG portsig;
	BOOL done = FALSE;
	
	Signal(parent_task, oksig);
	
	portsig = 1L << activescreen_msgport->mp_SigBit;
	
	/* Go into endeless loop handling the messages */
	while (!done) {
	
	    struct activescreen_msg *msg;
	    
	    ULONG sigs;
	    
	    if ((msg = (struct activescreen_msg *)GetMsg(activescreen_msgport))) {
	    	struct Screen *scr;
	    
	    	/* Handle the message. Go through the screen list
		   and find the screen that uses the supplied HIDD bitmap
		   object
		*/
		LockIBase(0UL);
		
		for (scr = IntuitionBase->FirstScreen; NULL != scr; scr = scr->NextScreen) {
		
		    /* Get the hidd object */
		    if (msg->bmobj == HIDD_BM_OBJ(scr->RastPort.BitMap)) {
		    
			kprintf("Active screen found: %s\n", scr->Title);
			
			IntuitionBase->ActiveScreen = scr;
		    
		    }
		}
		
		UnlockIBase(0UL);
		
		/* We do not reply the message, but instead free it here. */
		FreeMem(msg, sizeof (*msg));
		
	    } else {
	    	sigs = Wait(portsig | killsig);
	    }	
	    if (sigs & killsig) {
	    	/* Get outstanding messages */
		while ((msg = (struct activescreen_msg *)GetMsg(activescreen_msgport)))
			FreeMem(msg, sizeof (*msg));
	    	done = TRUE;
	    }
	    
	} /* while */
	
    } /* if (ok) */
    
    if (NULL != IntuitionBase)
	CloseLibrary((struct Library *)IntuitionBase);
	    
    if (NULL != activescreen_msgport)
	DeleteMsgPort(activescreen_msgport);
	    
}

