/*
    $Id$
    $Log$
    Revision 1.2  1996/09/12 14:52:47  digulla
    Better way to separate public and private parts of the library base

    Revision 1.1  1996/08/31 12:58:12  aros
    Merged in/modified for FreeBSD.

    Desc: FreeTagItems()
    Lang: english
*/
#include "utility_intern.h"

/*****************************************************************************

    NAME */
	#include <utility/tagitem.h>
	#include <clib/utility_protos.h>

	__AROS_LH1(void, FreeTagItems,

/*  SYNOPSIS */
	__AROS_LHA(struct TagItem *, tagList, A0),

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
	tagList[0].ti_Data = (ULONG)"A list of tags";
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
    __AROS_FUNC_INIT

    if( tagList )
	FreeVec( tagList );

    __AROS_FUNC_EXIT
} /* FreeTagItems */
