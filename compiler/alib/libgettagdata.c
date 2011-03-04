#include <proto/alib.h>

IPTR LibGetTagData(Tag tagValue, IPTR defaultVal, struct TagItem *tagList)
{
    struct TagItem *tag = LibFindTagItem(tagValue, tagList);
 
    return tag ? tag->ti_Data : defaultVal;
}
