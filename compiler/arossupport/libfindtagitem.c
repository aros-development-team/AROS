/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.
*/

#include <proto/arossupport.h>

struct TagItem *LibFindTagItem(Tag tagValue, const struct TagItem *tagList)
{
    struct TagItem *tstate = (struct TagItem *)tagList;
    struct TagItem *tag;

    while ((tag = LibNextTagItem(&tstate)))
    {
        if ((ULONG)tag->ti_Tag == (ULONG)tagValue)
            return tag;
    }

    return NULL;
}
