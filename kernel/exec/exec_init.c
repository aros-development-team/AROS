/*
    Copyright C 2003, The AROS Development Team. All rights reserved.
    $Id$

    Desc: exec.library startup code
    Lang: English
*/

/*
    Allthough this file exists inside PPC config code, it's really hardware
    independant code and should be more commonly spreaded among different
    platforms. It could be even common exec startup code.

    Please note, that this kernel version doesn't support restoring data from
    previous session interrupted by ColdReboot() call. In order to support that
    both ColdReboot() definition and this kernel has to be re-worked.

    NOTE (for hosted versions only):
	Your bootstrap code has to provide full MultiBoot header, MMAP section
	is mandatory. It has to point to memory area allocated for AROS. If
	not passed, Exec will assume, that usable memory starts at 0x00000000
*/

#include <aros/machine.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/resident.h>
#include <exec/memory.h>

#include <proto/exec.h>

/*
    Exec startup is our kernel start entry which is executed by proper loader.
    It may pass MultiBoot-compilant data in order to allow exec work properly.
*/

#include <aros/multiboot.h>

#include "startup.h"

UBYTE SECTION_CODE Exec_LibraryName[] = "exec.library";
UBYTE SECTION_CODE Exec_Version[] = "$VER: exec generic 41.1 (3.5.2003)\r\n";
UWORD SECTION_CODE Exec_Ver = 41;
UWORD SECTION_CODE Exec_Rev = 1;

/*
    First writable configuration memory area. The RomTagRages may contain up to
    8 entries, each describing beginning and end of memory area to scan looking
    for Resident modules. If you modify default config, do not forget to end it
    with -1 pointer.
*/

UWORD SECTION_CODE *RomTagRanges[2*8 + 1] = {
    (UWORD*)startup, (UWORD*)&_END, (UWORD*)~0
};

struct Resident Exec_Resident SECTION_CODE = {
    RTC_MATCHWORD,
    &Exec_Resident,
    (UBYTE*)&Exec_End,
    0,
    41,
    NT_LIBRARY,
    126,
    (UBYTE*)Exec_LibraryName,
    (UBYTE*)Exec_Version,
    startup
};

/*
    Gee! Static data? Not really. This one is created before kernel is write-
    protected. It may allthough go somewhere into RAM even, if kernel is placed
    in ROM memory.
    
    MultiBoot_Storage is 4KB long. It should be rather enough to store all data
    passed by bootstrap code. It may shrink in future even more.
*/

ULONG MultiBoot_Storage[1024] = {0, };

#ifdef SysBase
#undef SysBase
#endif
#define SysBase ((struct ExecBase*)ExecBase)

void STDCALL NORETURN LaunchKernel(ULONG magic, ULONG addr)
{
    struct ExecBase *ExecBase = (struct ExecBase*)0UL;
    struct multiboot *mbinfo;
    struct arosmb *arosmb = (struct arosmb*)MultiBoot_Storage;
    int i;

    /*
	If magic passed to kernel is inproper, it may be ColdReboot() thing.
	Then just verify that arosmb->magic field is valid. If so, then all
	data passed by bootstrap was properly saved.

	If magic is proper, kernel was launched with proper MultiBoot data
	and we have to parse them.

	If both magic field and arosmb->magic are wrong, we have undefined
	situation and may either reboot or halt forever here.
    */
    
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC && arosmb->magic == MBRAM_VALID)
    {
	/*
	    Handle proper reboot procedure
	*/
    }
    else if (magic == MULTIBOOT_BOOTLOADER_MAGIC && addr != NULL)
    {
	mbinfo = (struct multiboot*)addr;
	arosmb->magic = MBRAM_VALID;
	arosmb->flags = 0L;

	if (mbinfo->flags && MB_FLAGS_MEM)
	{
	    arosmb->flags |= MB_FLAGS_MEM;
	    arosmb->mem_lower = mbinfo->mem_lower;
	    arosmb->mem_upper = mbinfo->mem_upper;
	}
	if (mbinfo->flags && MB_FLAGS_LDRNAME)
	{
	    arosmb->flags |= MB_FLAGS_LDRNAME;
	    strncpy(arosmb->ldrname, mbinfo->loader_name, 29);
	}
	if (mbinfo->flags && MB_FLAGS_CMDLINE)
	{
	    arosmb->flags |= MB_FLAGS_CMDLINE;
	    strncpy(arosmb->cmdline, mbinfo->cmdline, 199);
	}
	if (mbinfo->flags && MB_FLAGS_MMAP)
	{
	    arosmb->flags |= MB_FLAGS_MMAP;
	    arosmb->mmap_addr = ((ULONG)((ULONG)arosmb + sizeof(struct arosmb)));
	    arosmb->mmap_len = mbinfo->mmap_length;
	    memcpy((APTR)arosmb->mmap_addr, 
		    (APTR)mbinfo->mmap_addr,
		    mbinfo->mmap_length);
	}
	if (mbinfo->flags && MB_FLAGS_DRIVES)
	{
	    if (mbinfo->drives_length > 0)
	    {
		arosmb->flags |= MB_FLAGS_DRIVES;
		if (arosmb->flags && MB_FLAGS_MMAP)
		{
		    arosmb->drives_addr = 
			((ULONG)arosmb->mmap_addr + arosmb->mmap_len);
		}
		else
		{
		    arosmb->drives_addr = 
			((ULONG)arosmb + sizeof(struct arosmb));
		}
		arosmb->drives_len = mbinfo->drives_length;
		memcpy((APTR)arosmb->drives_addr,
			(APTR)mbinfo->drives_addr,
			mbinfo->drives_length);
	    }
	}
    }
    else
    {
	/* Undefined situation. You may halt here only... */
	do {} while(1);
    }

    /*
	Did we already defined ExecBase address? It might happen before, when
	we discovered ColdReboot() and found usable ExecBase. If no address
	assigned, do it now.
    */
    if (ExecBase == NULL)
    {
	/* MMAP information is favorized always! */
	if (arosmb->flags && MB_FLAGS_MMAP)
	{
	    struct mb_mmap *mmap = (struct mb_mmap*)arosmb->mmap_addr;
	    
	    /* Iterate through mmaps and find first RAM area */
	    while(mmap->type != MMAP_TYPE_RAM) mmap++;
	    
	    /* For kernel purpouses, leave first 16KB free. */
	    if (mmap->addr_low < 0x4000) {
		ExecBase = (struct ExecBase *)0x4000;
	    } else {
		ExecBase = (struct ExecBase *)mmap->addr_low;
	    }
	}
	else
	{
	    /*
		No mmap given? Assume then, that RAM starts at absolute
		addres 0x00000000, and place ExecBase at 16th KB of mem
	    */
	    ExecBase = (struct ExecBase *)0x4000;
	}

	/* Move ExecBase so you may put functions table */
#warning WARNING: Assuming 150 exec functions!
	(ULONG)ExecBase += 150 * LIB_VECTSIZE;
	*(struct ExecBase**)4UL = ExecBase;
    }

    /* Prevent Kick* pointers from beeing deleted */
    APTR
	KickMemPtr = ExecBase->KickMemPtr,
	KickTagPtr = ExecBase->KickTagPtr,
	KickCheckSum = ExecBase->KickCheckSum;

    /* Clear most of ExecBase now */
    bzero(&ExecBase->IntVects[0],
	sizeof(struct ExecBase) - offsetof(ExecBase, IntVects[0]));

    /* Restore Kick* fields */
    ExecBase->KickMemPtr = KickMemPtr;
    ExecBase->KickTagPtr = KickTagPtr;
    ExecBase->KickCheckSum = KickCheckSum;

    /*
	Calculate checksum for ExecBase location. It may be used by reboot, to
	prove LibBase validity.
    */
    
    ExecBase->ChkBase = ~(ULONG)ExecBase;

    /*
	Initialize system-wide lists
    */

    NEWLIST(&SysBase->MemList);
    SysBase->MemList.lh_Type = NT_MEMORY;
    NEWLIST(&SysBase->ResourceList);
    SysBase->ResourceList.lh_Type = NT_RESOURCE;
    NEWLIST(&SysBase->DeviceList);
    SysBase->DeviceList.lh_Type = NT_DEVICE;
    NEWLIST(&SysBase->LibList);
    SysBase->LibList.lh_Type = NT_LIBRARY;
    NEWLIST(&SysBase->PortList);
    SysBase->PortList.lh_Type = NT_MSGPORT;
    NEWLIST(&SysBase->TaskReady);
    SysBase->TaskReady.lh_Type = NT_TASK;
    NEWLIST(&SysBase->TaskWait);
    SysBase->TaskWait.lh_Type = NT_TASK;
    NEWLIST(&SysBase->IntrList);
    SysBase->IntrList.lh_Type = NT_INTERRUPT;
    NEWLIST(&SysBase->SemaphoreList);
    SysBase->SemaphoreList.lh_Type = NT_SIGNALSEM;
    NEWLIST(&SysBase->ex_MemHandlers);

    for (i=0; i < 5; i++)
    {
	NEWLIST(&SysBase->SoftInts[i].sh_List);
	SysBase->SoftInts[i].sh_List.lh_Type = NT_SOFTINT;
    }

    /*
	Lists are already done, prepare ExecBase values
    */

    SysBase->TaskSigAlloc = 0x0000ffff;
    SysBase->TaskTrapAlloc = 0x8000;
    SysBase->TaskTrapCode = NULL;	/* These three fields will be filled */
    SysBase->TaskExceptCode = NULL;	/* By kernel.resource */
    SysBase->TaskExitCode = NULL;

    SysBase->LibNode.lib_Node.ln_Type = NT_LIBRARY;
    SysBase->LibNode.lib_Node.ln_Pri = 0;
    SysBase->LibNode.lib_Node.ln_Name = (UBYTE*)Exec_LibraryName;
    SysBase->LibNode.lib_Flags = LIBF_CHANGED | LIBF_SUMUSED;
    SysBase->LibNode.lib_PosSize = sizeof(struct ExecBase);
    SysBase->LibNode.lib_OpenCnt = 1;
    SysBase->LibNode.lib_IdString = (UBYTE*)Exec_Version;
    SysBase->LibNode.lib_Version = Exec_Ver;
    SysBase->LibNode.lib_Revision = Exec_Rev;

    /*
	NOTE:
	    On some architectures, VBlankFrequency will be used at init by
	    kernel.resource to set up interrupt timer properly.
    */
    SysBase->Quantum = 4;
    SysBase->VBlankFrequency = 200; /* Use 200Hz timer by default */
    
    /*
	Make SysBase functions
    */
    SysBase->LibNode.lib_NegSize =
	AROS_UFC4(UWORD, Exec_MakeFunctions,
	    AROS_UFCA(APTR, ExecBase, A0),
	    AROS_UFCA(APTR, ExecFunctions, A1),
	    AROS_UFCA(APTR, NULL, A2),
	    AROS_UFCA(struct ExecBase *, ExecBase, A6));
    
    if (arosmb->flags && MB_FLAGS_MMAP)
    {
	int i=0;
	struct mb_mmap *map = (struct mb_mmap*)arosmb->mmap_addr;

	while (i < arosmb->mmap_len)
	{
	    ULONG flags;

	    if (map->type == MMAP_TYPE_RAM)
	    {
		if ((map->addr_low < 0x01000000) &&
		    ((map->addr_low + map->len_low) < 0x01000000))
		{
		    if (map->addr_low <= 
			    ((ULONG)ExecBase + sizeof(struct ExecBase)))
		    {
			map->addr_low = (ULONG)ExecBase + sizeof(struct ExecBase);
			map->addr_low = (map->addr_low + 31) & ~31;
			map->len_low -= map->addr_low;
		    }
		    AddMemList(map->len_low,
			MEMF_CHIP | MEMF_PUBLIC | MEMF_KICK |
			    MEMF_LOCAL | MEMF_24BITDMA,
			-10,
			(APTR)map->addr_low,
			(STRPTR)"dma memory");
		}
		else
		{
		    AddMemList(map->len_low,
			MEMF_FAST | MEMF_PUBLIC | MEMF_KICK | MEMF_LOCAL,
			0,
			(APTR)map->addr_low,
			(STRPTR)"fast memory");
		}
	    }
	    i++;
	    map++;
	}
    }
    else
    {
	ULONG addr = (ULONG)ExecBase + sizeof(struct ExecBase);
	addr = (addr + 31) & ~31;

#warning WARNING: look out - no mmap structures!

	AddMemList(arosmb->mem_lower - addr,
	    MEMF_CHIP | MEMF_PUBLIC | MEMF_KICK |
		MEMF_LOCAL | MEMF_24BITDMA,
	    -10,
	    (APTR)addr,
	    (STRPTR)"dma memory");
	
	AddMemList(arosmb->mem_upper - (ULONG)&_END,
	    MEMF_FAST | MEMF_PUBLIC | MEMF_KICK | MEMF_LOCAL,
	    0,
	    (APTR)&_END,
	    (STRPTR)"fast memory");
    }
    
    SumLibrary((struct Library*)SysBase);
    Enqueue(&SysBase->LibList, &SysBase->LibNode.lib_Node);

    /*
	At this point, we do have more or less valid exec.library. Not, it's
	time to scan for other resident modules. Some of them have to be 
	initialized implictly.
    */

    SysBase->ResModules = RomTagScanner(SysBase, &RomTagRanges);

    /*
	Initialize kernel.resource!

	kernel.resource will add some functionality to the working system. It
	will also patch exec.library properly, in order to implement CPU and
	hardware specific things.
    */
    InitResident(FindResident("kernel.resource"), NULL);

    
    do {} while(1);
}

