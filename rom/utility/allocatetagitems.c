/*
    $Id$
    $Log$
    Revision 1.1  1996/08/31 12:58:11  aros
    Merged in/modified for FreeBSD.

    Desc: AllocateTagItems()
    Lang: english
*/
#include "utility_intern.h"

/*****************************************************************************

    NAME */
        #include <utility/tagitem.h>
        #include <clib/utility_protos.h>

        __AROS_LH1(struct TagItem *, AllocateTagItems,

/*  SYNOPSIS */
        __AROS_LHA(unsigned long, numTags, D0),

/*  LOCATION */
        struct UtilityBase *, UtilityBase, 11, Utility)

/*  FUNCTION
        Allocate a number of TagItems in an array for whatever you like.
        The memory allocated will be cleared.

    INPUTS
        numTags     - The number of TagItems to allocate.

    RESULT
        A pointer to an array of struct TagItem containing numTags tags.

    NOTES
        The number you supply must include the terminating tag (ie TAG_DONE)
        There is no provision for extra TagItems at the end of the list.

        If the number of tags to allocate is zero, then none will be.

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
        FreeTagsItems()

    INTERNALS

    HISTORY
        29-10-95    digulla automatically created from
                            utility_lib.fd and clib/utility_protos.h
        11-08-96    iaint   Moved code into the AROS source.

*****************************************************************************/
{
    __AROS_FUNC_INIT

    struct ExecBase *SysBase = UtilityBase->ub_SysBase;
    struct TagItem *tags = NULL;

    if( numTags )
        tags = AllocVec( numTags << 3, MEMF_CLEAR | MEMF_PUBLIC );

    return tags;

    __AROS_FUNC_EXIT

} /* AllocateTagItems */
