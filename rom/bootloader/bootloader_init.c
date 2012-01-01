/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Bootloader information initialisation.
*/

#define DEBUG 0

#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/multiboot.h>
#include <aros/symbolsets.h>
#include <aros/bootloader.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <utility/utility.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/bootloader.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include <ctype.h>
#include <string.h>

#include "bootloader_intern.h"

#include LC_LIBDEFS_FILE

static void GetCmdLine(char *Kernel_Args, struct BootLoaderBase *BootLoaderBase)
{
    STRPTR buff;
    ULONG len;

    /* Sanity check against broken bootstraps */
    if (!Kernel_Args)
    	return;

    D(bug("[BootLdr] Kernel arguments: %s\n", Kernel_Args));

    /* First make a copy of the command line */
    len = strlen(Kernel_Args) + 1;
    buff = AllocMem(len, MEMF_ANY);
    if (buff)
    {
	CopyMem(Kernel_Args, buff, len);

	while (1)
	{
	    struct Node *node;

	    /* remove any leading spaces */
	    buff = stpblk(buff);

	    /* Hit end of line (trailing spaces) ? Exit if so. */
	    if (!*buff)
	    	break;

	    /* Allocate node and insert into list */
	    node = AllocMem(sizeof(struct Node),MEMF_ANY);
	    if (!node)
	    	break;

	    node->ln_Name = buff;
	    AddTail(&(BootLoaderBase->Args), node);

	    /* We now have the command line */
	    BootLoaderBase->Flags |= BL_FLAGS_CMDLINE;

	    /* Skip up to a next space or EOL */
	    while (*buff != '\0' && !isspace(*buff))
		buff++;

	    /* End of line ? If yes, we are done. */
	    if (!*buff)
	    {
		D(bug("[BootLdr] Init: Last argument %s\n", node->ln_Name));
		break;
	    }

	    /* Split the line and repeat */
	    *buff++ = 0;
	    D(bug("[BootLdr] Init: Argument %s\n", node->ln_Name));
	}
    }
}

static const ULONG masks [] = {0x01, 0x03, 0x07, 0x0f ,0x1f, 0x3f, 0x7f, 0xff};

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR BootLoaderBase)
{
    struct TagItem *tag, *bootinfo;
    APTR KernelBase;
    struct Library *UtilityBase;
    struct vbe_mode *vmi = NULL;
    struct vbe_controller *vci = NULL;
    UWORD vmode = 0x00FF;	/* Dummy mode number by default		  */
    UBYTE palette = 0;		/* By default we don't know palette width */

    D(bug("[BootLdr] Init\n"));

    UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    if (!UtilityBase)
        return FALSE;

    NEWLIST(&(BootLoaderBase->Args));
    NEWLIST(&(BootLoaderBase->DriveInfo));

    KernelBase = OpenResource("kernel.resource");
    bootinfo = (struct TagItem *)KrnGetBootInfo();
    
    while ((tag = NextTagItem(&bootinfo)))
    {
    	switch (tag->ti_Tag)
    	{
    	case KRN_BootLoader:
    	    BootLoaderBase->LdrName = (STRPTR)tag->ti_Data;
    	    break;

    	case KRN_CmdLine:
    	    GetCmdLine((char *)tag->ti_Data, BootLoaderBase);
    	    break;

    	case KRN_VBEModeInfo:
    	    vmi = (struct vbe_mode *)tag->ti_Data;
    	    break;

	case KRN_VBEControllerInfo:
	    vci = (struct vbe_controller *)tag->ti_Data;
	    break;

	case KRN_VBEMode:
	    vmode = tag->ti_Data;
	    break;

	case KRN_VBEPaletteWidth:
	    palette = tag->ti_Data;
	    break;
	}
    }

    /* Get VESA mode information */
    D(bug("[BootLdr] VESA mode info 0x%p, controller info 0x%p\n", vmi, vci));

    /* We support only graphical framebuffers here. */
    if (vmi && (vmi->mode_attributes & VM_GRAPHICS))
    {
	BootLoaderBase->Vesa = AllocMem(sizeof(struct VesaInfo), MEMF_CLEAR);

	if (BootLoaderBase->Vesa)
	{
	    /* If we don't have VBEControllerInfo, this is VBE v2 mode structure */
	    unsigned short version = 0x0200;

	    if (vci)
	    {
		D(bug("[BootLdr] Init: Vesa card capability flags: 0x%08X\n", vci->capabilities));

	    	version = vci->version;
		/* FrameBufferSize is in KBytes! */
	    	BootLoaderBase->Vesa->FrameBufferSize = vci->total_memory << 6;
	    }

	    BootLoaderBase->Vesa->XSize            = vmi->x_resolution;
	    BootLoaderBase->Vesa->YSize            = vmi->y_resolution;
	    BootLoaderBase->Vesa->BitsPerPixel     = vmi->bits_per_pixel;
	    BootLoaderBase->Vesa->ModeNumber	   = vmode;
	    BootLoaderBase->Vesa->PaletteWidth	   = palette;

	    /* Framebuffer pointer is only valid for VBE v2 and better */
	    if (version >= 0x0200)
		BootLoaderBase->Vesa->FrameBuffer = (APTR)(IPTR)vmi->phys_base;

	    if (version >= 0x0300)
	    {
	    	/* VBE v3 structures specify linear framebuffer parameters in separate fields */
	    	BootLoaderBase->Vesa->BytesPerLine     = vmi->linear_bytes_per_scanline;
	    	BootLoaderBase->Vesa->Masks [VI_Red]   = masks[vmi->linear_red_mask_size-1]<<vmi->linear_red_field_position;
	    	BootLoaderBase->Vesa->Masks [VI_Blue]  = masks[vmi->linear_blue_mask_size-1]<<vmi->linear_blue_field_position;
	    	BootLoaderBase->Vesa->Masks [VI_Green] = masks[vmi->linear_green_mask_size-1]<<vmi->linear_green_field_position;
	    	BootLoaderBase->Vesa->Masks [VI_Alpha] = masks[vmi->linear_reserved_mask_size-1]<<vmi->linear_reserved_field_position;
	    	BootLoaderBase->Vesa->Shifts[VI_Red]   = 32 - vmi->linear_red_field_position - vmi->linear_red_mask_size;
	    	BootLoaderBase->Vesa->Shifts[VI_Blue]  = 32 - vmi->linear_blue_field_position - vmi->linear_blue_mask_size;
	    	BootLoaderBase->Vesa->Shifts[VI_Green] = 32 - vmi->linear_green_field_position - vmi->linear_green_mask_size;
	    	BootLoaderBase->Vesa->Shifts[VI_Alpha] = 32 - vmi->linear_reserved_field_position - vmi->linear_reserved_mask_size;
	    }
	    else
	    {
	    	BootLoaderBase->Vesa->BytesPerLine     = vmi->bytes_per_scanline;
	    	BootLoaderBase->Vesa->Masks [VI_Red]   = masks[vmi->red_mask_size-1]<<vmi->red_field_position;
	    	BootLoaderBase->Vesa->Masks [VI_Blue]  = masks[vmi->blue_mask_size-1]<<vmi->blue_field_position;
	    	BootLoaderBase->Vesa->Masks [VI_Green] = masks[vmi->green_mask_size-1]<<vmi->green_field_position;
	    	BootLoaderBase->Vesa->Masks [VI_Alpha] = masks[vmi->reserved_mask_size-1]<<vmi->reserved_field_position;
	    	BootLoaderBase->Vesa->Shifts[VI_Red]   = 32 - vmi->red_field_position - vmi->red_mask_size;
	    	BootLoaderBase->Vesa->Shifts[VI_Blue]  = 32 - vmi->blue_field_position - vmi->blue_mask_size;
	    	BootLoaderBase->Vesa->Shifts[VI_Green] = 32 - vmi->green_field_position - vmi->green_mask_size;
	    	BootLoaderBase->Vesa->Shifts[VI_Alpha] = 32 - vmi->reserved_field_position - vmi->reserved_mask_size;
	    }

	    D(bug("[BootLdr] Init: Vesa mode %x type (%dx%dx%d)\n", BootLoaderBase->Vesa->ModeNumber,
		  BootLoaderBase->Vesa->XSize, BootLoaderBase->Vesa->YSize, BootLoaderBase->Vesa->BitsPerPixel));
	    D(bug("[BootLdr] Init: Vesa FB at 0x%p size %d kB\n", BootLoaderBase->Vesa->FrameBuffer,
		  BootLoaderBase->Vesa->FrameBufferSize));
	    D(bug("[BootLdr] Init: Vesa mode palette width: %d\n", BootLoaderBase->Vesa->PaletteWidth));
	    D(bug("[BootLdr] Init: Vesa mode direct color flags %02X\n", vmi->direct_color_mode_info));
	}
    }

    CloseLibrary(UtilityBase);

    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
