/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: RemNamedObject() - Remove a NamedObject from a NameSpace.
    Lang: english
*/
#include <proto/exec.h>
#include "intern.h"

/*****************************************************************************

    NAME */
#include <utility/name.h>
#include <proto/utility.h>

	AROS_LH2(void, RemNamedObject,

/*  SYNOPSIS */
	AROS_LHA(struct NamedObject *, object, A0),
	AROS_LHA(struct Message     *, message, A1),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 44, Utility)

/*  FUNCTION
	Remove a NamedObject from a namespace.

	If the NamedObject cannot be removed at the time of this call, then
	the call will return without removing the NamedObject. It will
	mark the NamedObject as "waiting for removal".

	When the NamedObject is ready to be freed, the supplied message
	will be ReplyMsg()'d with the message->mn_Node.ln_Name field
	containing either:
	    - the address of the NamedObject that was removed. In this case
	      you can free the NamedObject yourself.
	    - NULL. In this case, another Task has freed the NamedObject,
	      and you should not do so.

    INPUTS
	object	    -	The NamedObject to attempt to remove.
	message     -	The message to send. This message is a standard
			Exec Message, which MUST have the mn_ReplyPort
			field correctly set. The mn_Node.ln_Name field
			will contain the address of the NamedObject or NULL
			upon arrival at your port.

    RESULT
	The NamedObject will be removed if possible, or marked for removal
	at the next best moment.

    NOTES
	Since this function effectively does a ReleaseNamedObject(), you
	must have found this object first.

    EXAMPLE

    BUGS

    SEE ALSO
	AttemptRemNamedObject(), AddNamedObject()

    INTERNALS
	AttemptRemNamedObject() calls this function with a NULL message,
	expecting that we remove the object if possible, or just return.

	ReleaseNamedObject() also calls this with a NULL message.

    HISTORY
	29-10-95    digulla automatically created from
			    utility_lib.fd and clib/utility_protos.h
	11-08-96    iaint   Adapted for AROS for stuff I did earlier.
	09-02-1997  iaint   Improved Message handling.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct NameSpace	   *ns;
    struct IntNamedObject *no;

    if(object)
    {
	no = GetIntNamedObject( object );
	ns = no->no_ParentSpace;

	/* If ns == 0, then this node hasn't been added to anywhere */
	if( ns )
	{
	    /* Whether we free now, or later, the message handling is
	       the same. So we use the same code.

	       The Forbid() is to prevent two tasks from trying to
	       attach a message at the same time.
	    */
	    Forbid();

	    if( message )
	    {
		if( no->no_FreeMessage == NULL)
		{
		    /* This is the message, mark for later */
		    message->mn_Node.ln_Name = (STRPTR)object;
		    no->no_FreeMessage = message;
		}
		else
		{
		    /* This message is not it, return NULL name. */
		    message->mn_Node.ln_Name = NULL;
		    ReplyMsg(message);
		}
	    }

	    Permit();

	    if(no->no_UseCount == 0)
	    {
		/* Ok, so we can remove the NamedObject */
		ObtainSemaphore( &ns->ns_Lock );

		Remove( (struct Node *)&no->no_Node );

		if(no->no_FreeMessage)
		{
		    ReplyMsg( no->no_FreeMessage );
		}
		ReleaseSemaphore( &ns->ns_Lock );
	    }

	} /* if ( ns ) */

    } /* if( object ) */

    AROS_LIBFUNC_EXIT

} /* RemNamedObject */
