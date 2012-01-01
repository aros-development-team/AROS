#include <proto/arossupport.h>

struct TagItem *LibFindTagItem(Tag tagValue, const struct TagItem *tagList)
{
    struct TagItem *tstate = (struct TagItem *)tagList;
    struct TagItem *tag;

    while ((tag = LibNextTagItem(&tstate)))
    {
	if (tag->ti_Tag == tagValue)
	    return tag;
    }

    return NULL;
}
