#define DEBUG

#include <aros/kernel.h>

#include <bootconsole.h>
#include <elfloader.h>
#include <runtime.h>
#include <string.h>

#include "bootstrap.h"
#include "support.h"

/*
 * Search for modules
 */
static unsigned long find_modules(struct multiboot *mb)
{
    unsigned long end = (unsigned long)&_end;

    /* Are there any modules at all? */
    if (mb->flags && MB_FLAGS_MODS)
    {
        int i;
        struct mb_module *mod = (struct mb_module *)mb->mods_addr;
        D(kprintf("[Multiboot] GRUB has loaded %d files\n", mb->mods_count));

        /* Go through the list of modules loaded by GRUB */
        for (i=0; i < mb->mods_count; i++)
        {
            end = AddModule(mod->mod_start, mod->mod_end, end);
            mod++;
        }
    }

    return end;
}

static void setupFB(struct multiboot *mb)
{
    if (mb->flags & MB_FLAGS_GFX)
    {
    	kprintf("[Multiboot] Got VESA display mode 0x%x from the bootstrap\n", mb->vbe_mode);

	D(kprintf("[Multiboot] Mode info 0x%p, controller into 0x%p\n", mb->vbe_mode_info, mb->vbe_control_info));
	D(kprintf("[Multiboot] VBE version 0x%04X\n", ((struct vbe_controller *)mb->vbe_control_info)->version));
	D(kprintf("[Multiboot} Mode flags 0x%04X, framebuffer 0x%p\n", ((struct vbe_mode  *)mb->vbe_mode_info)->mode_attributes, ((struct vbe_mode *)mb->vbe_mode_info)->phys_base));
	D(kprintf("[Multiboot} Windows A 0x%04X B 0x%04X\n", ((struct vbe_mode *)mb->vbe_mode_info)->win_a_segment, ((struct vbe_mode *)mb->vbe_mode_info)->win_b_segment));

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

        return;
    }

    if (mb->flags & MB_FLAGS_FB)
    {
    	kprintf("[Multiboot] Got framebuffer display %dx%dx%d from the bootstrap\n",
    		mb->framebuffer_width, mb->framebuffer_height, mb->framebuffer_bpp);
	D(kprintf("[Multiboot] Address 0x%016llX, type %d, %d bytes per line\n", mb->framebuffer_addr, mb->framebuffer_type, mb->framebuffer_pitch));

	/*
	 * AROS VESA driver supports only RGB framebuffer because we are
	 * unlikely to have VGA palette registers for other cases.
	 * FIXME: we have some pointer to palette registers. We just need to
	 * pass it to the bootstrap and handle it there (how? Is it I/O port
	 * address or memory-mapped I/O address?)
	 */
    	if (mb->framebuffer_type != MB_FRAMEBUFFER_RGB)
    	    return;

	/*
    	 * We have a framebuffer but no VBE information.
    	 * Looks like we are running on EFI machine with no VBE support (Mac).
    	 * Convert framebuffer data to VBEModeInfo and hand it to AROS.
    	 */
    	VBEModeInfo.mode_attributes		= VM_SUPPORTED|VM_COLOR|VM_GRAPHICS|VM_NO_VGA_HW|VM_NO_VGA_MEM|VM_LINEAR_FB;
    	VBEModeInfo.bytes_per_scanline		= mb->framebuffer_pitch;
    	VBEModeInfo.x_resolution		= mb->framebuffer_width;
    	VBEModeInfo.y_resolution		= mb->framebuffer_height;
    	VBEModeInfo.bits_per_pixel		= mb->framebuffer_bpp;
    	VBEModeInfo.memory_model		= VMEM_RGB;
    	VBEModeInfo.red_mask_size		= mb->framebuffer_red_mask_size;
    	VBEModeInfo.red_field_position	        = mb->framebuffer_red_field_position;
    	VBEModeInfo.green_mask_size		= mb->framebuffer_green_mask_size;
    	VBEModeInfo.green_field_position	= mb->framebuffer_green_field_position;
    	VBEModeInfo.blue_mask_size		= mb->framebuffer_blue_mask_size;
    	VBEModeInfo.blue_field_position		= mb->framebuffer_blue_field_position;
	VBEModeInfo.phys_base			= mb->framebuffer_addr;
	VBEModeInfo.linear_bytes_per_scanline   = mb->framebuffer_pitch;
	VBEModeInfo.linear_red_mask_size	= mb->framebuffer_red_mask_size;
	VBEModeInfo.linear_red_field_position   = mb->framebuffer_red_field_position;
	VBEModeInfo.linear_green_mask_size	= mb->framebuffer_green_mask_size;
	VBEModeInfo.linear_green_field_position = mb->framebuffer_green_field_position;
	VBEModeInfo.linear_blue_mask_size	= mb->framebuffer_blue_mask_size;
	VBEModeInfo.linear_blue_field_position  = mb->framebuffer_blue_field_position;

	tag->ti_Tag = KRN_VBEModeInfo;
        tag->ti_Data = KERNEL_OFFSET | (unsigned long)&VBEModeInfo;
        tag++;
    }
}

unsigned long mb1_parse(struct multiboot *mb, struct mb_mmap **mmap_addr, unsigned long *mmap_len)
{
    const char *cmdline = NULL;
    struct mb_mmap *mmap = NULL;
    unsigned long len = 0;

    con_InitMultiboot(mb);
    Hello();
    D(kprintf("[Multiboot] Multiboot v1 structure @ %p\n", mb));

    /*
     * Now allocate our mirror buffer.
     * The buffer is used only by graphical console.
     */
    AllocFB();

    if (mb->flags & MB_FLAGS_CMDLINE)
    {
    	cmdline = (const char *)mb->cmdline;
    	D(kprintf("[Multiboot] Command line @ %p : '%s'\n", mb->cmdline, cmdline));
    }

    if ((mb->flags & MB_FLAGS_MMAP))
    {
    	mmap = (struct mb_mmap *)mb->mmap_addr;
    	len  = mb->mmap_length;

	D(kprintf("[Multiboot] Memory map at 0x%p, length %u\n", mmap, len));
    }

    if (mb->flags & MB_FLAGS_MEM)
    {
        D(kprintf("[Multiboot] Low memory %u KB, upper memory %u KB\n", mb->mem_lower, mb->mem_upper));

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
    }

    if (!mmap)
    	panic("No memory information provided by the bootloader");

    if (ParseCmdLine(cmdline))
    {
    	setupFB(mb);
    }

    *mmap_addr = mmap;
    *mmap_len  = len;

    /* Search for external modules loaded by GRUB */
    return find_modules(mb);
}
