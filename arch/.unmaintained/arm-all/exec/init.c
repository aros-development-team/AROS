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

#define DEBUG 1
#include <aros/debug.h>
#include <aros/core.h>
#include <asm/registers.h>
#include <asm/cpu.h>

#include "memory.h"
//#include "traps.h"
#include <memory.h>

#include "exec_intern.h"
#include "etask.h"

#include "arm_exec_internal.h"


/*
 * local static functions.
 */
static void main_init_cont(void);


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
	kbdHidd_resident,
	Console_resident,
	TrackDisk_resident,
	ide_resident,
	Workbench_resident,
	Mathffp_resident,
	boot_resident,
	Dos_resident,
	LDDemon_resident,
	con_handler_resident,
	ram_handler_resident,
	cram_handler_resident,
	nil_handler_resident,
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
	&Timer_resident,      // CRASHES
	&Misc_resident,
	&Battclock_resident,
	&Keyboard_resident,
	&Gameport_resident,
	&Keymap_resident,
	&Input_resident,     // CRASHES
	&Intuition_resident, // CRASHES
//	&hiddgraphics_resident,
//	&displayHidd_resident,
	&hiddserial_resident,
//	&Console_resident,   // CRASHES
	&Workbench_resident,
	&Mathffp_resident,
	&boot_resident,
	&Dos_resident,
	&LDDemon_resident,
	&con_handler_resident,
	&ram_handler_resident,
	&nil_handler_resident,
//	&cram_handler_resident
};

/************************************************************************************/

void processor_init(void)
{
	/************ CPU setup *******************/

	/*** Turn MMU off *************************/
#if 0
	/* cannot do that... */
	__asm__ __volatile__ ("mov %%r0,#(0x2|0x10|0x20)\n\t
	                       mrc p15,0,%%r1,c1,c0,0\n\t
	                       and %r1,%r1,%r0\n\t
	                       mcr p15,0,%%r1,c1,c0,0\n\t" \
	                       : \
	                       : \
	                       : "%r0", "%r1" );
#endif
	/*** Turn caching off (for now) ***********/
	__asm__ __volatile__ ("nop	\n\t
	                       mrc	p15,0,r0,c1,c0,0\n\t
	                       bic	r0,r0,#12\n\t
	                       mcr	p15,0,r0,c1,c0,0\n\t" \
	                      : \
	                      : \
	                      : "%r0", "%r1" );
	/*** Do not allow interrupts */

	/************ LCD Controller **************/
	/*
	 * Turn the LCD controller on
	 */

	/************* Interrupt Controller **********/
	WREG_L(ICMR) = 0;
	WREG_L(ICLR) = 0;
	WREG_L(ICCR) = 0x1;
	
	/************** OS Timer *********************/
	WREG_L(OSCR)  = 0;
	WREG_L(OSMR0) = 3686400 / 2 ;
	WREG_L(OSMR1) = 0;
	WREG_L(OSMR2) = 0;
	WREG_L(OSMR3) = ~0;
	WREG_L(OSSR)  = 0xf;
	WREG_L(OIER)  = 0x0; // disable timers
	WREG_L(OWER)  = 0x0;
	
	/************* Memory Controller *************/
}

/************************************************************************************/

#define DO_SERIAL_DEBUG

#ifdef DO_SERIAL_DEBUG
static void init_serial(void)
{
	/*
	 * Set the UART3 to 115200 baud
	 */
	WREG_L(UTCR0)   = 0x8;
	WREG_L(UTCR1)   = 0x0;
	WREG_L(UTCR2)   = 0x1;
	WREG_L(UTCR3)   = 0x3;
	WREG_L(UTDR)    = 0xD;
	WREG_L(UTSR0)   = 0x0;
	WREG_L(UTSR1)   = 0x1;
}


/*static*/ void print_ser(char * string)
{
	int i = 0;
	while (0 != string[i]) {
		ULONG j = 0;
		volatile ULONG utsr1;
		do {
			utsr1 = RREG_L(UTSR1);
			j++;
		} while ((0 == (utsr1 & 0x04)) && (j < 1000000));
		WREG_L(UTDR) = (ULONG)string[i];
		i++;
	}
}

void print_serial(char * string)
{
	print_ser(string);
	print_ser("\r\n");
}
#endif

/************************************************************************************/

void mmu_lookup(ULONG addr, struct ExecBase * SysBase)
{
		ULONG * ttb = (ULONG *)(get_cp15_r2() & 0xffffc000);
		ULONG * fld;
		D(bug("translation table base at %p\n",ttb));
		ttb = (ULONG *)((ULONG)ttb | ((addr & 0xfff00000) >> 18));
		D(bug("first level descriptor for address %p at %p (%p)\n",addr,*ttb,ttb));
		fld = *ttb;
		D(bug("Content of first level descriptor: %x\n",*fld));
		if (0x2 == ((*fld) & 0x2)) {
			D(bug("This is a section entry!\n"));
			D(bug("B : %x\n",(*fld >>  2) & 1 ));
			D(bug("C : %x\n",(*fld >>  3) & 1 ));
			D(bug("AP: %x\n",(*fld >> 10) & 3 ));
			D(bug("Section base address: %x\n",(*fld) & 0xfff00000 ));
		} else if (0x1 == ((*fld) & 0x2)) {
			D(bug("This is a page entry\n"));
		} else {
			D(bug("Wrong entry!?\n"));
		}
}

/************************************************************************************/
extern ULONG initial_ssp;

extern ULONG _binary_rom_disk_start;


void main_init(void)
{
	struct ExecBase *SysBase = NULL;
	ULONG * arm_SP_User = 0xbad0c0de;
	ULONG * arm_SP_IRQ  = 0xbad0c0de;
	ULONG * arm_SP_FIQ  = 0xbad0c0de;
	ULONG * arm_SP_Abort = 0xbad0c0de;
	ULONG * arm_SP_Undef = 0xbad0c0de;
#if 0
	UWORD * rom_ranges[] = {(UWORD *)0xc0000000 , (UWORD *)0xc0000000 + (2 * 1024 * 1024),
	                        (UWORD *)~0};
#else
	UWORD * rom_ranges[] = {(UWORD *)0x90000 , (UWORD *)0xfff00,
	                        (UWORD *)~0};
#endif
#define MAX_MEM_HEADERS 10
	struct MemHeader * mh =  NULL;

	processor_init();
#ifdef DO_SERIAL_DEBUG
	init_serial();
	print_serial("Serial Port initialized!\n");
#endif

	/*
	 * detect memory of the system
	 */
	print_serial("Detecting memory now\n");
	mh = detect_memory();

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
	print_serial("preparing execbase now\n");
	SysBase = (struct ExecBase*)PrepareExecBase(mh);
	*(APTR *)0x4 = SysBase;
	/*
	 * Detect the rest of the memory...
	 */
	print_serial("detecting rest of memory now\n");
	detect_memory_rest(SysBase);

	/*
	  Setup ChkBase (checksum for base ptr), ChkSum (for library)
	  SysBase+ChkBase should be -1 otherwise somebody has destroyed ExecBase!
	 */
 	SysBase->ChkBase=~(ULONG)SysBase;
#warning TODO: SysBase->ChkSum=.....

	if (NULL == (arm_SP_User =(ULONG *)AllocMem(AROS_STACKSIZE,MEMF_PUBLIC)))  {
		do {} while(1);
	}
	arm_SP_User = (ULONG *)(((ULONG)arm_SP_User) + AROS_STACKSIZE);

	/*
	 * Allocate memory for the SSP. The SSP is already set
	 * but I need to AllocAbs() it so nobody else will step on this
	 * memory.
	 */
#if 0
	if (NULL == AllocAbs(AROS_STACKSIZE, 
	                     (APTR)(initial_ssp+sizeof(ULONG)-AROS_STACKSIZE))) {
		D(bug("Alloc for SSP failed!\n"));
	}
	D(bug("SSP: %x\n",initial_ssp));
#endif
	D(bug("Now running the romtagscanner!\n"));
	SysBase->ResModules=Exec_RomTagScanner(SysBase, rom_ranges);

	/*
	 * Init the core
	 */
	D(bug("init core now\n"));
	init_core(SysBase);

//	D(bug("init traps now\n"));
//	Init_Traps();

	if (NULL == (arm_SP_IRQ =(ULONG *)AllocMem(AROS_STACKSIZE,MEMF_PUBLIC)))  {
		do {} while(1);
	}
	arm_SP_IRQ = (ULONG *)(((ULONG)arm_SP_IRQ) + AROS_STACKSIZE - sizeof(ULONG));
	D(bug("Now setting IRQ Stackpointer! %x\n",arm_SP_IRQ));
	set_sp_mode(arm_SP_IRQ, MODE_IRQ);

	if (NULL == (arm_SP_FIQ =(ULONG *)AllocMem(AROS_STACKSIZE,MEMF_PUBLIC)))  {
		do {} while(1);
	}
	arm_SP_FIQ = (ULONG *)(((ULONG)arm_SP_FIQ) + AROS_STACKSIZE - sizeof(ULONG));
	D(bug("Now setting FIQ Stackpointer! %x\n",arm_SP_FIQ));
	set_sp_mode(arm_SP_FIQ, MODE_FIQ);

	if (NULL == (arm_SP_Abort =(ULONG *)AllocMem(AROS_STACKSIZE,MEMF_PUBLIC)))  {
		do {} while(1);
	}
	arm_SP_Abort = (ULONG *)(((ULONG)arm_SP_Abort) + AROS_STACKSIZE - sizeof(ULONG));
	D(bug("Now setting Abort Stackpointer! %x\n",arm_SP_Abort));
	set_sp_mode(arm_SP_Abort, MODE_ABORT);

	if (NULL == (arm_SP_Undef =(ULONG *)AllocMem(AROS_STACKSIZE,MEMF_PUBLIC)))  {
		do {} while(1);
	}
	arm_SP_Undef = (ULONG *)(((ULONG)arm_SP_Undef) + AROS_STACKSIZE - sizeof(ULONG));
	D(bug("Now setting Undef Stackpointer! %x\n",arm_SP_Undef));
	set_sp_mode(arm_SP_Undef, MODE_UNDEF);


	D(bug("cp15 register 0: 0x%x\n",get_cp15_r0()));
	D(bug("cp15 register 1: 0x%x\n",get_cp15_r1()));
	D(bug("cp15 register 2: 0x%x\n",get_cp15_r2()));
	mmu_lookup(0x0,SysBase);
	mmu_lookup(0xc0000000,SysBase);
	/*
	 * This is the last place where I am in supervisor mode.
	 * so let me switch into user mode and continue there.
	 * The user mode function will then call main_init_cont.
	 */
	D(bug("switching to user mode now\n"));
	switch_to_user_mode(main_init_cont, arm_SP_User);
}

/*
 * The following function will be executed whan AROS is in user mode
 */
static void main_init_cont(void)
{
	D(bug("!!!!! in user mode now !!!!\n"));
	InitCode(RTF_SINGLETASK, 0);
	D(bug("Ooops, should never get here!\n"));
	while (1) {
	}
	/*
	   All done. In normal cases CPU should never reach this point
	 */
}
