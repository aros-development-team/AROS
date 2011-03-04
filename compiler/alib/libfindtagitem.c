#include <proto/alib.h>

struct TagItem *LibFindTagItem(Tag tagValue, struct TagItem *tagList)
{
    const struct TagItem *tstate = tagList;
    struct TagItem *tag;

    while ((tag = LibNextTagItem(&tstate)))
    {
	if (tag->ti_Tag == tagValue)
	    return tag;
    }

    return NULL;
}
