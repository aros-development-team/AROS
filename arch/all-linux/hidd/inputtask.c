/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: Task used for wainting on events from linux
    Lang: English.
*/

#define AROS_ALMOST_COMPATIBLE 1

#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/arossupport.h>

#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/lists.h>

#include <dos/dos.h>

#include <oop/oop.h>
#include <hidd/unixio.h>

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

#undef OOPBase
#define OOPBase lsd->oopbase
static VOID inputtask_entry(struct inputtask_params *inputparams)
{
    struct linux_staticdata *lsd;
    struct inputtask_params itp;
    
    struct MsgPort *unixio_port = NULL;
    HIDD *unixio = NULL;
    IPTR ret;
    /* We must copy the parameter struct because they are allocated
     on the parent's stack */
kprintf("INSIDE INPUT TASK\n");
    itp = *inputparams;
    lsd = itp.lsd;

kprintf("CREATING UNIXIO, OOPBase=%p, %s\n", lsd->oopbase->lib_Node.ln_Name);
    unixio = (HIDD)New_UnixIO(lsd->oopbase);
kprintf("UNIXIO %p\n", unixio);
    if (NULL == unixio) {
    	goto failexit;
    }

    for (;;) {
	kprintf("GETTING INPUT FROM UNIXIO\n");
	/* Turn on kbd support */
//	init_kbd(lsd);
    	ret = (int)Hidd_UnixIO_Wait( unixio, lsd->kbdfd, vHidd_UnixIO_Read, NULL, NULL);
//	cleanup_kbd(lsd);
	kprintf("GOT INPUT FROM UNIXIO\n");
	
 	for (;;) {
	    char code;
	    
	    break;
	    
	    if (-1 == read(lsd->kbdfd, &code, 1)) {
	    	kprintf("!!! COULD NOT READ FROM LINUX KBD DEVICE: %s\n"
			, strerror(errno));
	    } else {
	    	/* Let the kbd hidd handle it */
	    }

	}
    	
    } /* Forever */
    
failexit:

    if (NULL != unixio_port)
    	DeleteMsgPort(unixio_port);
	
    if (NULL != unixio)
    	DisposeObject((Object *)unixio);

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

struct Task *init_input_task(struct linux_staticdata *lsd)
{
    struct inputtask_params p;
    p.ok_signal = AllocSignal(-1L);
    p.fail_signal = AllocSignal(-1L);
    p.kill_signal = SIGBREAKF_CTRL_C;
    p.lsd = lsd;
    p.parent = FindTask(NULL);
    
    kprintf("SIGNALS ALLOCATED\n");
    
    lsd->input_task = create_inputtask(&p, SysBase);
    kprintf("INPUTTASK CREATED\n");
    
    /* No need for these anymore */
    FreeSignal(p.ok_signal);
    FreeSignal(p.fail_signal);
    
    return lsd->input_task;
}

VOID kill_input_task(struct linux_staticdata *lsd)
{
#warning This will not work since the task does not Wait() for this signal.
#warning Also we should wait for the child task to exit
    Signal(lsd->input_task, SIGBREAKF_CTRL_C);
}
