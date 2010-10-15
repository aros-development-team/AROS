/*
 * Kernel's own implementation of TagItem utility functions.
 * It is needed because kernel.resource is initialized long
 * before utility.library and therefore can't use it.
 */
struct TagItem *krnNextTagItem(struct TagItem **tagListPtr);
intptr_t krnGetTagData(Tag tagValue, intptr_t defaultVal, struct TagItem *tagList);
