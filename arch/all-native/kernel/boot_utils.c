/* Utility functions for boot taglist relocation */

#include <proto/arossupport.h>
#include <utility/tagitem.h>

#include <string.h>

#include "boot_utils.h"
#include "kernel_base.h"
#include "kernel_bootmem.h"

void RelocateBootMsg(const struct TagItem *msg)
{
    struct TagItem *dest;
    struct TagItem *tag;
    const struct TagItem *tstate = msg;
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
    while ((tag = LibNextTagItem(&msg)))
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
