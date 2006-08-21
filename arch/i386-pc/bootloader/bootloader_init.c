/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Bootloader information initialisation.
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <utility/utility.h>
#include <proto/exec.h>
#include <proto/bootloader.h>

#include <aros/symbolsets.h>
#include <aros/bootloader.h>
#include <aros/multiboot.h>
#include "bootloader_intern.h"
#include LC_LIBDEFS_FILE

#define DEBUG 1
#include <aros/debug.h>

#include <string.h>

#define MB_MAGIC    0x1BADB002  /* Magic value */
#define MB_FLAGS    0x00000003  /* Need 4KB alignment for modules */

const struct
{
    ULONG   magic;
    ULONG   flags;
    ULONG   chksum;
} multiboot_header __attribute__((section(".text"))) =
{
    MB_MAGIC,
    MB_FLAGS,
    -(MB_MAGIC+MB_FLAGS)
};

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR BootLoaderBase)
{
    struct arosmb *mb = (struct arosmb *)0x1000;
    
    NEWLIST(&(BootLoaderBase->Args));
    NEWLIST(&(BootLoaderBase->DriveInfo));

    /* Right. Now we extract the data currently placed in 0x1000 */
    if (mb->magic == MBRAM_VALID)
    {
	/* Yay. There is data here */
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
	if (mb->flags & MB_FLAGS_CMDLINE)
	{
	    STRPTR cmd,buff;
	    ULONG temp;
	    struct Node *node;
	    
	    /* First make a working copy of the command line */
	    if ((buff = AllocMem(200,MEMF_ANY|MEMF_CLEAR)))
	    {
		strcpy(buff,mb->cmdline);
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
	    BootLoaderBase->Flags |= MB_FLAGS_GFX;
	    if (BootLoaderBase->Vesa.ModeNumber != 3)
	    {
	    	D(bug("[BootLdr] Init: Vesa mode %x type (%dx%dx%d)\n",
				BootLoaderBase->Vesa.ModeNumber,
				BootLoaderBase->Vesa.XSize,BootLoaderBase->Vesa.YSize,
				BootLoaderBase->Vesa.BitsPerPixel));
		D(bug("[BootLdr] Init: Vesa FB at 0x%08x size %d kB\n",
			    	BootLoaderBase->Vesa.FrameBuffer,
				BootLoaderBase->Vesa.FrameBufferSize));
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
    }
    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
