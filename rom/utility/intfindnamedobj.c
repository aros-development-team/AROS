/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Find a NamedObject in a NameSpace.
    Lang: English

    --------------------------------------------------------------------

    This function will start searching through a sublist of NamedObjects
    looking for the node which has the correct name (case insensitive).
    Case sensitive searches are done with FindName().

*/

#include "intern.h"
#include <proto/exec.h>
#include <proto/utility.h>

struct IntNamedObject *
IntFindNamedObj(struct NameSpace *ns,
		struct Node *start,
		CONST_STRPTR name,
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
		/*
		    We have found the node we are after.
		    Note, we actually get the correct address
		    later.
		*/
		no = (struct IntNamedObject *)start;
	    }
	    start = start->ln_Succ;
	}
    }

    /*
	This is safe, since the Node occurs just after the public
	part of the NamedObject, which is what we return.
    */
    if(no)
	return (struct IntNamedObject *)((struct NamedObject *)no - 1);
    else
	return NULL;
}
