/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: init.c 14808 2002-06-15 08:01:12Z bergers $
*/

#define AROS_USE_OOP

#include <aros/config.h>
#include <exec/io.h>
#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <proto/oop.h>
#include <proto/exec.h>

#include <devices/keyboard.h>
#define mybug(col,var) __asm__ __volatile__("move.l #"#col",0xf9002000+"#var"" :: ); 
//#define mybug(col,var)
#define DEBUG 1
#include <aros/debug.h>
#include <aros/core.h>
#include <asm/registers.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "traps.h"
#include <memory.h>

#include "exec_intern.h"
#include "etask.h"
#include "ep_info.h"

#if 1 
char tab[127];

#ifdef rkprintf
# undef rkprintf
#endif
#define rkprintf(x...)  scr_RawPutChars(tab, snprintf(tab,126, x))
#endif
extern void scr_RawPutChars(char *chr, int lim);

extern unsigned long _end;
struct mac68k_init_stuff init_stuff;
//extern struct init_stuff init_stuff;

extern struct ExecBase * PrepareExecBase(struct MemHeader *);
extern void switch_to_user_mode(void *, ULONG *);
extern void main_init_cont(void);
extern struct MemHeader * detect_memory(void);
extern void screen_init(void);
extern void fontinit(void);
void get_initial_epinfo(void);

struct bootldr_mem {
	unsigned long address;
	unsigned long size;
};

/*
 * Just to be sure all of these modules get linked into the 
 * final module, this structure should stay here, because
 * otherwise the linker might not take it if there is no
 * reference to a certain module at all.
 */
extern const struct Resident
//	Expansion_resident,
	Exec_resident;
#if 0
	Utility_resident,
	Aros_resident,
	OOP_resident,
	HIDD_resident,
	Mathieeesingbas_resident,
	irqHidd_resident,
	Graphics_resident,
	Layers_resident,
	Timer_resident,
	Misc_resident,
	Battclock_resident,
	Keyboard_resident,
	Gameport_resident,
	Keymap_resident,
	Input_resident,
	Intuition_resident,
	hiddgraphics_resident,
	displayHidd_resident,
	hiddserial_resident,
	mouseHidd_resident,
	Console_resident,
	TrackDisk_resident,
	ide_resident,
	Workbench_resident,
	Mathffp_resident,
	boot_resident,
	Dos_resident,
	LDDemon_resident,
	con_handler_resident,
	AFS_resident;
#endif

/* This list MUST be in the correct order (priority). */
static const struct Resident *romtagList[] =
{
//	&Expansion_resident,
	&Exec_resident
#if 0
	&Utility_resident,
	&Aros_resident,
	&Mathieeesingbas_resident,
	&OOP_resident,
	&HIDD_resident,
	&irqHidd_resident,
	&Graphics_resident,
	&Layers_resident,
	&Timer_resident,
	&Misc_resident,
	&Battclock_resident,
	&Keyboard_resident,
	&Gameport_resident,
	&Keymap_resident,
	&Input_resident,
	&Intuition_resident,	
	&hiddgraphics_resident,
	&displayHidd_resident,
	&hiddserial_resident,
	&Console_resident,
	&Workbench_resident,
	&Mathffp_resident,
	&boot_resident,
	&Dos_resident,
	&LDDemon_resident,
	&con_handler_resident
#endif
};

/************************************************************************************/

void processor_init(void)
{
#if 1
	/************ CPU setup *******************/
	/* switch to supervisor, Emile/Penguin already does it for us ? */

	__asm__ __volatile__("oriw #0x0700,%%sr" :: );
#endif
/* debug stuff */
#if 0
	__asm__ __volatile__ (			\
				"move.l #0xf9000080,%%a0\n" \
				"myloop:\n" \
				"move.l 0x00ff00ff,(%%a0)+\n" \
				"jmp myloop" \
				:: );
#endif
}

/************************************************************************************/
extern ULONG initial_ssp;
void sputc(char c);

void main_init(void)
{
	struct ExecBase *SysBase = NULL;
	ULONG * m68k_USP;
	UWORD * rom_ranges[] = {(UWORD *)0x1000 , (UWORD *)&_end, (UWORD *)~0};
	struct MemHeader * mh =  NULL;
	char *Processor_type[] = { "68020", "68030", "68040", "68060" };

	processor_init();

	get_initial_epinfo();
	screen_init();

	rkprintf("AROS - The Amiga Research OS - Mac68k\nCompiled %s\n\n",__DATE__);
	rkprintf("CPU: %s, MEM: %ld\n", Processor_type[init_stuff.cpu],	init_stuff.mem);
	rkprintf("MacModel: %ld\nVidBase: 0x%lx, VidDepth: %ld, ScreenWidth: %ld, ScreenHeight: %ld\n", \
			init_stuff.model, init_stuff.vidaddr, init_stuff.viddepth, init_stuff.vidwidth, init_stuff.vidheight);

#if 0	
	sputc('A');
	sputc('R');
	sputc('O');
	sputc('S');
#endif

	/*
	 * create entry for first chunk of system mem with information from the bootloader
	 */	

#define MEM_START ((unsigned long)&_end + 0x10000)

	rkprintf("MEM_START: 0x%lx\n, memchunksize[0]: 0x%lx\n", MEM_START, init_stuff.memchunksize[0]);

	mh=(struct MemHeader*)MEM_START;
	mh->mh_Node.ln_Succ    = NULL;
	mh->mh_Node.ln_Pred    = NULL;
	mh->mh_Node.ln_Type    = NT_MEMORY;
	mh->mh_Node.ln_Name    = "chip memory";
	mh->mh_Node.ln_Pri     = -5;
	mh->mh_Attributes      = MEMF_CHIP | MEMF_PUBLIC | MEMF_LOCAL | MEMF_24BITDMA |
	                         MEMF_KICK;
	mh->mh_First           = (struct MemChunk *)((UBYTE*)mh+MEMHEADER_TOTAL);
	mh->mh_First->mc_Next  = NULL;
	mh->mh_First->mc_Bytes = ((ULONG) init_stuff.memchunksize[0] - MEM_START) - MEMHEADER_TOTAL;

	mh->mh_Lower           = mh->mh_First;
	mh->mh_Upper           = (APTR)(init_stuff.memchunksize[0]);
	mh->mh_Free            = mh->mh_First->mc_Bytes;

	/*
	    We have to put somewhere in this function checking for ColdStart,
	    CoolStart and many other Exec vectors!
	 */

	/*
	   It is OK to place ExecBase here. Remember that interrupt table starts
	   at 0x0100UL address, so 4UL is quite safe.
	   Even with MP this addr is OK for ExecBase. We may write an int handler
	   which detects "read from 4UL" commands.
	 */

	rkprintf("PrepareExecBase\n");
	SysBase = (struct ExecBase*)PrepareExecBase(mh);
	rkprintf("SysBase = 4\n");
	*(APTR *)0x4 = SysBase;

	/*
	  Setup ChkBase (checksum for base ptr), ChkSum (for library)
	  SysBase+ChkBase should be -1 otherwise somebody has destroyed ExecBase!
	 */
 	SysBase->ChkBase=~(ULONG)SysBase;
#warning TODO: SysBase->ChkSum=.....
#if 0
	if(NULL == AllocAbs(&_end-0x1000+0x10000,(APTR)0x1000))
	{
		rkprintf("Kernel protection failed !!\n");
	}
#endif		
	if (NULL == (m68k_USP =(ULONG *)AllocMem(AROS_STACKSIZE,MEMF_PUBLIC)))  {
		do {} while(1);
	}
	m68k_USP = (ULONG *)(((ULONG)m68k_USP) + AROS_STACKSIZE);
	rkprintf("USP alloc\n");

	/*
	 * Allocate memory for the SSP. The SSP is already set
	 * but I need to AllocAbs() it so nobody else will step on this
	 * memory.
	 */
#if 1 
	if (NULL == AllocAbs(AROS_STACKSIZE, 
	                     (APTR)(initial_ssp+sizeof(ULONG)-AROS_STACKSIZE))) {
		D(bug("Alloc for SSP failed!\n"));
	}
	D(bug("SSP: %x\n",initial_ssp));
#endif
	//SysBase->ResModules=Exec_RomTagScanner(SysBase, rom_ranges);

	rkprintf("init core\n");
	/*
	 * Init the core
	 */
	init_core(SysBase);
	rkprintf("Init traps\n");
	Init_Traps();
	/*
	 * This is the last place where I am in supervisor mode.
	 * so let me switch into user mode and continue there.
	 * The user mode function will then call main_init_cont.
	 */
	rkprintf("going to UserMode\n");
	switch_to_user_mode(main_init_cont, m68k_USP);
}

void get_initial_epinfo(void)
{
	struct ep_entry *epinfo = (struct ep_entry *)&_end;
	UBYTE memchunks = 0;

	while(epinfo->tag != EP_LAST)
	{
		switch(epinfo->tag)
		{
			case EP_VADDR:
				init_stuff.vidaddr = epinfo->data[0];
				break;
			case EP_VDEPTH:
				init_stuff.viddepth = epinfo->data[0];
				break;
			case EP_VROW:
				init_stuff.vidrow = epinfo->data[0];
				break;
			case EP_VDIM:
				init_stuff.vidwidth = epinfo->data[0] & 0xffff;
				init_stuff.vidheight = epinfo->data[0] >> 16;
				break;
			case EP_MODEL:
				init_stuff.model = epinfo->data[0];
				break;
			case EP_CPUID:
				init_stuff.cpu = epinfo->data[0];
				break;
			case EP_MEMSIZE:
				init_stuff.mem = epinfo->data[0];
				break;
			case EP_MEMCHUNK:
				init_stuff.memchunk[memchunks] = epinfo->data[0];
				init_stuff.memchunksize[memchunks] = epinfo->data[1];
				memchunks++;
				break;
			default:
				break;
		}
		epinfo = (struct ep_entry *)((unsigned long)epinfo+epinfo->size);
	}
}
/*
 * The following function will be executed when AROS is in user mode
 */
void main_init_cont(void)
{
	AROS_GET_SYSBASE
	InitCode(RTF_SINGLETASK, 0);

	/*
	   All done. In normal cases CPU should never reach this point
	 */

*(ULONG *)0xc0debad0 = 0;

}
