/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifdef __palmos__
#include <Pilot.h>
#endif
#include <registers.h>
#include <palmgfx.h>

#define AROS_USE_OOP
#define AROS_ALMOST_COMPATIBLE 1

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


#include "memory.h"
#include <memory.h>

#include "exec_intern.h"
#include "etask.h"

//extern const char Exec_end;

extern struct ExecBase * PrepareExecBase(struct MemHeader *);
extern void switch_to_user_mode(void *, ULONG *);
extern void Init_PalmHardware(void);
extern void Init_IRQVectors(void);
extern int main_init_cont(void);


extern const struct Resident
#if 0
	Expansion_resident,
#endif
	Exec_resident
#if 0
	,
	Utility_resident,
	Aros_resident,
	Mathieeesingbas_resident,
/*	BOOPSI_resident, */
	OOP_resident,
	HIDD_resident,
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
	kbdHidd_resident,
	vgaHidd_resident,
	hiddserial_resident,
	pciHidd_resident,
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
	AFS_resident
#endif
	;


/* This list MUST be in the correct order (priority). */
static const struct Resident *romtagList[] =
{
#if 0
	&Expansion_resident,            /* SingleTask,  110  */
#endif
	&Exec_resident,                 /* SingleTask,  105  */
#if 0
	&Utility_resident,              /* ColdStart,   103  */
	&Aros_resident,                 /* ColdStart,   102  */
	&Mathieeesingbas_resident,      /* ColdStart,   101  */
#if 0
	&BOOPSI_resident,               /* ColdStart,   95	 */
#endif
	&OOP_resident,                  /* ColdStart,   94	 */
	&HIDD_resident,                 /* ColdStart,   92	 */
	&irqHidd_resident,              /* ColdStart,   90	 */	// IRQ Hidd!
	&Graphics_resident,             /* ColdStart,   65	 */
	&Layers_resident,               /* ColdStart,   60	 */
	&Timer_resident,                /* ColdStart,   50	 */
	&Misc_resident,                 /* ColdStart,   45	 */
	&Battclock_resident,	        /* ColdStart,   45	 */
	&Keyboard_resident,             /* ColdStart,   44	 */
	&Gameport_resident,             /* ColdStart,   43	 */
	&Keymap_resident,		/* ColdStart,   40	 */
	&Input_resident,                /* ColdStart,   30	 */
	&Intuition_resident,            /* ColdStart,   10	 */	
	&hiddgraphics_resident,		/* ColdStart,   9	 */
	&kbdHidd_resident,              /* ColdStart,   9	 */
	&vgaHidd_resident,		/* ColdStart,   9	 */
	&mouseHidd_resident,		/* ColdStart,   9	 */
	&hiddserial_resident,		/* ColdStart,   9	 */
	&pciHidd_resident,              /* ColdStart,   9	 */
	&Console_resident,              /* ColdStart,   5	 */
//	&emul_handler_resident,		/* ColdStart,   0	 */
//	&Workbench_resident,		/* ColdStart,  -120	 */
//	&Mathffp_resident,		/* ColdStart,  -120	 */

	/*
	  NOTE: You must not put anything between these two; the code which
	  initialized boot_resident will directly call Dos_resident and
	  anything between the two will be skipped.
	*/
	&boot_resident,			    /* ColdStart,  -50	 */
	&Dos_resident,			    /* None,	   -120  */
	&LDDemon_resident,		    /* AfterDOS,   -125  */
	&con_handler_resident,		    /* AfterDOS,   -126  */
	&AFS_resident,
#endif
	NULL
};

/************************************************************************************/

/************************************************************************************/


void main_init(void * memory, ULONG memSize)
{
	struct ExecBase *SysBase = NULL;
	ULONG * m68k_USP;
	struct MemHeader *mh = NULL;
//	Init_PalmHardware();
//	Init_IRQVectors();


//	Memory=GetMemSize();


	/*
	   Prepare first memory list. Our memory will statr at 0x00100000. First
	   1MB is reserved for kernel use only. DO NOT use it please.
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


	if (NULL == (m68k_USP =(ULONG)AllocMem(AROS_STACKSIZE,MEMF_PUBLIC)))  {
		do {} while(1);
	}
	m68k_USP += AROS_STACKSIZE;

	SysBase->ResModules=romtagList;
	
	/*
	 * This is the last place where I am in supervisor mode.
	 * so let me switch into user mode and continue there.
	 * The user mode function will then call main_init_cont.
	 */
	switch_to_user_mode(main_init_cont, m68k_USP);
}

/*
 * The following function will be executed whan AROS is in user mode
 */
int main_init_cont()
{
	struct ExecBase * SysBase;
	SysBase = (struct ExecBase *)(*(ULONG *)0x04);
	InitCode(RTF_SINGLETASK, 0);

	/*
	   All done. In normal cases CPU should never reach this instructions
	 */

	/*
	 * Currently  I still get here and just do something...
	 */
	{
		struct Task * t;
		t = FindTask("Idle Task");
		/*
		 * Make the idle task do something by raising its
		 * priority
		 */
		if (NULL != t) {
			SetTaskPri(t,0);
		}

		while (1) {
			ULONG i = 0;
			Forbid();
			clearscreen(0);
			Permit();
			while (i < 160/2) {
				drawlinevert(i);
				drawlinehoriz(i);
				i++;
			}
		}
	}
}

