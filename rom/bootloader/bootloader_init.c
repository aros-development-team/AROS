/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Bootloader information initialisation.
*/

#define DEBUG 0

#include <aros/config.h>
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

#include <string.h>

#include "bootloader_intern.h"

#include LC_LIBDEFS_FILE

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR BootLoaderBase)
{
    struct TagItem *bootinfo;
    STRPTR Kernel_Args;
    APTR KernelBase;
#if (AROS_FLAVOUR & AROS_FLAVOUR_STANDALONE)
    struct vbe_mode *vmi;
    struct vbe_controller *vci;
#endif

    D(bug("[BootLdr] Init\n"));
    NEWLIST(&(BootLoaderBase->Args));
    NEWLIST(&(BootLoaderBase->DriveInfo));

    KernelBase = OpenResource("kernel.resource");
    bootinfo = KrnGetBootInfo();
    BootLoaderBase->LdrName = (STRPTR)GetTagData(KRN_BootLoader, 0, bootinfo);

    /* Get kernel arguments */
    Kernel_Args = (STRPTR)GetTagData(KRN_CmdLine, 0, bootinfo);
    if (Kernel_Args) {
	STRPTR cmd, buff;
	ULONG temp;
	struct Node *node;
	ULONG len = strlen(Kernel_Args) + 1;	    

	D(bug("[BootLdr] Kernel arguments: %s\n", Kernel_Args));

	/* First make a working copy of the command line */
	buff = AllocMem(len, MEMF_ANY|MEMF_CLEAR);
	if (buff) {
	    CopyMem(Kernel_Args, buff, len);

	    /* remove any leading spaces */
	    cmd = stpblk(buff);
	    while(cmd[0]) {
		/* Split the command line */
		temp = strcspn(cmd," ");
		cmd[temp++] = 0x00;
		D(bug("[BootLdr] Init: Argument %s\n",cmd));

		/* Allocate node and insert into list */
		node = AllocMem(sizeof(struct Node),MEMF_ANY|MEMF_CLEAR);
		node->ln_Name = cmd;
		AddTail(&(BootLoaderBase->Args),node);
		/* Skip to next part */
		cmd = stpblk(cmd+temp);
	    }
		
	    BootLoaderBase->Flags |= BL_FLAGS_CMDLINE;
	}
    }

#if (AROS_FLAVOUR & AROS_FLAVOUR_STANDALONE)
    /* Get VESA mode information */
    vmi = (struct vbe_mode *)GetTagData(KRN_VBEModeInfo, 0, msg);
    vci = (struct vbe_controller *)GetTagData(KRN_VBEControllerInfo, 0, msg);
    D(bug("[BootLdr] VESA mode info 0x%p, controller info 0x%p\n", vmi, vci));

    if (vmi && vci) {
	BootLoaderBase->Vesa = AllocMem(sizeof(struct VesaInfo), MEMF_ANY);

	if (BootLoaderBase->Vesa) {
	    ULONG masks [] = {0x01, 0x03, 0x07, 0x0f ,0x1f, 0x3f, 0x7f, 0xff};

	    BootLoaderBase->Vesa->FrameBuffer = (APTR)vmi->phys_base;
	    BootLoaderBase->Vesa->FrameBufferSize = vci->total_memory * 64; /* FrameBufferSize is in KBytes! */
	    BootLoaderBase->Vesa->XSize = vmi->x_resolution;
	    BootLoaderBase->Vesa->YSize = vmi->y_resolution;
	    BootLoaderBase->Vesa->BytesPerLine = vmi->bytes_per_scanline;
	    BootLoaderBase->Vesa->BitsPerPixel = vmi->bits_per_pixel;
	    BootLoaderBase->Vesa->Masks [VI_Red]   = masks[vmi->red_mask_size-1]<<vmi->red_field_position;
	    BootLoaderBase->Vesa->Masks [VI_Blue]  = masks[vmi->blue_mask_size-1]<<vmi->blue_field_position;
	    BootLoaderBase->Vesa->Masks [VI_Green] = masks[vmi->green_mask_size-1]<<vmi->green_field_position;
	    BootLoaderBase->Vesa->Masks [VI_Alpha] = masks[vmi->reserved_mask_size-1]<<vmi->reserved_field_position;
	    BootLoaderBase->Vesa->Shifts[VI_Red]   = 32 - vmi->red_field_position - vmi->red_mask_size;
	    BootLoaderBase->Vesa->Shifts[VI_Blue]  = 32 - vmi->blue_field_position - vmi->blue_mask_size;
	    BootLoaderBase->Vesa->Shifts[VI_Green] = 32 - vmi->green_field_position - vmi->green_mask_size;
	    BootLoaderBase->Vesa->Shifts[VI_Alpha] = 32 - vmi->reserved_field_position - vmi->reserved_mask_size;

	    BootLoaderBase->Vesa->ModeNumber   = GetTagData(KRN_VBEMode, 3, msg);
	    BootLoaderBase->Vesa->PaletteWidth = GetTagData(KRN_VBEPaletteWidth, 6, msg);

	    D(bug("[BootLdr] Init: Vesa card capability flags: 0x%08lx\n", vci->capabilities));
	    D(bug("[BootLdr] Init: Vesa mode %x type (%dx%dx%d)\n", BootLoaderBase->Vesa->ModeNumber,
		  BootLoaderBase->Vesa->XSize, BootLoaderBase->Vesa->YSize, BootLoaderBase->Vesa->BitsPerPixel));
	    D(bug("[BootLdr] Init: Vesa FB at 0x%08x size %d kB\n", BootLoaderBase->Vesa->FrameBuffer,
		  BootLoaderBase->Vesa->FrameBufferSize));
	    D(bug("[BootLdr] Init: Vesa mode palette width: %d\n", BootLoaderBase->Vesa->PaletteWidth));
	    D(bug("[BootLdr] Init: Vesa mode direct color flags %02x\n", vmi->direct_color_mode_info));
	}
    }
#endif

    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
