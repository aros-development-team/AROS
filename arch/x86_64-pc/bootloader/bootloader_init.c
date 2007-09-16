/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id: bootloader_init.c 25411 2007-03-12 06:54:19Z sonic $

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
#include <proto/oop.h>

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
    
    NEWLIST(&(BootLoaderBase->Args));
    NEWLIST(&(BootLoaderBase->DriveInfo));

    D(bug("[BootLdr] Init\n"));
    
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
	if (tmp)
	{
	    STRPTR cmd,buff;
	    ULONG temp;
	    struct Node *node;
	    
	    /* First make a working copy of the command line */
	    if ((buff = AllocMem(200,MEMF_ANY|MEMF_CLEAR)))
	    {
		strcpy(buff, (STRPTR)tmp);
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
#if 0
	if (mb->flags & MB_FLAGS_GFX)
	{
	    ULONG masks [] = { 0x01, 0x03, 0x07, 0x0f ,0x1f, 0x3f, 0x7f, 0xff };

	    BootLoaderBase->Vesa.FrameBuffer = (APTR)mb->vmi.phys_base;
	    BootLoaderBase->Vesa.FrameBufferSize = mb->vci.total_memory * 64; /* FrameBufferSize is in KBytes! */
	    BootLoaderBase->Vesa.XSize = mb->vmi.x_resolution;
	    BootLoaderBase->Vesa.YSize = mb->vmi.y_resolution;
	    BootLoaderBase->Vesa.BytesPerLine = mb->vmi.bytes_per_scanline;
	    BootLoaderBase->Vesa.BitsPerPixel = mb->vmi.bits_per_pixel;
	    BootLoaderBase->Vesa.ModeNumber = mb->vbe_mode;
	    BootLoaderBase->Vesa.Masks[VI_Red] = masks[mb->vmi.red_mask_size-1]<<mb->vmi.red_field_position;
	    BootLoaderBase->Vesa.Masks[VI_Blue] = masks[mb->vmi.blue_mask_size-1]<<mb->vmi.blue_field_position;
	    BootLoaderBase->Vesa.Masks[VI_Green] = masks[mb->vmi.green_mask_size-1]<<mb->vmi.green_field_position;
	    BootLoaderBase->Vesa.Masks[VI_Alpha] = masks[mb->vmi.reserved_mask_size-1]<<mb->vmi.reserved_field_position;
	    BootLoaderBase->Vesa.Shifts[VI_Red] = 32 - mb->vmi.red_field_position - mb->vmi.red_mask_size;
	    BootLoaderBase->Vesa.Shifts[VI_Blue] = 32 - mb->vmi.blue_field_position - mb->vmi.blue_mask_size;
	    BootLoaderBase->Vesa.Shifts[VI_Green] = 32 - mb->vmi.green_field_position - mb->vmi.green_mask_size;
	    BootLoaderBase->Vesa.Shifts[VI_Alpha] = 32 - mb->vmi.reserved_field_position - mb->vmi.reserved_mask_size;
	    BootLoaderBase->Vesa.PaletteWidth = mb->vbe_palette_width;
	    BootLoaderBase->Flags |= MB_FLAGS_GFX;
	    if (BootLoaderBase->Vesa.ModeNumber != 3)
	    {
		D(bug("[BootLdr] Init: Vesa card capability flags: 0x%08lx\n", mb->vci.capabilities));
	    	D(bug("[BootLdr] Init: Vesa mode %x type (%dx%dx%d)\n",
				BootLoaderBase->Vesa.ModeNumber,
				BootLoaderBase->Vesa.XSize,BootLoaderBase->Vesa.YSize,
				BootLoaderBase->Vesa.BitsPerPixel));
		D(bug("[BootLdr] Init: Vesa FB at 0x%08x size %d kB\n",
			    	BootLoaderBase->Vesa.FrameBuffer,
				BootLoaderBase->Vesa.FrameBufferSize));
		D(bug("[BootLdr] Init: Vesa mode palette width: %d\n", BootLoaderBase->Vesa.PaletteWidth));
		D(bug("[BootLdr] Init: Vesa mode direct color flags %02x\n", mb->vmi.direct_color_mode_info));
	    }
	    else
	    {
		D(bug("[BootLdr] Init: Textmode graphics\n"));
	    }
	}

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
