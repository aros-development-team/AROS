/*
    $Id$
    $Log$
    Revision 1.2  1997/01/27 00:32:32  ldp
    Polish

    Revision 1.1  1996/12/18 01:27:36  iaint
    NamedObjects

    Desc: RemNamedObject() - Remove a NamedObject from a NameSpace.
    Lang: english
*/
#include "utility_intern.h"

/*****************************************************************************

    NAME */
        #include <proto/utility.h>

        AROS_LH2(void, RemNamedObject,

/*  SYNOPSIS */
        AROS_LHA(struct NamedObject *, object, A0),
        AROS_LHA(struct Message     *, message, A1),

/*  LOCATION */
        struct UtilityBase *, UtilityBase, 44, Utility)

/*  FUNCTION
        Removes a NamedObject from a NameSpace. If the NamedObject cannot
        be removed at the time of this call, then the call will return
        without removing the NamedObject. However it will mark the
        NamedObject as "waiting for removal", and when the number of users
        of a NamedObject = 0, it will ReplyMsg() the message which is
        supplied as a parameter. When the message is replied it will have
        either the address of the object if it was removed, or NULL if it
        was not removed, or removed by another method stored in the
        message->mn_Node.ln_Name field of the message structure.

    INPUTS
        object      -   The NamedObject to attempt to remove.
        message     -   The message to send. This message is a standard
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
        utility/name.h, AttemptRemNamedObject(), AddNamedObject()

    INTERNALS
        AttemptRemNamedObject() calls this function with a NULL message,
        expecting that we remove the object if possible, or just return.

    HISTORY
        29-10-95    digulla automatically created from
                            utility_lib.fd and clib/utility_protos.h
        11-08-96    iaint   Adapted for AROS for stuff I did earlier.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct NameSpace       *ns;
    struct IntNamedObject *no;

    if(object)
    {
        Forbid();

        no = GetIntNamedObject( object );
        ns = no->no_ParentSpace;

        /* If ns == 0, then this node hasn't been added to anywhere */
        if( ns )
        {
            /* If the UseCount > 1, then we can't remove at the moment.
               It must be greater than 1, since the user should have
               called FindNamedObject() on this object first.
            */
            if( no->no_UseCount > 1)
            {
                /* If we have a message, attach it, otherwise return */
                if( message )
                {
                    message->mn_Node.ln_Name = (STRPTR)object;
                    no->no_FreeMessage = message;
                }

                Permit();
                return;
            }

            /* Ok, so we can remove the NamedObject */
            ObtainSemaphore( &ns->ns_Lock );

            Remove( (struct Node *)no );
            no->no_UseCount = 0;

            if( message )
            {
                message->mn_Node.ln_Name = (STRPTR)object;
                no->no_FreeMessage = message;
                ReplyMsg( message );
            }
            ReleaseSemaphore( &ns->ns_Lock );

        } /* if ( ns ) */

        Permit();

    } /* if( object ) */

    AROS_LIBFUNC_EXIT

} /* RemNamedObject */
