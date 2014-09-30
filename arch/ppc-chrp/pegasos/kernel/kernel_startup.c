/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <aros/macros.h>
#include <aros/multiboot.h>
#include <asm/ppc/ppc740.h>
#include <hardware/openfirmware.h>
#include <proto/arossupport.h>

#include <bootconsole.h>
#include <inttypes.h>
#include <string.h>

#include "kernel_base.h"
#include "kernel_bootmem.h"
#include "kernel_debug.h"
#include "kernel_intern.h"

#define D(x) x

static char boot_stack[];
static void kernel_start(const struct TagItem *bootMsg);

int __startup kernel_entry(struct TagItem *bootMsg, ULONG magic)
{
    if (magic == AROS_BOOT_MAGIC)
    {
	core_kick(bootMsg, kernel_start);
    }
    return -1;
}

void core_kick(struct TagItem *bootMsg, void *target)
{
    const struct TagItem *bss = LibFindTagItem(KRN_KernelBss, bootMsg);

    /* First clear .bss */
    if (bss)
    	__clear_bss((const struct KernelBSS *)bss->ti_Data);

    asm volatile("mr %%r1, %1; bctr"::"c"(target),"r"(boot_stack + STACK_SIZE - SP_OFFSET));
}

/* Own memory copy routine because memcpy() can rely on CopyMem() which is not available yet */
static inline void __boot_memcpy(unsigned char *dest, unsigned char *src, unsigned int size)
{
    unsigned int i;

    for (i = 0; i < size; i++)
        *dest++ = *src++;
}

static void RelocateTagData(struct TagItem *tag, unsigned long size)
{
    unsigned char *src = (unsigned char *)tag->ti_Data;
    unsigned char *dest = krnAllocBootMem(size);

    tag->ti_Data = (IPTR)dest;
    __boot_memcpy(dest, src, size);
}

static struct OFWNode *krnCopyOFWTree(struct OFWNode *orig)
{
    uint32_t node_length = sizeof(struct OFWNode) + strlen(orig->on_name) + 1;
    struct OFWNode *new_node = krnAllocBootMem(node_length);
    struct OFWNode *child;
    struct OFWProperty *prop, *new_prop;

    new_node->on_name = new_node->on_storage;

    __boot_memcpy(new_node->on_name, orig->on_name, strlen(orig->on_name) + 1);
    NEWLIST(&new_node->on_children);
    NEWLIST(&new_node->on_properties);

    ForeachNode(&orig->on_properties, prop)
    {
	uint32_t prop_name_length = strlen(prop->op_name) + 1;
	uint32_t prop_length = sizeof(struct OFWProperty) + prop_name_length + prop->op_length;

	new_prop = krnAllocBootMem(prop_length);

	new_prop->op_name   = new_prop->op_storage;
	new_prop->op_value  = &new_prop->op_storage[prop_name_length];
	new_prop->op_length = prop->op_length;

	__boot_memcpy(new_prop->op_name, prop->op_name, strlen(prop->op_name)+1);
	__boot_memcpy(new_prop->op_value, prop->op_value, prop->op_length);

	ADDTAIL(&new_node->on_properties, new_prop);
    }

    ForeachNode(&orig->on_children, child)
	ADDTAIL(&new_node->on_children, krnCopyOFWTree(child));

    return new_node;
}

static void kernel_start(const struct TagItem *bootMsg)
{
    struct TagItem *tag;
    char *serial_options = NULL;

    /* Enable FPU */
    wrmsr(rdmsr() | MSR_FP);

    tag = LibFindTagItem(KRN_CmdLine, bootMsg);
    if (tag)
    {    
	char *opts = strcasestr((char *)tag->ti_Data, "debug=serial");

	if (opts)
	    serial_options = opts + 12;
    }

    /*
     * Pegasos defaults.
     * Machine's only serial port is configured as Serial2 (0x2F8, IRQ 3).
     * ISA I/O space is mapped at 0xFE000000.
     */
    IO_Base     = (void *)0xFE000000;
    Serial_Base = 0x02F8;
    serial_Init(serial_options);

    D(bug("AROS64 - The AROS Research OS, PegasosPPC version. Compiled %s\n", __DATE__));
    D(bug("[Kernel] kernel.resource entry: 0x%p\n", kernel_entry));
    D(bug("[Kernel] Boot stack: 0x%p - 0x%p\n", boot_stack, boot_stack + STACK_SIZE));
    D(bug("[Kernel] Boot data: 0x%p\n", __BootData));

    if (!__BootData)
    {
    	/*
	 * This is our first start.
	 * Set up BootData and relocate boot taglist to safe place.
	 */
	struct TagItem *dest;
	unsigned long mlen;
        IPTR ptr;
        struct vbe_mode *vmode = NULL;
        char *cmdline = NULL;

	tag = LibFindTagItem(KRN_KernelHighest, bootMsg);
        if (!tag)
        {
            D(bug("Incomplete information from the bootstrap\n"));
            D(bug("Highest kickstart address is not supplied\n"));

            for(;;) HALT;
        }

        /* Align kickstart top address (we are going to place a structure after it) */
        ptr = AROS_ROUNDUP2(tag->ti_Data + 1, sizeof(APTR));

	memset((void *)ptr, 0, sizeof(struct BootData));
        __BootData = (struct BootData *)ptr;

	D(bug("[Kernel] KRN_Highest 0x%p, BootData 0x%p\n", tag->ti_Data, ptr));

	/*
	 * Our boot taglist is placed by the bootstrap just somewhere in memory.
	 * The first thing is to move it into some safe place.
	 */
        ptr = AROS_ROUNDUP2(ptr + sizeof(struct BootData), sizeof(APTR));
        BootMsg = (struct TagItem *)ptr;

	dest = BootMsg;
        while ((tag = LibNextTagItem(&bootMsg)))
    	{
    	    dest->ti_Tag  = tag->ti_Tag;
    	    dest->ti_Data = tag->ti_Data;
    	    dest++;
    	}
	dest->ti_Tag = TAG_DONE;
	dest++;

	__BootData->bd_BootMem = dest;

	/* Now relocate linked data */
	mlen = LibGetTagData(KRN_MMAPLength, 0, BootMsg);
	bootMsg = BootMsg;
	while ((tag = LibNextTagItem(&bootMsg)))
    	{
    	    unsigned long l;
    	    struct KernelBSS *bss;

    	    switch (tag->ti_Tag)
    	    {
    	    case KRN_KernelBss:
    	    	l = sizeof(struct KernelBSS);
    	    	for (bss = (struct KernelBSS *)tag->ti_Data; bss->addr; bss++)
    	    	    l += sizeof(struct KernelBSS);

    	    	RelocateTagData(tag, l);
    	    	break;

	    case KRN_OpenFirmwareTree:
		tag->ti_Data = (IPTR)krnCopyOFWTree((struct OFWNode *)tag->ti_Data);
		break;

    	    case KRN_VBEModeInfo:
    	    	RelocateTagData(tag, sizeof(struct vbe_mode));
    	    	vmode = (struct vbe_mode *)tag->ti_Data;
    	    	break;

    	    case KRN_VBEControllerInfo:
    	    	RelocateTagData(tag, sizeof(struct vbe_controller));
    	    	break;

	    case KRN_CmdLine:
	    	l = strlen((char *)tag->ti_Data) + 1;
	    	RelocateTagData(tag, l);
	    	cmdline = (char *)tag->ti_Data;
	    	break;

	    case KRN_BootLoader:
	    	l = strlen((char *)tag->ti_Data) + 1;
	    	RelocateTagData(tag, l);
	    	break;
	    }
	}
    }

    for (;;);
}

/* Our boot-time stack */
static char boot_stack[STACK_SIZE] __attribute__((aligned(16)));

__attribute__((section(".data"))) struct BootData *__BootData = NULL;
