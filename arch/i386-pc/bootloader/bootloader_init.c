/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Bootloader information initialisation.
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <utility/utility.h>
#include <proto/exec.h>
#include <proto/bootloader.h>

#include <aros/asmcall.h>
#include <aros/bootloader.h>
#include <aros/multiboot.h>
#include "bootloader_intern.h"
#include LC_LIBDEFS_FILE

#define DEBUG 1
#include <aros/debug.h>

#include <string.h>

#ifdef SysBase
#undef SysBase
#endif

static const UBYTE name[];
static const UBYTE version[];
static const void * const LIBFUNCTABLE[];
extern const char LIBEND;

struct BootLoaderBase *AROS_SLIB_ENTRY(init, BASENAME)();

extern void AROS_SLIB_ENTRY(GetBootInfo,BASENAME)();

int Bootloader_entry(void)
{
    return -1;
}

const struct Resident Bootloader_resident __attribute__((section(".text"))) =
{
    RTC_MATCHWORD,
    (struct Resident *)&Bootloader_resident,
    (APTR)&LIBEND,
    RTF_COLDSTART,
    41,
    NT_RESOURCE,
    100,
    (UBYTE *)name,
    (UBYTE *)&version[6],
    (ULONG *)&AROS_SLIB_ENTRY(init,BASENAME)
};

static const UBYTE name[] = NAME_STRING;
static const UBYTE version[] = VERSION_STRING;

static const void * const LIBFUNCTABLE[] =
{
    &AROS_SLIB_ENTRY(GetBootInfo,BASENAME),
    (void *)-1
};

AROS_UFH3(struct BootLoaderBase *, AROS_SLIB_ENTRY(init,BASENAME),
    AROS_UFHA(ULONG,	dummy,	D0),
    AROS_UFHA(ULONG,	slist,	A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT

    UWORD neg = AROS_ALIGN(LIB_VECTSIZE * 3);
    struct BootLoaderBase * BootLoaderBase = NULL;
    struct arosmb *mb = (struct arosmb *)0x1000;
    
    BootLoaderBase = (struct BootLoaderBase *)(((UBYTE *)
	AllocMem( neg + sizeof(struct BootLoaderBase),
		    MEMF_CLEAR | MEMF_PUBLIC)) + neg);

    if( BootLoaderBase )
    {
	BootLoaderBase->bl_SysBase = SysBase;
	BootLoaderBase->bl_UtilityBase = OpenLibrary("utility.library",0);
	BootLoaderBase->bl_Node.ln_Pri = 0;
	BootLoaderBase->bl_Node.ln_Type = NT_RESOURCE;
	BootLoaderBase->bl_Node.ln_Name = (STRPTR)name;
	NEWLIST(&(BootLoaderBase->Args));
	NEWLIST(&(BootLoaderBase->DriveInfo));

	MakeFunctions(BootLoaderBase, (APTR)LIBFUNCTABLE, NULL);
	AddResource(BootLoaderBase);
    }

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
	    BootLoaderBase->Vesa.FrameBufferSize = mb->vci.total_memory * 64;
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
    return BootLoaderBase;

    AROS_USERFUNC_EXIT
}
