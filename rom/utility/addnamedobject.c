/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AddNamedObject() - adds a NamedObject to a given NameSpace.
    Lang: english
*/
#include <proto/exec.h>
#include "intern.h"

/*****************************************************************************

    NAME */
	#include <proto/utility.h>

	AROS_LH2(BOOL, AddNamedObject,

/*  SYNOPSIS */
	AROS_LHA(struct NamedObject *, nameSpace, A0),
	AROS_LHA(struct NamedObject *, object, A1),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 37, Utility)

/*  FUNCTION
	Adds a given NamedObject to a NameSpace which is addressed through
	a second NamedObject. Allows you to link a common group of
	NamedObjects together. If the NameSpace doesn't support duplicate
	names, then a search for a duplicate will be made, and FALSE returned
	if one is found.

    INPUTS
	nameSpace   -	The NameSpace to add the NamedObject object to.
			If this value is NULL, then the NamedObject will
			be added to the root NameSpace. This is useful
			for sharing NamedObjects between Tasks.
	object	    -	The NamedObject to add to the NameSpace.

    RESULT
	If the NamedObject can be added to either the supplied NameSpace or
	the system global NameSpace, this function will return TRUE.

	Otherwise it will return FALSE. This will generally happen when
	the NSF_NODUPS flag is set and this NamedObject has the same name
	as a second object, or when the object is already in a NameSpace.

    NOTES
	See BUGS.

    EXAMPLE

    BUGS
	Although the AmigaOS 3.1 autodocs did not say so, under 3.0 you
	couldn't add a NamedObject to a NameSpace when the NamedObject you
	were adding had a NameSpace itself. This has changed. This is
	because the autodocs did not say this, and they are right :)

    SEE ALSO
	utility/name.h, RemNamedObject()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    utility_lib.fd and clib/utility_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct NameSpace *ns;
    struct IntNamedObject  *no;  /* for object */
    BOOL		    ret; /* return value */

    /* Set to FALSE so lack of object returns FALSE */
    ret = FALSE;

    if(object)
    {
	/* Set true since now only failure will set as false */
	ret = TRUE;

	ns = GetNameSpace(nameSpace, UtilityBase);
	no = GetIntNamedObject(object);

	/* First of all, see if we are already in a NameSpace, if so, just
	    exit. This way we don't do any damage to any linked lists.
	*/
	if(no->no_ParentSpace)
	    return FALSE;

	ObtainSemaphore(&ns->ns_Lock);

	/*
	    By checking whether the Tail node exists, we check whether the
	    list has any nodes in it. If it doesn't then obviously we don't
	    have to check any other members of the list for name matches

	    Don't use FindName() here because the comparison may not be
	    case sensitive. IntFindNamedObj() does all that.
	*/
	if((ns->ns_Flags & NSF_NODUPS) && ns->ns_List.mlh_Tail)
	{
	    if(IntFindNamedObj(ns, (struct Node *)ns->ns_List.mlh_Head, no->no_Node.ln_Name, UtilityBase))
		ret = FALSE;
	    else
		Enqueue((struct List *)ns, (struct Node *)&no->no_Node);
	}
	else
	    Enqueue((struct List *)ns, (struct Node *)&no->no_Node);

	if(ret == TRUE) /* Added to NameSpace ns */
	    no->no_ParentSpace = ns;

	ReleaseSemaphore( &ns->ns_Lock );

    } /* if( object ) */

    return ret;

    AROS_LIBFUNC_EXIT

} /* AddNamedObject */
