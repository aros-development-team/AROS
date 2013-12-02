/*
    Copyright © 2011-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Multiboot v2 parser
    Lang: english
*/

/* #define DEBUG */
#define DTAGS(x)
#define DMMAP(x)

#include <aros/kernel.h>
#include <aros/multiboot.h>
#include <aros/multiboot2.h>

#include <bootconsole.h>
#include <elfloader.h>
#include <runtime.h>

#include "bootstrap.h"
#include "support.h"

/*
 * AROS expects memory map in original format. However, we won't bother 
 * adding more and more new kernel tags. We convert the memory map instead. 
 * The conversion happens in place. We use the fact that the layout of 
 * mb2_mmap is the same as mb_mmap except for the missing 'size' field. An 
 * mb_mmap can therefore be created by subtracting four bytes from the base 
 * address and filling in the 'size' field (mb2_mmap's 'pad' field, or the 
 * end of the tag structure) with the provided entry size. This will still 
 * work if mb2_mmap is extended in future, but we assume the old mb_mmap 
 * will not be extended.
 */
static struct mb_mmap *mmap_convert(struct mb2_tag_mmap *tag, unsigned long *mmap_len)
{
    volatile struct mb2_mmap *mmap2 = tag->mmap;
    volatile struct mb_mmap *mmap = (void *)tag->mmap - 4;
    int mmap2_len = tag->size - sizeof(struct mb2_tag_mmap);
    struct mb_mmap *ret = (struct mb_mmap *)mmap;

    DMMAP(kprintf("[Multiboot2] Memory map at 0x%p, total size %u, entry size %u\n", mmap2, mmap2_len, tag->entry_size));

    while (mmap2_len >= sizeof(struct mb2_mmap))
    {
        mmap->size = sizeof(struct mb_mmap) - 4;

        mmap++;
        mmap2 = (void *)mmap2 + tag->entry_size;
        mmap2_len -= tag->entry_size;
    }

    *mmap_len = (char *)mmap - (char *)ret;
    return ret;
}

unsigned long mb2_parse(void *mb, struct mb_mmap **mmap_addr, unsigned long *mmap_len)
{
    struct mb2_tag *mbtag;
    struct mb2_tag_framebuffer *fb = NULL;
    struct mb2_tag_vbe *vbe = NULL;
    struct mb2_tag_module *mod;
    const char *cmdline = NULL;
    struct mb_mmap *mmap = NULL;
    unsigned long memlower = 0;
    unsigned long long memupper = 0;
    unsigned long usable = (unsigned long)&_end;

    con_InitMultiboot2(mb);
    Hello();
    D(kprintf("[Multiboot2] Multiboot v2 data @ 0x%p [%u bytes]\n", mb, *(unsigned int *)mb));

    AllocFB();

    /*
     * The supplied pointer points to a UQUAD value specifying total length of the
     * whole data array.
     */
    usable = TOP_ADDR(usable, mb + *(unsigned long long *)mb);

    /*
     * Iterate all tags and retrieve the information we want.
     * Every next tag is UQUAD-aligned. 'size' field doesn't include padding, so we round it up
     * to a multiple of 8.
     */
    for (mbtag = mb + 8; mbtag->type != MB2_TAG_END; mbtag = (void *)mbtag + AROS_ROUNDUP2(mbtag->size, 8))
    {
        DTAGS(kprintf("[Multiboot2] Tag %u, size %u\n", mbtag->type, mbtag->size));

        switch (mbtag->type)
        {
        case MB2_TAG_CMDLINE:
            cmdline = ((struct mb2_tag_string *)mbtag)->string;
            D(kprintf("[Multiboot2] Command line @ 0x%p : '%s'\n", cmdline, cmdline));
            break;

        case MB2_TAG_MMAP:
            mmap = mmap_convert((struct mb2_tag_mmap *)mbtag, mmap_len);
            D(kprintf("[Multiboot2] Memory map @ 0x%p\n", mmap));
            break;

        case MB2_TAG_BASIC_MEMINFO:
            /* Got lower/upper memory size */
            memlower =                     ((struct mb2_tag_basic_meminfo *)mbtag)->mem_lower << 10;
            memupper = (unsigned long long)((struct mb2_tag_basic_meminfo *)mbtag)->mem_upper << 10;

            tag->ti_Tag  = KRN_MEMLower;
            tag->ti_Data = memlower;
            tag++;

            tag->ti_Tag  = KRN_MEMUpper;
            tag->ti_Data = memupper;
            tag++;

            break;

        case MB2_TAG_FRAMEBUFFER:
            fb = (struct mb2_tag_framebuffer *)mbtag;
            break;

        case MB2_TAG_VBE:
            vbe = (struct mb2_tag_vbe *)mbtag;
            break;

        case MB2_TAG_BOOTLOADER_NAME:
            tag->ti_Tag  = KRN_BootLoader;
            tag->ti_Data = (unsigned long)((struct mb2_tag_string *)mbtag)->string;
            tag++;

            break;

#ifdef MULTIBOOT_64BIT
        case MB2_TAG_EFI64:
            D(kprintf("[Multiboot2] EFI 64-bit System table 0x%016llX\n", ((struct mb2_tag_efi64 *)mbtag)->pointer));

            tag->ti_Tag  = KRN_EFISystemTable;
            tag->ti_Data = ((struct mb2_tag_efi64 *)mbtag)->pointer;
#else
        case MB2_TAG_EFI32:
            D(kprintf("[Multiboot2] EFI 32-bit System table 0x%08X\n", ((struct mb2_tag_efi32 *)mbtag)->pointer));

            tag->ti_Tag  = KRN_EFISystemTable;
            tag->ti_Data = ((struct mb2_tag_efi32 *)mbtag)->pointer;
#endif
            tag++;

            break;
        }
    }

    if (!mmap && memlower && memupper)
    {
        /* Build a memory map if we haven't got one */
        mmap = mmap_make(mmap_len, memlower, memupper);
    }

    if (ParseCmdLine(cmdline))
    {
        if (vbe)
        {
            kprintf("[Multiboot2] Got VESA display mode 0x%x from the bootstrap\n", vbe->vbe_mode);

            /*
             * We are already running in VESA mode set by the bootloader.
             * Pass on the mode information to AROS.
             */
            tag->ti_Tag  = KRN_VBEModeInfo;
            tag->ti_Data = (unsigned long)&vbe->vbe_mode_info;
            tag++;

            tag->ti_Tag  = KRN_VBEControllerInfo;
            tag->ti_Data = (unsigned long)&vbe->vbe_control_info;
            tag++;

            tag->ti_Tag  = KRN_VBEMode;
            tag->ti_Data = vbe->vbe_mode;
            tag++;
        }
        else if (fb)
        {
            kprintf("[Multiboot2] Got framebuffer display %dx%dx%d from the bootstrap\n",
                    fb->common.framebuffer_width, fb->common.framebuffer_height, fb->common.framebuffer_bpp);
            D(kprintf("[Multiboot2] Address 0x%016llX, type %d, %d bytes per line\n", fb->common.framebuffer_addr, fb->common.framebuffer_type, fb->common.framebuffer_pitch));

            /*
             * AROS VESA driver supports only RGB framebuffer because we are
             * unlikely to have VGA palette registers for other cases.
             * FIXME: we have some pointer to palette registers. We just need to
             * pass it to the bootstrap and handle it there (how? Is it I/O port
             * address or memory-mapped I/O address?)
             */
            if (fb->common.framebuffer_type == MB2_FRAMEBUFFER_RGB)
            {
                /*
                 * We have a framebuffer but no VBE information.
                 * Looks like we are running on EFI machine with no VBE support (Mac).
                 * Convert framebuffer data to VBEModeInfo and hand it to AROS.
                 */
                VBEModeInfo.mode_attributes             = VM_SUPPORTED|VM_COLOR|VM_GRAPHICS|VM_NO_VGA_HW|VM_NO_VGA_MEM|VM_LINEAR_FB;
                VBEModeInfo.bytes_per_scanline          = fb->common.framebuffer_pitch;
                VBEModeInfo.x_resolution                = fb->common.framebuffer_width;
                VBEModeInfo.y_resolution                = fb->common.framebuffer_height;
                VBEModeInfo.bits_per_pixel              = fb->common.framebuffer_bpp;
                VBEModeInfo.memory_model                = VMEM_RGB;
                VBEModeInfo.red_mask_size               = fb->framebuffer_red_mask_size;
                VBEModeInfo.red_field_position          = fb->framebuffer_red_field_position;
                VBEModeInfo.green_mask_size             = fb->framebuffer_green_mask_size;
                VBEModeInfo.green_field_position        = fb->framebuffer_green_field_position;
                VBEModeInfo.blue_mask_size              = fb->framebuffer_blue_mask_size;
                VBEModeInfo.blue_field_position         = fb->framebuffer_blue_field_position;
                VBEModeInfo.phys_base                   = fb->common.framebuffer_addr;
                VBEModeInfo.linear_bytes_per_scanline   = fb->common.framebuffer_pitch;
                VBEModeInfo.linear_red_mask_size        = fb->framebuffer_red_mask_size;
                VBEModeInfo.linear_red_field_position   = fb->framebuffer_red_field_position;
                VBEModeInfo.linear_green_mask_size      = fb->framebuffer_green_mask_size;
                VBEModeInfo.linear_green_field_position = fb->framebuffer_green_field_position;
                VBEModeInfo.linear_blue_mask_size       = fb->framebuffer_blue_mask_size;
                VBEModeInfo.linear_blue_field_position  = fb->framebuffer_blue_field_position;

                tag->ti_Tag  = KRN_VBEModeInfo;
                tag->ti_Data = KERNEL_OFFSET | (unsigned long)&VBEModeInfo;
                tag++;
            }
        }
    }

    /* Return memory map address. Length is already provided by either mmap_make() or mmap_convert() */
    *mmap_addr = mmap;

    /* Search for external modules loaded by GRUB */
    for (mod = mb + 8; mod->type != MB2_TAG_END; mod = (void *)mod + AROS_ROUNDUP2(mod->size, 8))
    {
        if (mod->type == MB2_TAG_MODULE)
            usable = AddModule(mod->mod_start, mod->mod_end, usable);
    }

    return usable;
}
