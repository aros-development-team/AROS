/*
    $Id$
    $Log$
    Revision 1.7  1997/01/27 13:17:13  digulla
    Added #include <proto/exec.h>

    Revision 1.6  1997/01/27 00:32:31  ldp
    Polish

    Revision 1.5  1996/12/10 14:00:13  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.4  1996/10/24 15:51:35  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.3  1996/09/13 17:36:37  digulla
    Use IPTR

    Revision 1.2  1996/09/12 14:52:47  digulla
    Better way to separate public and private parts of the library base

    Revision 1.1  1996/08/31 12:58:12  aros
    Merged in/modified for FreeBSD.

    Desc: FreeTagItems()
    Lang: english
*/
#include <proto/exec.h>
#include "utility_intern.h"

/*****************************************************************************

    NAME */
#include <utility/tagitem.h>
#include <proto/utility.h>

	AROS_LH1(void, FreeTagItems,

/*  SYNOPSIS */
	AROS_LHA(struct TagItem *, tagList, A0),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 13, Utility)

/*  FUNCTION
	Free a list of TagItems which was allocated by AllocateTagItems().

    INPUTS
	tagList     - A list of TagItems - must have been allocated by
		      AllocateTagItems() or CloneTagItems().

    RESULT
	The memory containing the tagList is returned to the system.

    NOTES
	The memory will only be freed if the input is non-NULL.

    EXAMPLE
	struct TagItem *tagList;

	tagList =  AllocateTagItems( 4 );

	tagList[0].ti_Tag  = NA_Name;
	tagList[0].ti_Data = (IPTR)"A list of tags";
	tagList[3].ti_Tag  = TAG_DONE;

	\* Do what you want with your TagList here ... *\

	FreeTagItems( tagList );

    BUGS

    SEE ALSO
	utility/tagitem.h, utility/AllocateTagItems()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    utility_lib.fd and clib/utility_protos.h
	11-08-96    iaint   Moved into AROS source tree.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if( tagList )
	FreeVec( tagList );

    AROS_LIBFUNC_EXIT
} /* FreeTagItems */
