/* Utility functions for boot taglist relocation */

#include <proto/arossupport.h>
#include <utility/tagitem.h>

#include <string.h>

#include "boot_utils.h"
#include "kernel_base.h"
#include "kernel_bootmem.h"

/* Our own block copier, because memcpy() can rely on CopyMem() which is not available yet */
void krnCopyMem(const void *src, void *dest, unsigned long size)
{
    const char *s = src;
    char *d = dest;
    unsigned long i;

    for (i = 0; i < size; i++)
        *d++ = *s++;
}

void RelocateBootMsg(const struct TagItem *msg)
{
    struct TagItem *dest;
    struct TagItem *tag;
    struct TagItem *tstate = (struct TagItem *)msg;
    ULONG num = 1;

    /* First count how much memory we will need */
    while ((tag = LibNextTagItem(&tstate)))
    {
    	num++;
    }

    /* Allocate the memory */
    dest = krnAllocBootMem(num * sizeof(struct TagItem));
    BootMsg = dest;

    /* Now copy tagitems */
    tstate = (struct TagItem *)msg;
    while ((tag = LibNextTagItem(&tstate)))
    {
    	dest->ti_Tag  = tag->ti_Tag;
    	dest->ti_Data = tag->ti_Data;
    	dest++;
    }

    /* Make sure the list is terminated */
    dest->ti_Tag = TAG_DONE;
}

void RelocateTagData(struct TagItem *tag, unsigned long size)
{
    char *src = (char *)tag->ti_Data;
    unsigned char *dest = krnAllocBootMem(size);

    tag->ti_Data = (IPTR)dest;
    krnCopyMem(src, dest, size);
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
