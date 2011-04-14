#include <proto/arossupport.h>

IPTR LibGetTagData(Tag tagValue, IPTR defaultVal, const struct TagItem *tagList)
{
    struct TagItem *tag = LibFindTagItem(tagValue, tagList);
 
    return tag ? tag->ti_Data : defaultVal;
}
