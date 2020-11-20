/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

/* #define DEBUG */

#include <aros/kernel.h>

#include <bootconsole.h>
#include <elfloader.h>
#include <runtime.h>
#include <string.h>

#include "bootstrap.h"
#include "support.h"

D(
#define str_BSMultiboot "bootstrap:multiboot"
)

unsigned long mb1_parse(struct multiboot *mb, struct mb_mmap **mmap_addr, unsigned long *mmap_len)
{
    const char *cmdline = NULL;
    struct mb_mmap *mmap = NULL;
    unsigned long len = 0;
    unsigned long usable = (unsigned long)&_end;

    con_InitMultiboot(mb);
    Hello();
    D(kprintf("[%s] Multiboot v1 structure @ %p\n", str_BSMultiboot, mb);)

    /*
     * Now allocate our mirror buffer.
     * The buffer is used only by graphical console.
     */
    AllocFB();

    if (mb->flags & MB_FLAGS_CMDLINE)
    {
        cmdline = (const char *)mb->cmdline;
        D(kprintf("[%s] Command line @ %p : '%s'\n", str_BSMultiboot, mb->cmdline, cmdline);)

        usable = STR_TOP_ADDR(usable, cmdline);
    }

    if ((mb->flags & MB_FLAGS_MMAP))
    {
        mmap = (struct mb_mmap *)mb->mmap_addr;
        len  = mb->mmap_length;

        D(kprintf("[%s] Memory map at 0x%p, length %u\n", str_BSMultiboot, mmap, len);)
        usable = TOP_ADDR(usable, (void *)mmap + len);
    }

    if (mb->flags & MB_FLAGS_MEM)
    {
        D(kprintf("[%s] Low memory %u KB, upper memory %u KB\n", str_BSMultiboot, mb->mem_lower, mb->mem_upper);)

        if (!mmap)
        {
            /*
             * To simplify things down, memory map is mandatory for our kickstart.
             * So we create an implicit one if the bootloader didn't supply it.
             */
            mmap = mmap_make(&len, mb->mem_lower << 10, (unsigned long long)mb->mem_upper << 10);
        }

        /* Kickstart wants size in bytes */
        tag->ti_Tag = KRN_MEMLower;
        tag->ti_Data = mb->mem_lower << 10;
        tag++;

        tag->ti_Tag = KRN_MEMUpper;
        tag->ti_Data = (unsigned long long)mb->mem_upper << 10;
        tag++;
    }

    if (mb->flags & MB_FLAGS_LDRNAME)
    {
        tag->ti_Tag  = KRN_BootLoader;
        tag->ti_Data = mb->loader_name;
        tag++;
        
        usable = STR_TOP_ADDR(usable, (const char *)mb->loader_name);
    }

    if (!mmap)
        panic("No memory information provided by the bootloader");

    if (ParseCmdLine(cmdline))
    {
        /* No VESA mode was set by command line arguments, supply what we already have */
        if (mb->flags & MB_FLAGS_GFX)
        {
            /* We prefer complete VBE data if present */
            D(
                kprintf("[%s] Got VESA display mode 0x%x from the bootstrap\n", str_BSMultiboot, mb->vbe_mode);
                kprintf("[%s]   Mode info 0x%p, controller into 0x%p\n", str_BSMultiboot, mb->vbe_mode_info, mb->vbe_control_info);
                kprintf("[%s]   VBE version 0x%04X\n", str_BSMultiboot, ((struct vbe_controller *)mb->vbe_control_info)->version);
                kprintf("[%s]   Resolution %d x %d\n", str_BSMultiboot, ((struct vbe_mode  *)mb->vbe_mode_info)->x_resolution, ((struct vbe_mode  *)mb->vbe_mode_info)->y_resolution);
                kprintf("[%s]   Mode flags 0x%04X, framebuffer 0x%p\n", str_BSMultiboot, ((struct vbe_mode  *)mb->vbe_mode_info)->mode_attributes, ((struct vbe_mode *)mb->vbe_mode_info)->phys_base);
                kprintf("[%s]   Windows A 0x%04X B 0x%04X\n", str_BSMultiboot, ((struct vbe_mode *)mb->vbe_mode_info)->win_a_segment, ((struct vbe_mode *)mb->vbe_mode_info)->win_b_segment);
            )

            /*
             * We are already running in VESA mode set by the bootloader.
             * Pass on the mode information to AROS.
             */
            tag->ti_Tag = KRN_VBEModeInfo;
            tag->ti_Data = mb->vbe_mode_info;
            tag++;

            tag->ti_Tag = KRN_VBEControllerInfo;
            tag->ti_Data = mb->vbe_control_info;
            tag++;

            tag->ti_Tag = KRN_VBEMode;
            tag->ti_Data = mb->vbe_mode;
            tag++;

            usable = TOP_ADDR(usable, mb->vbe_mode_info + sizeof(struct vbe_mode));
            usable = TOP_ADDR(usable, mb->vbe_control_info + sizeof(struct vbe_controller));
        }
        else if (mb->flags & MB_FLAGS_FB)
        {
            /*
             * We have a framebuffer but no VBE information.
             * Looks like we are running on EFI machine with no VBE support (Mac).
             * Convert framebuffer data to VBEModeInfo and hand it to AROS.
             */
            D(
                kprintf("[%s] Got framebuffer display %dx%dx%d from the bootstrap\n", str_BSMultiboot, mb->framebuffer_width, mb->framebuffer_height, mb->framebuffer_bpp);
                kprintf("[%s]   Address 0x%016llX, type %d, %d bytes per line\n", str_BSMultiboot, mb->framebuffer_addr, mb->framebuffer_type, mb->framebuffer_pitch);
            )

            /*
             * AROS VESA driver supports only RGB framebuffer because we are
             * unlikely to have VGA palette registers for other cases.
             * FIXME: we have some pointer to palette registers. We just need to
             * pass it to the bootstrap and handle it there (how? Is it I/O port
             * address or memory-mapped I/O address?)
             */
            if (mb->framebuffer_type == MB_FRAMEBUFFER_RGB)
            {
				VBEModeInfo.mode_attributes             = VM_SUPPORTED|VM_COLOR|VM_GRAPHICS|VM_NO_VGA_HW|VM_NO_VGA_MEM|VM_LINEAR_FB;
				VBEModeInfo.bytes_per_scanline          = mb->framebuffer_pitch;
				VBEModeInfo.x_resolution                = mb->framebuffer_width;
				VBEModeInfo.y_resolution                = mb->framebuffer_height;
				VBEModeInfo.bits_per_pixel              = mb->framebuffer_bpp;
				VBEModeInfo.memory_model                = VMEM_RGB;
				VBEModeInfo.red_mask_size               = mb->framebuffer_red_mask_size;
				VBEModeInfo.red_field_position          = mb->framebuffer_red_field_position;
				VBEModeInfo.green_mask_size             = mb->framebuffer_green_mask_size;
				VBEModeInfo.green_field_position        = mb->framebuffer_green_field_position;
				VBEModeInfo.blue_mask_size              = mb->framebuffer_blue_mask_size;
				VBEModeInfo.blue_field_position         = mb->framebuffer_blue_field_position;
				VBEModeInfo.phys_base                   = mb->framebuffer_addr;
				VBEModeInfo.linear_bytes_per_scanline   = mb->framebuffer_pitch;
				VBEModeInfo.linear_red_mask_size        = mb->framebuffer_red_mask_size;
				VBEModeInfo.linear_red_field_position   = mb->framebuffer_red_field_position;
				VBEModeInfo.linear_green_mask_size      = mb->framebuffer_green_mask_size;
				VBEModeInfo.linear_green_field_position = mb->framebuffer_green_field_position;
				VBEModeInfo.linear_blue_mask_size       = mb->framebuffer_blue_mask_size;
				VBEModeInfo.linear_blue_field_position  = mb->framebuffer_blue_field_position;
			
				tag->ti_Tag = KRN_VBEModeInfo;
				tag->ti_Data = KERNEL_OFFSET | (unsigned long)&VBEModeInfo;
				tag++;
			}
        }
    }

    *mmap_addr = mmap;
    *mmap_len  = len;

    /* Search for external modules loaded by GRUB */
    /* Are there any modules at all? */
    if (mb->flags && MB_FLAGS_MODS)
    {
        struct mb_module *mod = (struct mb_module *)mb->mods_addr;
        int i;

        D(kprintf("[%s] GRUB has loaded %d files\n", str_BSMultiboot, mb->mods_count);)

        /* Go through the list of modules loaded by GRUB */
        for (i=0; i < mb->mods_count; i++)
        {
            usable = AddModule(mod->mod_start, mod->mod_end, usable);
            mod++;
        }
    }

    /* We return the lowest address suitable for loading kickstart into */
    return usable;
}
