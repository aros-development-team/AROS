#define AROS_ALMOST_COMPATIBLE 1

#include <proto/exec.h>
#include <exec/tasks.h>
#include <exec/lists.h>
#include <exec/memory.h>

#include "input_intern.h"

/***************************
**  GetEventsFromQueue()  **
***************************/
struct InputEvent *GetEventsFromQueue(struct inputbase *InputDevice)
{
    struct InputEvent *ie;

    ie = InputDevice->EventQueueHead;

    /* No more events in the queue */
    InputDevice->EventQueueHead = NULL;
    InputDevice->EventQueueTail = NULL;    
    
    return (ie);
}



/******************
**  AddEQTail()  **
******************/
/* Adds a chian of InputEvents to the eventqueue */
VOID AddEQTail(struct InputEvent *ie, struct inputbase *InputDevice)
{

    if (!InputDevice->EventQueueHead) /* Empty queue ? */
    {
    	InputDevice->EventQueueHead = ie;
    }
    else
    {
        InputDevice->EventQueueTail->ie_NextEvent = ie;
    }
    
    /* Get last event in eventchain to add */
    while (ie->ie_NextEvent)
    	ie = ie->ie_NextEvent;
    	
    InputDevice->EventQueueTail =  ie;
    
    return;
}

/************************
**  CreateInputTask()  **
************************/
struct Task *CreateInputTask(APTR taskparams, struct inputbase *InputDevice)
{
    struct Task *task;
    APTR stack;
    
    task = AllocMem(sizeof (struct Task), MEMF_PUBLIC|MEMF_CLEAR);
    if (task)
    {
    	NEWLIST(&task->tc_MemEntry);
    	task->tc_Node.ln_Type=NT_TASK;
    	task->tc_Node.ln_Name="input.device";
    	task->tc_Node.ln_Pri = IDTASK_PRIORITY;

    	stack=AllocMem(IDTASK_STACKSIZE, MEMF_PUBLIC);
    	if(stack != NULL)
    	{
	    task->tc_SPLower=stack;
	    task->tc_SPUpper=(BYTE *)stack + IDTASK_STACKSIZE;

#if AROS_STACK_GROWS_DOWNWARDS
	    task->tc_SPReg = (BYTE *)task->tc_SPUpper-SP_OFFSET - sizeof(APTR);
	    ((APTR *)task->tc_SPUpper)[-1] = taskparams;
#else
	    task->tc_SPReg=(BYTE *)task->tc_SPLower-SP_OFFSET + sizeof(APTR);
	    *(APTR *)task->tc_SPLower = taskparams;
#endif

	    if(AddTask(task, ProcessEvents, NULL) != NULL)
	    {
	    	/* Everything went OK */
	    	return (task);
	    }	
	    FreeMem(stack, IDTASK_STACKSIZE);
    	}
        FreeMem(task,sizeof(struct Task));
    }
    return (NULL);

}

/*********************
**  IsQualifierKey  **
*********************/
BOOL IsQualifierKey(UWORD key)
{
    BOOL result = FALSE;
    
    key &= ~IECODE_UP_PREFIX;
     
    if ((key >= 0x60) && (key <= 0x67))
    {
        result = TRUE;
    }
    
    return result;
}

/************************
**  IsKeyRepeatable()  **
************************/
BOOL IsRepeatableKey(UWORD key)
{
    BOOL result = TRUE;
    
    key &= ~IECODE_UP_PREFIX;
    
    /* stegerg: It looks like this is really all so no need to
       check the keymap->repeatable tables. I have checked this
       on the Amiga. All keys except the qualifier keys are
       treated as repeatable raw (!!!) keys. Here in input.device
       we have only raw keys. For vanilla keys there's MapRawKey()
       and this function thakes care of filtering out repeated
       raw keys if the corresponding KeyMap->repeatable table says
       that this key is not repeatable */
       
    if (IsQualifierKey(key))
    {
        result = FALSE;
    }
    
    return TRUE;
}
