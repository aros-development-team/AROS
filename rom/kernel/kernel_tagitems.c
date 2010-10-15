#include <aros/kernel.h>
#include <utility/tagitem.h>

#include <inttypes.h>

#include <kernel_tagitems.h>

struct TagItem *krnNextTagItem(const struct TagItem **tagListPtr)
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

struct TagItem *krnFindTagItem(Tag tagValue, const struct TagItem *tagList)
{
    struct TagItem *tag;
    const struct TagItem *tagptr = tagList;

    while((tag = krnNextTagItem(&tagptr)))
    {
        if (tag->ti_Tag == tagValue)
            return tag;
    }

    return 0;
}

intptr_t krnGetTagData(Tag tagValue, intptr_t defaultVal, struct TagItem *tagList)
{
    const struct TagItem *tstate = tagList;
    struct TagItem *tag;
    
    while ((tag = krnNextTagItem(&tstate)))
    {
	if (tag->ti_Tag == tagValue)
	    return tag->ti_Data;
    }
    
    return defaultVal;
}

void krnSetTagData(Tag tagValue, intptr_t newtagValue, struct TagItem *tagList)
{
    struct TagItem *tag = 0;

    if (tagList && ((tag = krnFindTagItem(tagValue, tagList)) != 0))
        tag->ti_Data = newtagValue;
    
    return;
}
