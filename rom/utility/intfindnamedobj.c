/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/

/*
    This function will start searching through a sublist of NamedObjects
    looking for the node which has the correct name (case insensitive).
    Case sensitive searches are done with FindName().

    The reason for the Utility_ prefixing the call is so that I can
    use a define IntFindNamedObj() that handles UtilityBase.
*/

#include "utility_intern.h"
#include <proto/exec.h>
#include <proto/utility.h>

struct IntNamedObject *
IntFindNamedObj(struct NameSpace *ns,
		struct Node *start,
		STRPTR name,
		struct UtilityBase *UtilityBase)
{
    struct IntNamedObject *no = NULL;

    if((ns->ns_Flags & NSF_CASE))
    {
	no = (struct IntNamedObject *)
	    FindName((struct List *)start->ln_Pred, name);
    }
    else
    {
	/* We are at the end of the list when ln_Succ == NULL */
	while(start->ln_Succ != NULL)
	{
	    if(!Stricmp(name, start->ln_Name))
	    {
		/* We have found the node we are after. */
		return GetIntNamedObject(start);
	    }
	    start = start->ln_Succ;
	}
    }

    return NULL;
}
