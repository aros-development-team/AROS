/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Bootloader information initialisation.
*/

#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <utility/utility.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/bootloader.h>
#include <proto/utility.h>
#include <proto/kernel.h>

#include <aros/symbolsets.h>
#include <aros/bootloader.h>
#include <aros/kernel.h>
#include "../bootstrap/multiboot.h"
#include "bootloader_intern.h"
#include LC_LIBDEFS_FILE

#include <string.h>

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR BootLoaderBase)
{
    void *KernelBase = TLS_GET(KernelBase);
    struct TagItem *msg = KrnGetBootInfo();
    IPTR tmp;
    struct vbe_mode *vmi;
    struct vbe_controller *vci;

    BootLoaderBase->Flags = 0;
    
    NEWLIST(&(BootLoaderBase->Args));
    NEWLIST(&(BootLoaderBase->DriveInfo));

    D(bug("[BootLdr] Init. msg=%p\n", msg));
    
    /* Right. Now we extract the data currently placed in 0x1000 by exec */
    if (msg)
    {
	/* Yay. There is data here */
#if 0
        if (mb->flags & MB_FLAGS_LDRNAME)
	{
	    STRPTR temp;

	    temp = AllocMem(200,MEMF_ANY);
	    if (temp)
	    {
		strcpy(temp,mb->ldrname);
		BootLoaderBase->LdrName = temp;
		BootLoaderBase->Flags |= MB_FLAGS_LDRNAME;
		D(bug("[BootLdr] Init: Loadername = %s\n",BootLoaderBase->LdrName));
	    }
	    else
		bug("[BootLdr] Init: Failed to alloc memory for string\n");
	}
#endif
        tmp = GetTagData(KRN_CmdLine, 0, msg);
        D(bug("[BootLdr] KRN_CmdLine=%p\n", tmp));
	if (tmp)
	{
	    STRPTR cmd,buff;
	    ULONG temp;
	    struct Node *node;
	    
	    D(bug("[BootLdr] CmdLine=\"%s\"\n", (STRPTR)tmp));
	    
	    /* First make a working copy of the command line */
	    if ((buff = AllocMem(200,MEMF_ANY|MEMF_CLEAR)))
	    {
		strncpy(buff, (STRPTR)tmp, 200);
		/* remove any leading spaces */
		cmd = stpblk(buff);
		while(cmd[0])
		{
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
		
		BootLoaderBase->Flags |= MB_FLAGS_CMDLINE;
	    }
	}
	vmi = (struct vbe_mode *)GetTagData(KRN_VBEModeInfo, 0, msg);
	vci = (struct vbe_controller *)GetTagData(KRN_VBEControllerInfo, 0, msg);

	if (vmi && vci)
	{
	    ULONG masks [] = { 0x01, 0x03, 0x07, 0x0f ,0x1f, 0x3f, 0x7f, 0xff };
	    UWORD mode = GetTagData(KRN_VBEMode, 0, msg);
	    UBYTE palwidth = GetTagData(KRN_VBEPaletteWidth, 6, msg);

	    BootLoaderBase->Vesa.FrameBuffer = (APTR)vmi->phys_base;
	    BootLoaderBase->Vesa.FrameBufferSize = vci->total_memory * 64; /* FrameBufferSize is in KBytes! */
	    BootLoaderBase->Vesa.XSize = vmi->x_resolution;
	    BootLoaderBase->Vesa.YSize = vmi->y_resolution;
	    BootLoaderBase->Vesa.BytesPerLine = vmi->bytes_per_scanline;
	    BootLoaderBase->Vesa.BitsPerPixel = vmi->bits_per_pixel;
	    BootLoaderBase->Vesa.ModeNumber = mode; /* FIXME! */
	    BootLoaderBase->Vesa.Masks[VI_Red] = masks[vmi->red_mask_size-1]<<vmi->red_field_position;
	    BootLoaderBase->Vesa.Masks[VI_Blue] = masks[vmi->blue_mask_size-1]<<vmi->blue_field_position;
	    BootLoaderBase->Vesa.Masks[VI_Green] = masks[vmi->green_mask_size-1]<<vmi->green_field_position;
	    BootLoaderBase->Vesa.Masks[VI_Alpha] = masks[vmi->reserved_mask_size-1]<<vmi->reserved_field_position;
	    BootLoaderBase->Vesa.Shifts[VI_Red] = 32 - vmi->red_field_position - vmi->red_mask_size;
	    BootLoaderBase->Vesa.Shifts[VI_Blue] = 32 - vmi->blue_field_position - vmi->blue_mask_size;
	    BootLoaderBase->Vesa.Shifts[VI_Green] = 32 - vmi->green_field_position - vmi->green_mask_size;
	    BootLoaderBase->Vesa.Shifts[VI_Alpha] = 32 - vmi->reserved_field_position - vmi->reserved_mask_size;
	    BootLoaderBase->Vesa.PaletteWidth = palwidth;
	    BootLoaderBase->Flags |= MB_FLAGS_GFX;
	    if (BootLoaderBase->Vesa.ModeNumber != 3)
	    {
		D(bug("[BootLdr] Init: Vesa card capability flags: 0x%08lx\n", vci->capabilities));
	    	D(bug("[BootLdr] Init: Vesa mode %x type (%dx%dx%d)\n",
				BootLoaderBase->Vesa.ModeNumber,
				BootLoaderBase->Vesa.XSize,BootLoaderBase->Vesa.YSize,
				BootLoaderBase->Vesa.BitsPerPixel));
		D(bug("[BootLdr] Init: Vesa FB at 0x%08x size %d kB\n",
			    	BootLoaderBase->Vesa.FrameBuffer,
				BootLoaderBase->Vesa.FrameBufferSize));
		D(bug("[BootLdr] Init: Vesa mode palette width: %d\n", BootLoaderBase->Vesa.PaletteWidth));
		D(bug("[BootLdr] Init: Vesa mode direct color flags %02x\n", vmi->direct_color_mode_info));
	    }
	    else
	    {
		D(bug("[BootLdr] Init: Textmode graphics\n"));
	    }
	}
#if 0
	if (mb->flags & MB_FLAGS_DRIVES)
	{
	    struct mb_drive *curr;                                                                                                  
	    struct DriveInfoNode *node;

	    for (curr = (struct mb_drive *) mb->drives_addr;
		    (unsigned long) curr < mb->drives_addr + mb->drives_len;
		    curr = (struct mb_drive *) ((unsigned long) curr + curr->size))                                                 
	    {
		node = AllocMem(sizeof(struct DriveInfoNode),MEMF_ANY|MEMF_CLEAR);
		node->Number = curr->number;
		node->Mode = curr->mode;
		node->Cylinders = curr->cyls;
		node->Heads = curr->heads;
		node->Sectors = curr->secs;
		ADDTAIL(&(BootLoaderBase->DriveInfo),(struct Node *)node);

		D(bug("[BootLdr] Init: Drive %02x, CHS (%d/%d/%d) mode %s\n",
			curr->number,
			curr->cyls,curr->heads,curr->secs,
			curr->mode?"CHS":"LBA"));
	    }
	    BootLoaderBase->Flags |= MB_FLAGS_DRIVES;
	}
#endif
    }
    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
