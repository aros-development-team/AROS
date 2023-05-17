#include <utility/tagitem.h>

IPTR krnGetTagData(Tag tagValue, IPTR defaultVal, const struct TagItem *tagList);
struct TagItem *krnFindTagItem(Tag tagValue, const struct TagItem *tagList);
struct TagItem *krnNextTagItem(const struct TagItem **tagListPtr);
