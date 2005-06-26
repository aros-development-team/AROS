/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: FindNamedObject() - find a NamedObject in a given NameSpace.
    Lang: english
*/
#include <proto/exec.h>
#include "intern.h"

/*****************************************************************************

    NAME */
#include <utility/name.h>
#include <proto/utility.h>

	AROS_LH3(struct NamedObject *, FindNamedObject,

/*  SYNOPSIS */
	AROS_LHA(struct NamedObject *, nameSpace, A0),
	AROS_LHA(CONST_STRPTR        , name, A1),
	AROS_LHA(struct NamedObject *, lastObject, A2),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 40, Utility)

/*  FUNCTION
	This function will search through a given NameSpace, or the
	system global NameSpace to find a NamedObject with the name
	requested. Optionally you can have the search start from a
	specific NamedObject. This way you can look for each occurence
	of a specifically named NamedObject in a NameSpace that allows
	for duplicates.

    INPUTS
	nameSpace   -	The NameSpace to search through. If NULL will use
			the system default NameSpace.
	name	    -	The name of the object to search for. If NULL,
			any and all NamedObjects will be matched.
	lastObject  -	The (optional) last NamedObject to start the search
			from.

    RESULT
	If a NamedObject with the name supplied exists, it will be returned.
	Otherwise will return NULL.

	When you have finised with this NamedObject, you should call
	ReleaseNamedObject( NamedObject ).

    NOTES
	If you are going to use a returned NamedObject to be the starting
	point for another search you must call ReleaseNamedObject() AFTER
	searching, as the ReleaseNamedObject() call can cause the NamedObject
	to be freed, leaving you with an invalid pointer.

    EXAMPLE

    BUGS

    SEE ALSO
	ReleaseNamedObject()

    INTERNALS
	Could we implement named objects with hash chains perhaps?
	Possibly not as then NextObject handling would be quite tricky.

    HISTORY
	29-10-95    digulla automatically created from
			    utility_lib.fd and clib/utility_protos.h
	11-08-96    iaint   Wrote based on AmigaOS 3.0 function.
	07-02-97    iaint   Corrected handling of NULL name.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct NamedObject	    *foundObj = NULL;
    struct IntNamedObject   *no;
    struct Node 	    *StartObj;
    struct NameSpace	    *ns;

    ns = GetNameSpace( nameSpace, UtilityBase);
    ObtainSemaphore( &ns->ns_Lock );

    /* It is a bit stupid to do something with a NULL name */
    if( name )
    {
	/*
	    if the user supplied a lastObject, then we shall use that
	    to get the index of the starting object. Otherwise we shall
	    extract the address of the first node in the NameSpace.
	*/
	if(lastObject)
	    StartObj = (GetIntNamedObject(lastObject))->no_Node.ln_Succ;
	else
	    StartObj = (struct Node *)ns->ns_List.mlh_Head;

	if((no = IntFindNamedObj(ns, StartObj, name, UtilityBase)))
	{
	    no->no_UseCount++;
	    foundObj = GetNamedObject(no);
	}

    } /* if( name ) */
    else /* if(name == NULL) */
    {
	if(lastObject)
	    foundObj = (struct NamedObject *)(
		(GetIntNamedObject(lastObject))->no_Node.ln_Succ
	    );
	else
	    foundObj = (struct NamedObject *)ns->ns_List.mlh_Head;
    }
    ReleaseSemaphore( &ns->ns_Lock );

    return foundObj;

    AROS_LIBFUNC_EXIT

} /* FindNamedObject */
