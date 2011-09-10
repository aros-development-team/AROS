#include <utility/tagitem.h>

void krnCopyMem(const void *src, void *dest, unsigned long size);

void RelocateBootMsg(const struct TagItem *msg);
void RelocateTagData(struct TagItem *tag, unsigned long size);
void RelocateStringData(struct TagItem *tag);
void RelocateBSSData(struct TagItem *tag);
