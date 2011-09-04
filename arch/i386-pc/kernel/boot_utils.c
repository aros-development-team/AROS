/*
 * This code is a candidate for arch/all-native/kernel
 */

#include <proto/arossupport.h>
#include <utility/tagitem.h>

#include <string.h>

#include "boot_utils.h"
#include "kernel_base.h"
#include "kernel_bootmem.h"

void *RelocateBootMsg(const struct TagItem *msg, struct TagItem *dest)
{
    struct TagItem *tag;

    BootMsg = dest;

    while ((tag = LibNextTagItem(&msg)))
    {
    	dest->ti_Tag  = tag->ti_Tag;
    	dest->ti_Data = tag->ti_Data;
    	dest++;
    }
    dest->ti_Tag = TAG_DONE;
    dest++;

    return dest;
}

void RelocateTagData(struct TagItem *tag, unsigned long size)
{
    char *src = (char *)tag->ti_Data;
    unsigned char *dest = krnAllocBootMem(size);
    unsigned int i;

    tag->ti_Data = (IPTR)dest;

    /* Do not use memcpy() because it can rely on CopyMem() which is not available yet */
    for (i = 0; i < size; i++)
        *dest++ = *src++;
}

void RelocateStringData(struct TagItem *tag)
{
    unsigned int l = strlen((char *)tag->ti_Data) + 1;

    RelocateTagData(tag, l);
}

void RelocateBSSData(struct TagItem *tag)
{
    struct KernelBSS *bss;
    unsigned int l = sizeof(struct KernelBSS);

    for (bss = (struct KernelBSS *)tag->ti_Data; bss->addr; bss++)
    	l += sizeof(struct KernelBSS);

    RelocateTagData(tag, l);
}
