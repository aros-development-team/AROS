/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
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

#include <aros/core.h>
#include <asm/registers.h>

#include "memory.h"
#include "traps.h"
#include <memory.h>

#include "exec_intern.h"
#include "etask.h"

extern struct ExecBase * PrepareExecBase(struct MemHeader *);
extern void switch_to_user_mode(void *, ULONG *, ULONG *);
extern void main_init_cont(void);


/*
 * Just to be sure all of these modules get linked into the 
 * final module, this structure should stay here, because
 * otherwise the linker might not take it if there is no
 * reference to a certain module at all.
 */
extern const struct Resident
	Expansion_resident,
	Exec_resident,
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


/* This list MUST be in the correct order (priority). */
static const struct Resident *romtagList[] =
{
	&Expansion_resident,
	&Exec_resident,
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
};

/************************************************************************************/

void processor_init(void)
{
	/************ CPU setup *******************/
	__asm__ __volatile__("oriw #0x0700,%%sr" :: );

	/************ LCD Controller **************/
	/*
	 * Turn the LCD controller on
	 */
	WREG_B(PFDATA) = 0x010;

	WREG_L(LSSA)  = 0x90000;
	WREG_B(LVPW)  = 160/16;
	WREG_W(LXMAX) = 160-1;
	WREG_W(LYMAX) = 160-1;
	WREG_W(LCXP)  = 80;
	WREG_W(LCYP)  = 80;
	WREG_W(LCWCH) = (10 << 8) | 10;
	WREG_B(LBLKC) = 0x80 | 0x10;
	WREG_B(LPICF) = 0x4;

#if 0 
	// DO NOT ACTIVATE THESE! IT STOPS OUTPUT ON XCOPILOT!!
	WREG_B(LPOLCF)= 0x0;
	WREG_B(LACDRC)= 0x0;
	WREG_B(LPXCD) = 0x0;
	WREG_B(LCKCON)= 0x0;
	WREG_W(LRRA)  = 0x0;
#endif
	WREG_B(LPOSR) = 0x0;
	WREG_B(LOTCR) = 0x4e; // 0xfffffa2b

	/************* Interrupt Controller **********/
	WREG_L(IMR) = ~((1 << 1) | (1 << 5));
	WREG_B(IVR) = 0x40;
}

/************************************************************************************/


void main_init(void * memory, ULONG memSize)
{
	struct ExecBase *SysBase = NULL;
	ULONG * m68k_USP, * m68k_SSP;
	struct MemHeader *mh = NULL;
	UWORD * ranges[] = {0x10c00000 , 0x10c00000 + 1024 * 1024,
	                    -1};

	processor_init();
	/*
	   Prepare first memory list. Our memory will start at 'memory'.
	 */

	mh=(struct MemHeader*)memory;
	mh->mh_Node.ln_Type    = NT_MEMORY;
	mh->mh_Node.ln_Name    = "chip memory";
	mh->mh_Node.ln_Pri     = -5;
	mh->mh_Attributes      = MEMF_CHIP | MEMF_PUBLIC | MEMF_LOCAL | MEMF_24BITDMA |
	                         MEMF_KICK;
	mh->mh_First           = (struct MemChunk *)((UBYTE*)mh+MEMHEADER_TOTAL);
	mh->mh_First->mc_Next  = NULL;
	mh->mh_First->mc_Bytes = memSize - MEMHEADER_TOTAL;
	mh->mh_Lower           = mh->mh_First;
	mh->mh_Upper           = (APTR)(memory + memSize);
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

	SysBase = (struct ExecBase*)PrepareExecBase(mh);
	*(APTR *)0x4 = SysBase;

	/*
	  Setup ChkBase (checksum for base ptr), ChkSum (for library)
	  SysBase+ChkBase should be -1 otherwise somebody has destroyed ExecBase!
	 */
 	SysBase->ChkBase=~(ULONG)SysBase;
#warning TODO: SysBase->ChkSum=.....

	if (NULL == (m68k_USP =(ULONG *)AllocMem(AROS_STACKSIZE,MEMF_PUBLIC)))  {
		do {} while(1);
	}
	m68k_USP = (ULONG *)(((ULONG)m68k_USP) + AROS_STACKSIZE);

	SysBase->ResModules=Exec_RomTagScanner(SysBase, ranges);
	
	/*
	 * Get some memory for the SSP. 
	 */
	if (NULL == (m68k_SSP =(ULONG *)AllocMem(AROS_STACKSIZE,MEMF_PUBLIC)))  {
		do {} while(1);
	}
	m68k_SSP = (ULONG *)(((ULONG)m68k_SSP) + AROS_STACKSIZE);

	/*
	 * Init the core
	 */
	init_core(SysBase);
	Init_Traps();
	/*
	 * This is the last place where I am in supervisor mode.
	 * so let me switch into user mode and continue there.
	 * The user mode function will then call main_init_cont.
	 */
	switch_to_user_mode(main_init_cont, m68k_SSP, m68k_USP);
}

/*
 * The following function will be executed whan AROS is in user mode
 */
void main_init_cont(void)
{
	struct ExecBase * SysBase;
	SysBase = (struct ExecBase *)(*(ULONG *)0x04);
	InitCode(RTF_SINGLETASK, 0);

	/*
	   All done. In normal cases CPU should never reach this point
	 */

*(ULONG *)0xc0debad0 = 0;

}
