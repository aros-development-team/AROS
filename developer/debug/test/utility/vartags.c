/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Check that a tag list can be passed as a series of arguments to a
    varargs function.
*/

#include <proto/dos.h>
#include <proto/utility.h>

static const Tag tags[] =
    {ANO_NameSpace, ANO_UserSpace, ANO_Priority, ANO_Flags};

static struct TagItem *CloneTags(Tag tag1, ...)
{
    AROS_SLOWSTACKTAGS_PRE_AS(tag1, struct TagItem *)

    retval = CloneTagItems(AROS_SLOWSTACKTAGS_ARG(tag1));

    AROS_SLOWSTACKTAGS_POST
}

LONG main(void)
{
    BOOL success = TRUE;
    struct TagItem *tag_list, *tag_item, *temp_tag_list;
    UWORD i;

    /* Create a tag list from an argument list */

    temp_tag_list = tag_list = CloneTags((IPTR)ANO_NameSpace, (IPTR)TRUE,
        (IPTR)ANO_UserSpace, (IPTR)100, (IPTR)ANO_Priority, (IPTR)-127, (IPTR)ANO_Flags, (IPTR)0, (IPTR)TAG_END);

    /* Check that the returned tag list is correct */

    for (i = 0; (tag_item = NextTagItem(&temp_tag_list)) != NULL; i++)
    {
        if (i >= sizeof(tags) / sizeof(Tag))
        {
            Printf("Found extraneous tag 0x%lx!\n", tag_item->ti_Tag);
            success = FALSE;
        }
        else if (tag_item->ti_Tag == tags[i])
            Printf("Tag %d (0x%lx) found\n", i, tags[i]);
        else
        {
            Printf("Tag %d (0x%lx) missing! Found 0x%lx instead\n", i,
                tags[i], tag_item->ti_Tag);
            success = FALSE;
        }
    }

    FreeTagItems(tag_list);

    return success ? RETURN_OK : RETURN_ERROR;
}
