/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <aros/asmcall.h>
#include <aros/atomic.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/alib.h>
#include <intuition/classes.h>
#include <utility/hooks.h>
#include <utility/utility.h>
#include "intuition_intern.h"

#define IntuitionBase   (GetPrivIBase(cl->cl_UserData))

#define ENABLE_MEM_POOL 1

#if ENABLE_MEM_POOL
#    define alloc(a, b)   AllocMem(b, MEMF_PUBLIC|MEMF_CLEAR)
#    define free(a, b, c) FreeMem(b, c)
#else
#    define alloc(a, b)   AllocPooled(a, b)
#    define free(a, b, c) FreePooled(a, b, c)
#endif


/*****i************************************************************************
 
    NAME */
AROS_UFH3(IPTR, rootDispatcher,

          /*  SYNOPSIS */
          AROS_UFHA(Class  *, cl,  A0),
          AROS_UFHA(Object *, o,   A2),
          AROS_UFHA(Msg,      msg, A1))

/*  FUNCTION
    internal !
 
    Processes all messages sent to the RootClass. Unknown messages are
    silently ignored.
 
    INPUTS
    cl - Pointer to the RootClass
    o - This object was the destination for the message in the first
        place
    msg - This is the message.
 
    RESULT
    Processes the message. The meaning of the result depends on the
    type of the message.
 
    NOTES
    This is a good place to debug BOOPSI objects since every message
    should eventually show up here.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
******************************************************************************/
{
    AROS_USERFUNC_INIT

    IPTR   retval = 0;
    Class *iclass;

    switch (msg->MethodID)
    {
	case OM_NEW:
            iclass = (Class *) o;
	    
            /* 
                Get memory for the instance data. The class knows how much is
                needed. NOTE: The object argument is actually the class!
            */
            
            o = (Object *) alloc
            (
                iclass->cl_MemoryPool, iclass->cl_ObjectSize
            );

            if (o)
            {
        	_OBJ(o)->o_Class = iclass;

        	AROS_ATOMIC_INC(iclass->cl_ObjectCount);

        	retval = (IPTR) BASEOBJECT(o);
            }
            break;

	case OM_DISPOSE:
            /* 
                Free memory. Caller is responsible that everything else
                is already cleared! 
            */
            iclass = OCLASS(o);

            free
            (
        	iclass->cl_MemoryPool, _OBJECT(o), iclass->cl_ObjectSize
            );
            
            AROS_ATOMIC_DEC(OCLASS(o)->cl_ObjectCount);
            break;

	case OM_ADDTAIL:
            /* Add <o> to list. */
            AddTail (((struct opAddTail *)msg)->opat_List, (struct Node *) _OBJECT(o));
            retval = TRUE;
            break;

	case OM_REMOVE:
            /* Remove object from list. */
            Remove ((struct Node *) _OBJECT(o));
            retval = TRUE;
            break;

	case OM_SET:
	case OM_GET:
	case OM_UPDATE:
	case OM_NOTIFY:
	case OM_ADDMEMBER:
	case OM_REMMEMBER:

	default:
            /* Ignore */
            break;

    } /* switch */

    return (retval);

    AROS_USERFUNC_EXIT

} /* rootDispatcher */
