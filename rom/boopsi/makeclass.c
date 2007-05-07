/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Initialize a BOOPSI class
    Lang: english
*/
#include <exec/lists.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include "intern.h"

/*****************************************************************************

    NAME */
#include <intuition/classes.h>
#include <proto/boopsi.h>

	AROS_LH5(struct IClass *, MakeClass,

/*  SYNOPSIS */
	AROS_LHA(UBYTE         *, classID, A0),
	AROS_LHA(UBYTE         *, superClassID, A1),
	AROS_LHA(struct IClass *, superClassPtr, A2),
	AROS_LHA(ULONG          , instanceSize, D0),
	AROS_LHA(ULONG          , flags, D1),

/*  LOCATION */
	struct Library *, BOOPSIBase, 10, BOOPSI)

/*  FUNCTION
	Only for class implementators.

	This function creates a new public BOOPSI class. The SuperClass
	should be another BOOPSI class; all BOOPSI classes are subclasses
	of the ROOTCLASS.

	SuperClasses can by private or public. You can specify a name/ID
	for the class if you want it to become a public class. For public
	classes, you must call AddClass() afterwards to make it public
	accessible.

	The return value contains a pointer to the IClass structure of your
	class. You must specify your dispatcher in cl_Dispatcher. You can
	also store shared data in cl_UserData.

	To get rid of the class, you must call FreeClass().

    INPUTS
	classID - NULL for private classes otherwise the name/ID of the
		public class.
	superClassID - Name/ID of a public SuperClass. NULL is you don't
		want to use a public SuperClass or if you have the pointer
		your SuperClass.
	superClassPtr - Pointer to the SuperClass. If this is non-NULL,
		then superClassID is ignored.
	instanceSize - The amount of memory which your objects need (in
		addition to the memory which is needed by the SuperClass(es))
	flags - For future extensions. To maintain comaptibility, use 0
		for now.

    RESULT
	Pointer to the new class or NULL if
	- There wasn't enough memory
	- The superclass couldn't be found
	- There already is a class with the same name/ID.

    NOTES
	No copy is made of classID. So make sure the lifetime of the contents
	of classID is at least the same as the lifetime of the class itself.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    Class * iclass;

    /* trust the user ;-) */
    if (!superClassID && !superClassPtr)
	return (NULL);

    /* Does this class already exist ? */
    if (FindClass (classID))
	return (NULL);

    /* Has the user specified a classPtr ? */
    if (!superClassPtr)
    {
	/* Search for the class ... */
	superClassPtr = FindClass (superClassID);

	if (!superClassPtr)
	    return (NULL);  /* nothing found */
    }

    /* Get some memory */
    if ((iclass = (Class *) AllocMem (sizeof (Class), MEMF_PUBLIC|MEMF_CLEAR)))
    {
	/* Felder init */
	iclass->cl_Super      = superClassPtr;
	iclass->cl_ID	      = classID;
	iclass->cl_InstOffset = superClassPtr->cl_InstOffset +
				superClassPtr->cl_InstSize;
	iclass->cl_InstSize   = instanceSize;
	iclass->cl_Flags      = flags;

	/* SuperClass is used one more time now */
	superClassPtr->cl_SubclassCount ++;
    }

    return (iclass);
    AROS_LIBFUNC_EXIT
} /* MakeClass */
