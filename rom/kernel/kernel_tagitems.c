#include <aros/kernel.h>
#include <utility/tagitem.h>

#include <inttypes.h>

#include <kernel_tagitems.h>

struct TagItem *krnNextTagItem(struct TagItem **tagListPtr)
{
    if (!(*tagListPtr))
	return NULL;

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

intptr_t krnGetTagData(Tag tagValue, intptr_t defaultVal, struct TagItem *tagList)
{
    struct TagItem *tstate = tagList;
    struct TagItem *tag;
    
    while ((tag = krnNextTagItem(&tstate)))
    {
	if (tag->ti_Tag == tagValue)
	    return tag->ti_Data;
    }
    
    return defaultVal;
}
