/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1997/03/17 18:56:25  srittau
    Fixed some typos in FUNCTION description.

    Revision 1.6  1997/01/27 00:36:40  ldp
    Polish

    Revision 1.5  1996/12/10 14:00:05  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.4  1996/11/08 11:28:03  aros
    All OS function use now Amiga types

    Moved intuition-driver protos to intuition_intern.h

    Revision 1.3  1996/10/24 15:51:22  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.2  1996/10/23 16:30:09  aros
    Ooops.. PublicClassList is a MinNode list :-)

    Revision 1.1  1996/08/28 17:55:35  digulla
    Proportional gadgets
    BOOPSI


    Desc:
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
#include <exec/lists.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <intuition/classes.h>
#include <proto/intuition.h>

	AROS_LH5(struct IClass *, MakeClass,

/*  SYNOPSIS */
	AROS_LHA(UBYTE         *, classID, A0),
	AROS_LHA(UBYTE         *, superClassID, A1),
	AROS_LHA(struct IClass *, superClassPtr, A2),
	AROS_LHA(ULONG          , instanceSize, D0),
	AROS_LHA(ULONG          , flags, D1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 113, Intuition)

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
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    Class * iclass;

    /* trust the user ;-) */
    if (!superClassID && !superClassPtr)
	return (NULL);

    /* Does this class already exist ? */
    if (FindName (PublicClassList, classID))
	return (NULL);

    /* Has the user specified a classPtr ? */
    if (!superClassPtr)
    {
	/* Search for the class ... */
	for (superClassPtr=GetHead(PublicClassList);
	    superClassPtr;
	    superClassPtr=GetSucc(superClassPtr)
	)
	{
	    if (!strcmp(superClassPtr->cl_ID, superClassID))
		break;
	}

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
