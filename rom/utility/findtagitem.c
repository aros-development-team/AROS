/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.8  1997/02/17 02:17:49  ldp
    Implement using NextTagItem() instead of local system tag handling.

    Revision 1.7  1997/02/14 21:19:42  ldp
    Add extra checks for empty arguments

    Revision 1.6  1997/01/27 00:32:30  ldp
    Polish

    Revision 1.5  1996/12/10 14:00:13  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.4  1996/10/24 15:51:35  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.3  1996/08/13 14:10:30  digulla
    Replaced AROS_LA by AROS_LHA

    Revision 1.2  1996/08/01 17:41:41  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/types.h>
#include <utility/utility.h>
#include <utility/tagitem.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
#include <proto/utility.h>

	AROS_LH2(struct TagItem *, FindTagItem,

/*  SYNOPSIS */
	AROS_LHA(Tag,              tagValue, D0),
	AROS_LHA(struct TagItem *, tagList,  A0),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 5, Utility)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tagptr, *tag;

    tagptr = tagList;

    while( (tag = NextTagItem(&tagptr)) )
    {
	if(tag->ti_Tag == tagValue) return tag;
    }

    return NULL;

    AROS_LIBFUNC_EXIT
} /* FindTagItem */
