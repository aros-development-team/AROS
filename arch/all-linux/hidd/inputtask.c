/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Task used for wainting on events from linux
    Lang: English.
*/


#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/arossupport.h>

#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/lists.h>

#include <dos/dos.h>

#include <oop/oop.h>
#include <hidd/unixio.h>
#include <hidd/mouse.h>

#include "linux_intern.h"

#define INPUTTASK_PRIORITY 50

#define INPUTTASK_STACKSIZE (AROS_STACKSIZE)
#define INPUTTASK_NAME "Linux input event task"

/* this task should wait for mouse and keyboard events */

struct inputtask_params {
    struct Task *parent;
    ULONG ok_signal;
    ULONG fail_signal;
    ULONG kill_signal;
    struct linux_staticdata *lsd;
};


struct mouse_state {
    BOOL buts[3];
    BYTE dx;
    BYTE dy;
};

#undef OOPBase
#define OOPBase lsd->oopbase

#undef LSD
#define LSD lsd


static void update_mouse_state(   struct mouse_state *oldstate
				, struct mouse_state *newstate
				, struct pHidd_Mouse_Event *mev)
{
    ULONG i;
    ULONG hidd2but[] = {
	vHidd_Mouse_Button1,
	vHidd_Mouse_Button2,
	vHidd_Mouse_Button3
    };
    
    /* Discover the difference between the old and the new state */
    for (i = 0; i < 3; i ++) {
	if (oldstate->buts[i] != newstate->buts[i]) {
	    mev->button = hidd2but[i];
	    mev->type = ( oldstate->buts[i] ? vHidd_Mouse_Release : vHidd_Mouse_Press );
	    return;
	}
    }
    
    
    if (    (oldstate->dx != newstate->dx)
	 || (oldstate->dy != newstate->dy) ) {
	 
	 mev->x	= newstate->dx;
	 mev->y	= newstate->dy;
	 mev->type = vHidd_Mouse_Motion;

    }
    
}
static void free_unixio_message(struct MsgPort *msgport, struct linux_staticdata *lsd)
{
    struct Message *msg;
    
    msg = GetMsg(msgport);
    if (NULL == msg) {
	kprintf("!!! linux input task: NO MSG FROM UNIXIO !!!\n");
    } else {
	FreeMem(msg, sizeof (struct uioMessage));
    }
}

static VOID inputtask_entry(struct inputtask_params *inputparams)
{
    struct linux_staticdata *lsd;
    struct inputtask_params itp;
    UBYTE lastcode = 0xFF;
    
    struct MsgPort *kbd_port = NULL;
    struct MsgPort *mouse_port = NULL;
    
    struct mouse_state oldstate = { { 0, 0, 0 }, 0, 0 };
    struct pHidd_Mouse_Event mouse_event;
    
    HIDD *unixio = NULL;
    ULONG kbdsig, mousesig, sigs;
    /* We must copy the parameter struct because they are allocated
     on the parent's stack */
kprintf("INSIDE INPUT TASK\n");
    itp = *inputparams;
    lsd = itp.lsd;
kprintf("in inputtask: lsd = %p\n", lsd);

kprintf("CREATING UNIXIO,,, OOPBase=%p\n", lsd->oopbase);
kprintf("now\n");
    unixio = (HIDD)New_UnixIO(lsd->oopbase, SysBase);
kprintf("UNIXIO %p\n", unixio);
    if (NULL == unixio) {
    	goto failexit;
    }
    
    kbd_port   = CreateMsgPort();
    mouse_port = CreateMsgPort();
    
    if (NULL == kbd_port || NULL == mouse_port)
	goto failexit;
    
    Signal(itp.parent, itp.ok_signal);
    
    kbdsig	= 1L << kbd_port->mp_SigBit;
    mousesig	= 1L << mouse_port->mp_SigBit;
kprintf("SIGS: %p, %p\n", kbdsig, mousesig);
kprintf("FDS: %d, %d\n", lsd->kbdfd, lsd->mousefd);
    
    for (;;) {
	LONG err_kbd, err_mouse;
	
	kprintf("GETTING INPUT FROM UNIXIO\n");
	/* Turn on kbd support */
//	init_kbd(lsd);
	
	err_kbd		= Hidd_UnixIO_AsyncIO(unixio, lsd->kbdfd,   vHidd_UnixIO_Terminal, kbd_port,	vHidd_UnixIO_Read, SysBase);
	err_mouse	= Hidd_UnixIO_AsyncIO(unixio, lsd->mousefd, vHidd_UnixIO_Terminal, mouse_port,  vHidd_UnixIO_Read, SysBase);
	
//    	ret = (int)Hidd_UnixIO_Wait( unixio, lsd->kbdfd, vHidd_UnixIO_Read, NULL, NULL);
//	cleanup_kbd(lsd);
	kprintf("GOT INPUT FROM UNIXIO\n");
	
	sigs = Wait( kbdsig | mousesig );

	if (sigs & kbdsig) {
	
kprintf("---------- GOT KBD INPUT --------------------\n");
 	    for (;;) {
		UBYTE code;
		size_t bytesread;
	    
	    
		bytesread = read(lsd->kbdfd, &code, 1);
		if (-1 == bytesread)  {
	    	    kprintf("!!! COULD NOT READ FROM LINUX KBD DEVICE: %s\n"
			, strerror(errno));
		    break;
		
		} else {
	    	    /* Let the kbd hidd handle it */
		    /* Key doewn ? */
		    if (code < 0x80 && code == lastcode)
			break;
		    
	    	    kprintf("GOT SCANCODE %d from kbd hidd\n", code);
		    if (code == 1 || code == 81) {
			    kill(getpid(), SIGTERM);
		    }
		
		    /* Send code to the application */
ObtainSemaphore(&lsd->sema);
		    if (NULL != lsd->kbdhidd) {
			HIDD_LinuxKbd_HandleEvent(lsd->kbdhidd, code);
		    }
ReleaseSemaphore(&lsd->sema);
		    lastcode = code;
		}
		break;

	    }	/* for (;;) */
	    free_unixio_message(kbd_port, lsd);
	} /* if (sigs & kbdsig) */
	
	if (sigs & mousesig) {
	    ULONG i;
	    LONG bytesread;
	    UBYTE buf[4];
	    BYTE dx = 0, dy = 0;
	    struct mouse_state newstate;
	    
	    /* Got mouse event */
	    kprintf("------------- MOUSE EVENT ------------\n");
	    for (i = 0; i < 4; i ++) {
	    	bytesread = read(lsd->mousefd, &buf[i], 1);
		if (-1 == bytesread)
		{
		    if (errno == EAGAIN)
		    {
		    	i--;
			continue;
		    }
		    kprintf("!!! linux input task: Could not read from mouse device: %s\n", strerror(errno));
		    goto end_mouse_event;    
		}
		
		if ((buf[0] & 8) != 8) i--;
	    }
	    
	    kprintf("%02x: %02x: %02x\n", buf[0], buf[1], buf[2]);
	    /* Get button states */
	    newstate.buts[0] = (buf[0] & 0x01) ? 1 : 0;
	    newstate.buts[1] = (buf[0] & 0x02) ? 1 : 0;
	    newstate.buts[2] = (buf[0] & 0x04) ? 1 : 0;
	    
	    if (buf[1] != 0) {
		dx = (buf[0] & 0x10) ? buf[1] - 256 : buf[1];
	    }
	    
	    if (buf[2] != 0) {
		dy = (buf[0] & 0x20) ? buf[2] - 256 : buf[2];
	    }
		
	    newstate.dx = dx;
	    newstate.dy = -dy;
	    
	    kprintf("EVENT: STATE 1:%d, 2:%d, 3:%d, dx:%d, dy:%d\n"
		, newstate.buts[0], newstate.buts[1], newstate.buts[2]
		, newstate.dx, newstate.dy);
		
    	    mouse_event.x = newstate.dx;
	    mouse_event.y = newstate.dy;
	    
	    if (newstate.dx || newstate.dy)
	    {
	    	mouse_event.button = vHidd_Mouse_NoButton;
		mouse_event.type = vHidd_Mouse_Motion;
		
	    	HIDD_LinuxMouse_HandleEvent(lsd->mousehidd, &mouse_event);
	    }
		
	    if (newstate.buts[0] != oldstate.buts[0])
	    {
	    	mouse_event.button = vHidd_Mouse_Button1;
		mouse_event.type = newstate.buts[0] ? vHidd_Mouse_Press : vHidd_Mouse_Release;	
			
	    	HIDD_LinuxMouse_HandleEvent(lsd->mousehidd, &mouse_event);
	    }

	    if (newstate.buts[1] != oldstate.buts[1])
	    {
	    	mouse_event.button = vHidd_Mouse_Button2;
		mouse_event.type = newstate.buts[1] ? vHidd_Mouse_Press : vHidd_Mouse_Release;	
			
	    	HIDD_LinuxMouse_HandleEvent(lsd->mousehidd, &mouse_event);
	    }
	    
	    if (newstate.buts[2] != oldstate.buts[2])
	    {
	    	mouse_event.button = vHidd_Mouse_Button3;
		mouse_event.type = newstate.buts[2] ? vHidd_Mouse_Press : vHidd_Mouse_Release;	
			
	    	HIDD_LinuxMouse_HandleEvent(lsd->mousehidd, &mouse_event);
	    }
	    
    	    oldstate = newstate;
	    	    
#if 0
    	    ObtainSemaphore(&lsd->sema);
	    HIDD_LinuxMouse_HandleEvent(lsd->mousehidd, &mouse_event);
    	    ReleaseSemaphore(&lsd->sema);
#endif
	    
end_mouse_event:
	    free_unixio_message(mouse_port, lsd);
	}
    	
    } /* Forever */
    
failexit:

    if (NULL != kbd_port)
    	DeleteMsgPort(kbd_port);

    if (NULL != mouse_port)
	DeleteMsgPort(mouse_port);
	
    if (NULL != unixio)
    	OOP_DisposeObject((OOP_Object *)unixio);

    Signal(itp.parent, itp.fail_signal);
     
    return;
    
}

static struct Task *create_inputtask( struct inputtask_params *params, struct ExecBase *SysBase)
{
    struct Task *task;
    APTR stack;
    
    task = AllocMem(sizeof (struct Task), MEMF_PUBLIC|MEMF_CLEAR);
    if (NULL != task) {
    	NEWLIST(&task->tc_MemEntry);
    	task->tc_Node.ln_Type=NT_TASK;
    	task->tc_Node.ln_Name= INPUTTASK_NAME;
    	task->tc_Node.ln_Pri = INPUTTASK_PRIORITY;

    	stack=AllocMem(INPUTTASK_STACKSIZE, MEMF_PUBLIC);
    	if(NULL != stack) {
	    task->tc_SPLower=stack;
	    task->tc_SPUpper=(BYTE *)stack + INPUTTASK_STACKSIZE;

#if AROS_STACK_GROWS_DOWNWARDS
	    task->tc_SPReg = (BYTE *)task->tc_SPUpper-SP_OFFSET-sizeof(APTR);
	    ((APTR *)task->tc_SPUpper)[-1] = params;
#else
	    task->tc_SPReg=(BYTE *)task->tc_SPLower-SP_OFFSET + sizeof(APTR);
	    *(APTR *)task->tc_SPLower = params;
#endif
	    /* You have to clear signals first. */
	    SetSignal(0, params->ok_signal | params->fail_signal);

	    if(AddTask(task, inputtask_entry, NULL) != NULL)   {
	    	/* Everything went OK. Wait for task to initialize */
		ULONG sigset;
		
		kprintf("WAITING FOR SIGNAL\n");

		sigset = Wait( params->ok_signal | params->fail_signal );
		kprintf("GOT SIGNAL\n");
		if (sigset & params->ok_signal) {
		    return task;
		}
	    }	
	    FreeMem(stack, INPUTTASK_STACKSIZE);
    	}
        FreeMem(task,sizeof(struct Task));
    }
    return NULL;
}

struct Task *init_linuxinput_task(struct linux_staticdata *lsd)
{
    struct inputtask_params p;
    p.ok_signal = AllocSignal(-1L);
    p.fail_signal = AllocSignal(-1L);
    p.kill_signal = SIGBREAKF_CTRL_C;
    p.lsd = lsd;
    p.parent = FindTask(NULL);

kprintf("init_input_task: p.lsd = %p\n", p.lsd);
    
    kprintf("SIGNALS ALLOCATED\n");
    
    lsd->input_task = create_inputtask(&p, SysBase);
    kprintf("INPUTTASK CREATED\n");
    
    /* No need for these anymore */
    FreeSignal(p.ok_signal);
    FreeSignal(p.fail_signal);
    
    return lsd->input_task;
}

VOID kill_linuxinput_task(struct linux_staticdata *lsd)
{
#warning This will not work since the task does not Wait() for this signal.
#warning Also we should wait for the child task to exit
    Signal(lsd->input_task, SIGBREAKF_CTRL_C);
}
