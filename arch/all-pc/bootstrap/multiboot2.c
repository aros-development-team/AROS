/*
    Copyright (C) 2011-2026, The AROS Development Team. All rights reserved.

    Desc: Multiboot v2 parser
*/

/* #define DEBUG */

#include <aros/kernel.h>
#include <aros/multiboot.h>
#include <aros/multiboot2.h>

#include <bootconsole.h>
#include <elfloader.h>
#include <runtime.h>

#include "bootstrap.h"
#include "support.h"

D(
#define str_BSMultiboot2 "bootstrap:multiboot2"
)

static void mb2_fill_vbe_from_fb(const struct mb2_tag_framebuffer *fb)
{
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
    {
        UBYTE maxend = 0;
        UBYTE rend = fb->framebuffer_red_field_position + fb->framebuffer_red_mask_size;
        UBYTE gend = fb->framebuffer_green_field_position + fb->framebuffer_green_mask_size;
        UBYTE bend = fb->framebuffer_blue_field_position + fb->framebuffer_blue_mask_size;
        if (rend > maxend) maxend = rend;
        if (gend > maxend) maxend = gend;
        if (bend > maxend) maxend = bend;
        if (fb->common.framebuffer_bpp > maxend)
        {
            VBEModeInfo.reserved_mask_size      = fb->common.framebuffer_bpp - maxend;
            VBEModeInfo.reserved_field_position = maxend;
        }
        else
        {
            VBEModeInfo.reserved_mask_size      = 0;
            VBEModeInfo.reserved_field_position = 0;
        }
    }
    VBEModeInfo.phys_base                     = fb->common.framebuffer_addr;
    VBEModeInfo.linear_bytes_per_scanline     = fb->common.framebuffer_pitch;
    VBEModeInfo.linear_red_mask_size          = fb->framebuffer_red_mask_size;
    VBEModeInfo.linear_red_field_position     = fb->framebuffer_red_field_position;
    VBEModeInfo.linear_green_mask_size        = fb->framebuffer_green_mask_size;
    VBEModeInfo.linear_green_field_position   = fb->framebuffer_green_field_position;
    VBEModeInfo.linear_blue_mask_size         = fb->framebuffer_blue_mask_size;
    VBEModeInfo.linear_blue_field_position    = fb->framebuffer_blue_field_position;
    VBEModeInfo.linear_reserved_mask_size     = VBEModeInfo.reserved_mask_size;
    VBEModeInfo.linear_reserved_field_position = VBEModeInfo.reserved_field_position;
}

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

    DMMAP(kprintf("[%s] Memory map at 0x%p, total size %u, entry size %u\n", str_BSMultiboot2, mmap2, mmap2_len, tag->entry_size);)

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
#if defined(MULTIBOOT_64BIT)
    int mb2_have_efi64 = 0;
    unsigned long long mb2_efi_systable = 0;
#else
    unsigned long mb2_efi_systable = 0;
#endif

    con_InitMultiboot2(mb);
    Hello();
    D(kprintf("[%s] Multiboot v2 data @ 0x%p [%u bytes]\n", str_BSMultiboot2, mb, *(unsigned int *)mb);)

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
        DTAGS(kprintf("[%s] Tag %u, size %u\n", str_BSMultiboot2, mbtag->type, mbtag->size);)

        switch (mbtag->type)
        {
        case MB2_TAG_CMDLINE:
            cmdline = ((struct mb2_tag_string *)mbtag)->string;
            D(kprintf("[%s] Command line @ 0x%p : '%s'\n", str_BSMultiboot2, cmdline, cmdline);)
            break;

        case MB2_TAG_MMAP:
            mmap = mmap_convert((struct mb2_tag_mmap *)mbtag, mmap_len);
            D(kprintf("[%s] Memory map @ 0x%p\n", str_BSMultiboot2, mmap);)
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

#if defined(MULTIBOOT_64BIT)
        case MB2_TAG_EFI64:
            D(kprintf("[%s] EFI 64-bit System table 0x%016llX\n", str_BSMultiboot2, ((struct mb2_tag_efi64 *)mbtag)->pointer);)
            if (mb2_have_efi64 == 0)
            {
                mb2_efi_systable = ((struct mb2_tag_efi64 *)mbtag)->pointer;
                mb2_have_efi64 = 1;
            }
            break;
#endif

        case MB2_TAG_EFI32:
            D(kprintf("[%s] EFI 32-bit System table 0x%08X\n", str_BSMultiboot2, ((struct mb2_tag_efi32 *)mbtag)->pointer);)
#if defined(MULTIBOOT_64BIT)
            if (mb2_have_efi64 == 0)
                mb2_efi_systable = ((struct mb2_tag_efi32 *)mbtag)->pointer;
#else
            mb2_efi_systable = ((struct mb2_tag_efi32 *)mbtag)->pointer;
#endif
            break;
        }
    }

    if (mb2_efi_systable != 0)
    {
        tag->ti_Tag  = KRN_EFISystemTable;
        tag->ti_Data = mb2_efi_systable;
        tag++;
    }

    if (!mmap && memlower && memupper)
    {
        /* Build a memory map if we haven't got one */
        mmap = mmap_make(mmap_len, memlower, memupper);
    }

    if (ParseCmdLine(cmdline))
    {
        BOOL fb_rgb = (fb && (fb->common.framebuffer_type == MB2_FRAMEBUFFER_RGB));
        BOOL vbe_graphics = (vbe && (vbe->vbe_mode_info.mode_attributes & VM_GRAPHICS));

#if !defined(MULTIBOOT_64BIT)
        /*
         * A 32-bit target can never address a framebuffer above 4GiB: both
         * VBEModeInfo.phys_base and ti_Data would truncate. Ignore such a
         * framebuffer and fall back to VBE/VGA instead.
         */
        if (fb_rgb && (fb->common.framebuffer_addr >> 32) != 0)
        {
            kprintf("[bootstrap:multiboot2] smartfb: framebuffer @ 0x%016llX is above 4GiB - unusable on 32-bit, skipping\n",
                fb->common.framebuffer_addr);
            fb_rgb = FALSE;
        }
#endif
        BOOL choose_gop = FALSE, mode_set_here = FALSE;

        if (fb_rgb)
        {
            if (!vbe_graphics || mb2_efi_systable != 0 || (fb->common.framebuffer_addr >> 32) != 0)
                choose_gop = TRUE;
        }

        /*
         * The bootstrap's Multiboot2 header asks for a 32bpp framebuffer, but
         * that is only a hint and loaders are free to ignore it. GRUB hands us
         * a packed 24bpp mode on some machines, which is the one truecolor
         * layout our display code and the emulated hardware disagree over -
         * the geometry comes out right but the picture is striped and
         * discoloured. On a BIOS machine there is still a VBE BIOS to call, so
         * ask it for the same resolution in a 32bpp mode and use that instead.
         * setupVESAMode() describes the mode it sets itself, so there is
         * nothing left for us to pass on when it succeeds.
         */
        if (choose_gop && mb2_efi_systable == 0 && fb->common.framebuffer_bpp == 24)
        {
            kprintf("[bootstrap:multiboot2] smartfb: loader gave a packed 24bpp framebuffer, asking VBE for %ux%ux32\n",
                fb->common.framebuffer_width,
                fb->common.framebuffer_height);

            if (setupVESAMode(fb->common.framebuffer_width, fb->common.framebuffer_height, 32, 60, FALSE, FALSE))
            {
                choose_gop = FALSE;
                fb_rgb = FALSE;
                mode_set_here = TRUE;
            }
            else
            {
                kprintf("[bootstrap:multiboot2] smartfb: no 32bpp VBE mode available, keeping the framebuffer\n");
            }
        }

        if (choose_gop)
        {
            kprintf("[bootstrap:multiboot2] smartfb: using GOP framebuffer (%ux%ux%u @ 0x%016llX pitch=%u)%s%s%s\n",
                fb->common.framebuffer_width,
                fb->common.framebuffer_height,
                fb->common.framebuffer_bpp,
                fb->common.framebuffer_addr,
                fb->common.framebuffer_pitch,
                vbe ? " with VBE data present;" : "",
                (mb2_efi_systable != 0) ? " EFI boot;" : "",
                ((fb->common.framebuffer_addr >> 32) != 0) ? " FB above 4GB" : "");

            mb2_fill_vbe_from_fb(fb);

            tag->ti_Tag  = KRN_VBEModeInfo;
            tag->ti_Data = KERNEL_OFFSET | (unsigned long)&VBEModeInfo;
            tag++;
        }
        else if (vbe_graphics && !mode_set_here)
        {
            D(kprintf("[%s] Got VESA display mode 0x%x from the bootstrap\n", str_BSMultiboot2, vbe->vbe_mode);)
            kprintf("[bootstrap:multiboot2] smartfb: using VBE mode 0x%x (%ux%ux%u, fb=0x%08X)\n",
                vbe->vbe_mode,
                vbe->vbe_mode_info.x_resolution,
                vbe->vbe_mode_info.y_resolution,
                vbe->vbe_mode_info.bits_per_pixel,
                vbe->vbe_mode_info.phys_base);

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
        else if (fb && !mode_set_here)
        {
            D(
                kprintf("[%s] Got framebuffer display %dx%dx%d from the bootstrap\n", str_BSMultiboot2,
                    fb->common.framebuffer_width, fb->common.framebuffer_height, fb->common.framebuffer_bpp);
                kprintf("[%s] Address 0x%016llX, type %d, %d bytes per line\n", str_BSMultiboot2, fb->common.framebuffer_addr, fb->common.framebuffer_type, fb->common.framebuffer_pitch);
            )

            /*
             * AROS VESA driver supports only RGB framebuffer because we are
             * unlikely to have VGA palette registers for other cases.
             * FIXME: we have some pointer to palette registers. We just need to
             * pass it to the bootstrap and handle it there (how? Is it I/O port
             * address or memory-mapped I/O address?)
             */
            if (fb_rgb)
            {
                kprintf("[bootstrap:multiboot2] smartfb: using framebuffer-only path (%ux%ux%u @ 0x%016llX)\n",
                    fb->common.framebuffer_width,
                    fb->common.framebuffer_height,
                    fb->common.framebuffer_bpp,
                    fb->common.framebuffer_addr);

                mb2_fill_vbe_from_fb(fb);

                tag->ti_Tag  = KRN_VBEModeInfo;
                tag->ti_Data = KERNEL_OFFSET | (unsigned long)&VBEModeInfo;
                tag++;
            }
            else
            {
                kprintf("[bootstrap:multiboot2] smartfb: unsupported framebuffer type %u\n",
                    fb->common.framebuffer_type);
            }
        }

        if (fb_rgb)
        {
            /*
             * Pass full 64-bit framebuffer address regardless of whether we are using
             * VBE data or synthesized GOP data. This allows the kernel/bootloader.resource
             * to override 32-bit phys_base fields when framebuffer is above 4GB.
             */
            tag->ti_Tag  = KRN_FBAddr;
            tag->ti_Data = (unsigned long long)fb->common.framebuffer_addr;
            tag++;

            kprintf("[bootstrap:multiboot2] smartfb: KRN_FBAddr=0x%016llX\n", fb->common.framebuffer_addr);
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
