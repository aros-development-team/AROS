#define AROS_ALMOST_COMPATIBLE 1
#include <proto/exec.h>
#include <exec/lists.h>
#include <exec/interrupts.h>
#include <devices/inputevent.h>
#include <intuition/intuition.h>
#include <aros/asmcall.h>

#include "input_intern.h"

#define DEBUG 0
#include <aros/debug.h>

extern struct inputbase *IBase;
#define InputDevice IBase
extern void intui_WaitEvent(struct InputEvent *, struct Window **);


/********************************
** Input device main function  **
********************************/
void ProcessEvents (void)
{

    int i;
    struct Window *w;
    struct List *handlerlist = (struct List *)&(InputDevice->HandlerList);
    struct SignalSemaphore *handlersema = &(InputDevice->HandlerSema);
    struct InputEvent ie = {0,};

    
    /* We must initialize the intuition input handler from
    ** inside the input.device task, so that the intuition
    ** IDCMP replyport gets the right task as owner.
    */
    
    D(bug("ProcessEvents: initializing intuition input handler\n"));
    InputDevice->IntuiInputHandler = InitIIH(InputDevice);
    
    /* Now, what on earth do we do if this fails ? Do an Alert() ?? 
    */
    if (!InputDevice->IntuiInputHandler)
    	;

    	
    /* Add the intuition inputhandler to th handler list */
    ObtainSemaphore( handlersema );
    AddTail( handlerlist, (struct Node *)InputDevice->IntuiInputHandler);
    ReleaseSemaphore( handlersema );
    
    D(bug("ProcessEvents: Going into infinite loop\n"));

    

    for (;;)
    {
	struct Interrupt *ihiterator;

	D(bug("ipe : waiting for event\n"));
	intui_WaitEvent(&ie, &w);
	D(bug("ipe: Got event of class %d for window %s\n",
		ie.ie_Class, w->Title));
		
	/* Arbitrate for handler list */
	ObtainSemaphore( handlersema );
		
	ForeachNode(handlerlist, ihiterator)
	{
	    D(bug("ipe: calling inputhandler %s at %p\n",
		ihiterator->is_Node.ln_Name, ihiterator->is_Code));
		
	    AROS_UFC3(struct InputEvent *, ihiterator->is_Code,
		AROS_UFCA(struct InputEvent *,  &ie,            	A0),
		AROS_UFCA(APTR,                 ihiterator->is_Data,    A1),
		AROS_UFCA(struct Window *,      w,              	A2));
	    D(bug("ipe: returned from inputhandler\n"));
	}

	ReleaseSemaphore( handlersema );
	

    } /* for (;;) */
    
        
    for (;;) {i=i;}
   
} /* ProcessEvents */

