extern struct TagItem *BootMsg;

struct TagItem *krnNextTagItem(struct TagItem **tagListPtr);
struct TagItem *krnFindTagItem(Tag tagValue, struct TagItem *tagList);
intptr_t krnGetTagData(Tag tagValue, intptr_t defaultVal, struct TagItem *tagList);
void __clear_bss(struct TagItem *msg);
