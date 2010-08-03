#include <aros/kernel.h>
#include <utility/tagitem.h>

#include <inttypes.h>
#include <string.h>

#include <kernel_tagitems.h>

struct TagItem *krnNextTagItem(struct TagItem **tagListPtr)
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

struct TagItem *krnFindTagItem(Tag tagValue, struct TagItem *tagList)
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
    struct TagItem *ti = 0;

    if (tagList && (ti = krnFindTagItem(tagValue, tagList)))
        return ti->ti_Data;

        return defaultVal;
}

struct KernelBSS
{
    void *addr;
    IPTR len;
};

void __clear_bss(struct TagItem *msg)
{
    struct KernelBSS *bss;

    bss = (struct KernelBSS *)krnGetTagData(KRN_KernelBss, 0, msg);
    
    if (bss)
    {
        while (bss->addr) {
	    bzero((void*)bss->addr, bss->len);
            bss++;
        }
    }
}
