/*
    Copyright � 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <inttypes.h>
#include <utility/tagitem.h>

#include "kernel_intern.h"

struct TagItem *krnNextTagItem(const struct TagItem **tagListPtr)
{
    if (!(*tagListPtr)) return 0;

    while(1)
    {
        switch((*tagListPtr)->ti_Tag)
        {
            case TAG_MORE:
                if (!((*tagListPtr) = (struct TagItem *)(*tagListPtr)->ti_Data))
                    return NULL;
                continue;
            case TAG_IGNORE:
                break;

            case TAG_END:
                (*tagListPtr) = 0;
                return NULL;

            case TAG_SKIP:
                (*tagListPtr) += (*tagListPtr)->ti_Data + 1;
                continue;

            default:
                return (struct TagItem *)(*tagListPtr)++;
        }

        (*tagListPtr)++;
    }
}

struct TagItem *krnFindTagItem(Tag tagValue, const struct TagItem *tagList)
{
    struct TagItem *tag;
    const struct TagItem *tagptr = tagList;

    while((tag = krnNextTagItem(&tagptr)))
    {
        if (tag->ti_Tag == tagValue)
            return tag;
    }

    return NULL;
}

intptr_t krnGetTagData(Tag tagValue, intptr_t defaultVal, const struct TagItem *tagList)
{
    struct TagItem *ti = 0;
    intptr_t value = defaultVal;

    if (tagList != NULL)
    {
        ti = krnFindTagItem(tagValue, tagList);
        if (ti != NULL)
            value = ti->ti_Data;
    }
    
    return value;
}

