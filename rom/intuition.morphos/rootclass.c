/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <exec/lists.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/alib.h>
//#include <proto/arossupport.h>
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#ifndef UTILITY_HOOKS_H
#   include <utility/hooks.h>
#endif
#include <utility/utility.h>
#include <aros/asmcall.h>
#include "intuition_intern.h"

#define IntuitionBase   (GetPrivIBase(cl->cl_UserData))

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
 
    HISTORY
    14.09.93    ada created
 
******************************************************************************/
{
    AROS_USERFUNC_INIT

    IPTR  retval = 0;
    Class *objcl;

    switch (msg->MethodID)
    {
    case OM_NEW:
        objcl = (Class *)o;

        /* Get memory. The objects shows how much is needed.
           (The object is not an object, it is a class pointer!) */
        o = (Object *) AllocMem (objcl->cl_InstOffset +
                                 objcl->cl_InstSize +
                                 sizeof (struct _Object),
                                 MEMF_ANY|MEMF_CLEAR);

        if (o)
        {
            _OBJ(o)->o_Class = objcl;
            
            #warning "Use atomic macro for this once we have some"
            Forbid();
            objcl->cl_ObjectCount++;
            Permit();
            
            retval = (IPTR) BASEOBJECT(o);
        }
        break;

    case OM_DISPOSE:
        /* Free memory. Caller is responsible that everything else
           is already cleared! */
        objcl = OCLASS(o);
        #warning "Use atomic macro for this once we have some"
        Forbid();
        OCLASS(o)->cl_ObjectCount--;
        Permit();
        FreeMem (_OBJECT(o),
                 objcl->cl_InstOffset + objcl->cl_InstSize + sizeof (struct _Object));
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
