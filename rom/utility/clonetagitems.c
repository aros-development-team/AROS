/*
    $Id$
    $Log$
    Revision 1.1  1996/10/22 04:45:59  aros
    Some more utility.library functions.

    Desc: CloneTagItems()
    Lang: english
*/
#include "utility_intern.h"

/*****************************************************************************

    NAME */
        #include <utility/tagitem.h>
        #include <clib/utility_protos.h>

        __AROS_LH1(struct TagItem *, CloneTagItems,

/*  SYNOPSIS */
        __AROS_LHA(struct TagItem *, tagList, A0),

/*  LOCATION */
        struct UtilityBase *, UtilityBase, 12, Utility)

/*  FUNCTION
        Duplicates a TagList

    INPUTS
        tagList     -   The TagList that you want to clone

    RESULT
        A TagList which contains a copy of the TagItems contained in the
        original list. The list is cloned so that calling FindTagItem()
        on a tag in the clone will return the same value as that in the
        original list (assuming the original has not been modified).

    NOTES
        If the original TagList is NULL, then no cloning will take place.

    EXAMPLE
        struct TagItem *tagList, *tagListClone;

        \* Set up the original taglist tagList *\

        tagListClone = CloneTagItems( tagList );

        \* Do what you want with your TagList here *\

        FreeTagItems( tagListClone );

    BUGS

    SEE ALSO
        <utility/tagitem.h>, AllocateTagItems(), FreeTagItems(),
        RefreshTagItemClones()

    INTERNALS

    HISTORY
        29-10-95    digulla automatically created from
                            utility_lib.fd and clib/utility_protos.h
        11-08-96    iaint   Implemented as AROS function.
        01-09-96    iaint   Updated the docs to give the same warnings
                            as the autodocs.
        05-09-96    iaint   Bit of optimisation (one variable :( )

*****************************************************************************/
{
    __AROS_FUNC_INIT

    struct TagItem *newList,
                   *ti;
    LONG numTags = 1;

    /* Make sure we actually have some valid input here... */
    if(!tagList)    return NULL;

    /*
        We start the counter at 1 since this count will not include the
        TAG_DONE TagItem

        newList is used here to save a variable. We don't need to do
        anything to the value of newList afterwards, since AllocateTagitems()
        will take care of setting it to NULL if the allocation fails.
    */
    newList = tagList;
    while(ti = NextTagItem(&newList))   numTags++;

    /*
        Then we shall allocate the TagList.
        If we can actually allocate a clone tag list, then we shall copy
        the tag values from one tag to another, the function
        "RefreshTagItemClones()" is used here to help re-use code.

        Of course we don't have to worry about the iff statement, since
        the original is guaranteed to have not been changed since
        CloneTagItems() :)
    */

    if( newList = AllocateTagItems( numTags ) )
        RefreshTagItemClones( newList, tagList );

    /* newList == 0 when allocation failed - AllocateTagItems handles this*/
    return newList;

    __AROS_FUNC_EXIT

} /* CloneTagItems */
