/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AllocNamedObject() - allocate a NamedObject.
    Lang: english
*/
#include "intern.h"
#include <proto/exec.h>
#include <string.h>

/*****************************************************************************

    NAME */
#include <proto/utility.h>

	AROS_LH2(struct NamedObject *, AllocNamedObjectA,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR          , name, A0),
	AROS_LHA(CONST struct TagItem *, tagList, A1),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 38, Utility)

/*  FUNCTION
	Allocate a new NamedObject and initializes it as requested.
	This object can then be used as an object in a name space.
	Optionally you give this object a name space, and use it to
	nest name spaces. You can also allocate some memory which is
	attached to this object for your own personal use.

	When the object is allocated, it will automatically have one user.
	To allow other users to remove this object from a namespace, you
	must call ReleaseNamedObject() on this object.

    INPUTS
	name	-   The name of the NamedObject. Obviously this MUST be
		    specified (otherwise it wouldn't be named would it?)
	tagList -   A TagList containing some extra information for this
		    NamedObject. These are:

		    ANO_NameSpace: Allocate a NameSpace for this
			NamedObject. This will allow you to link other
			NamedObjects into a group. You cannot add a
			NamedObject with a NameSpace to another NameSpace.
			Boolean, default is FALSE.

		    ANO_UserSpace: This tag says that you want extra memory
			allocated for a UserSpace. The ti_Data field of
			this TagItem contains the amount of memory to
			allocate. Specifying this Tag with a ti_Data of 0,
			is equivalent to the default, which is no UserSpace.
			The UserSpace address can be found in the no_Object
			field of the NamedObject structure.

		    ANO_Priority: This is the List priority of the
			NamedObject and should be a signed BYTE value
			between -128 and 127. This is taken into account
			in adding and finding NamedObjects, as the highest
			priority NamedObject will be returned first. The
			default value is 0.

		    ANO_Flags: This allows you to initialize the value of
			the NameSpace flags which control certain aspects
			of the NameSpace. See the file utility/name.h.

    RESULT
	A pointer to a new NamedObject, or NULL if the allocation failed
	due to no free memory.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	FreeNamedObject()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    utility_lib.fd and clib/utility_protos.h
	11-08-96    iaint   Reworked for AROS.
	08-10-96    iaint   Changed to three memory areas after discussion
			    in AROS-DEV today.
	18-10-96    iaint   Completely rewrote.
	04-02-97    iaint   Updated documentation.
	16-04-01    iaint   Combined the memory for the IntNamedObject and
			    the name as an optimisation.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct IntNamedObject	*no  = NULL;
    /*
	This is the size of the required sections of the NamedObject.
    */
    LONG			size = 0;

    if(name)
    {
	no = AllocVec
	(
	    sizeof(struct IntNamedObject) + strlen(name) + 1,
	    MEMF_CLEAR|MEMF_PUBLIC
	);

	if (no == NULL)
	    return NULL;

	/* The name is at the first byte after the IntNamedObject struct */
	no->no_Node.ln_Name = (STRPTR)(no + 1);
	strcpy(no->no_Node.ln_Name, name);

	no->no_Node.ln_Pri = GetTagData( ANO_Priority, 0, tagList );
	no->no_UseCount = 0;
	no->no_FreeObject = FALSE;

	/* Find out if we need a NameSpace. */
	if(GetTagData(ANO_NameSpace, FALSE, tagList))
	{
	    no->no_NameSpace = AllocMem(sizeof(struct NameSpace), MEMF_CLEAR|MEMF_PUBLIC);
	    if(no->no_NameSpace != NULL)
	    {
		no->no_NameSpace->ns_Flags = GetTagData(ANO_Flags, 0, tagList);
		InitSemaphore(&no->no_NameSpace->ns_Lock);
		NEWLIST((struct List *)&no->no_NameSpace->ns_List);
	    }
	    else
	    {
		FreeNamedObject(GetNamedObject(no));
		return FALSE;
	    }
	}

	/* Set up the UserSpace. Maybe in the future we will be able to
	   have a UserSpace who has a different memory type to the
	   NamedObject.
	*/

	if((size = GetTagData(ANO_UserSpace, 0, tagList)))
	{
	    GetNamedObject(no)->no_Object = AllocVec(size, MEMF_CLEAR|MEMF_PUBLIC);
	    if(no->no.no_Object == NULL)
	    {
		FreeNamedObject(GetNamedObject(no));
		return NULL;
	    }
	    else
	    {
		/* We should free the object in FreeNamedObject(). */
		no->no_FreeObject = TRUE;
	    }
	}
	else
	{
	    /* Don't free the object, we didn't allocate it. */
	    no->no_FreeObject = FALSE;
	    GetNamedObject(no)->no_Object = NULL;
	}

	no->no_UseCount = 1;
	return GetNamedObject(no);

    } /* if ( name ) */

    return NULL;

    AROS_LIBFUNC_EXIT

} /* AllocNamedObjectA */
