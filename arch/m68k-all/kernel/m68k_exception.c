/*
    copyright © 1995-2010, the aros development team. all rights reserved.
    $id$

    desc: m68k-amiga bootstrap to exec.
    lang: english
 */

#define DEBUG 0
#include <aros/kernel.h>
#include <aros/debug.h>
#include <aros/asmcall.h>

#include <exec/resident.h>
#include <exec/execbase.h>
#include <exec/memory.h>

#include "exec_intern.h"

#include "m68k_exception.h"

/* Here's how it's all laid out on the Amiga
 *    M68K Exception
 *      0	Reset: Initial SP
 *      1	Reset: Initial PC (NOTE: Really is AbsExecBase!)
 *      2	Bus Error
 *      3	Address Error
 *      4	Illegal Instruction
 *      5	Divide by Zero
 *      6	CHK Instruction
 *      7	TRAPV Instruction
 *      8	Privileged Instruction
 *      9	Trace
 *      10	Line 1010 Emulator
 *      11	Line 1111 Emulator
 *      12	-
 *      13	-
 *      14	Format Error
 *      15	Uninitilaized Interrupt Vector
 *      16	-
 *      ..
 *      23	-
 *      24	Spurious Interrupt
 *      25	Level 1 Interrupt
 *      		Paula 0: Serial TX
 *      		Paula 1: Disk DMA done
 *      		Paula 2: Software Int
 *      26	Level 2 Interrupt
 *      		Paula 3: CIA
 *      27	Level 3 Interrupt
 *      		Paula 4: Copper
 *      		Paula 5: Vert Blank
 *      		Paula 6: Blitter
 *      28	Level 4 Interrupt
 *      		Paula 7: Audio 0
 *      		Paula 8: Audio 1
 *      		Paula 9: Audio 2
 *      		Paula 10: Audio 3
 *      29	Level 5 Interrupt
 *      		Paula 11: Serial RX
 *      		Paula 12: Disk Sync
 *      30	Level 6 Interrupt
 *      		Paula 13: External
 *      		Paula 14: Copper (special)
 *      31	Level 7 Interrupt
 *      		Paula 15: NMI
 *      32	TRAP #0
 *      ..
 *      47	TRAP #15
 *      48	-
 *      ..
 *      63	AbsExecBase (copy)
 *      64	User 1
 *      ..
 *      255	User 191
 */

extern ULONG M68KFaultTable_00[];
asm (
	"	.text\n"
	"	.align	4\n"
	"	.globl M68KFaultTable_00\n"
	"M68KFaultTable_00:\n"
	".Lfault_0:	bsr.w	.Lfault\n"	// Placeholder
	".Lfault_1:	bsr.w	.Lfault\n"	// Placeholder
	".Lfault_2:	bsr.w	.Lfault\n"	// Bus Error
	".Lfault_3:	bsr.w	.Lfault\n"	// Address Error
	".Lfault_4:	bsr.w	.Lfault\n"	// Illegal instruction
	".Lfault_5:	bsr.w	.Lfault\n"	// Zero divide
	".Lfault_6:	bsr.w	.Lfault\n"	// CHK
	".Lfault_7:	bsr.w	.Lfault\n"	// TRAPV
	".Lfault_8:	bsr.w	.Lfault\n"	// Privilege violation
	".Lfault_9:	bsr.w	.Lfault\n"	// Trace
	".Lfault_10:	bsr.w	.Lfault\n"	// Line A
	".Lfault_11:	bsr.w	.Lfault\n"	// Line F
	".Lfault_12:	bsr.w	.Lfault\n"	// Placeholder
	".Lfault_13:	bsr.w	.Lfault\n"	// Placeholder
	".Lfault_14:	bsr.w	.Lfault\n"	// Placeholder
	".Lfault_15:	bsr.w	.Lfault\n"	// Placeholder
	".Lfault_16:	bsr.w	.Lfault\n"	// Placeholder
	".Lfault_17:	bsr.w	.Lfault\n"	// Placeholder
	".Lfault_18:	bsr.w	.Lfault\n"	// Placeholder
	".Lfault_19:	bsr.w	.Lfault\n"	// Placeholder
	".Lfault_20:	bsr.w	.Lfault\n"	// Placeholder
	".Lfault_21:	bsr.w	.Lfault\n"	// Placeholder
	".Lfault_22:	bsr.w	.Lfault\n"	// Placeholder
	".Lfault_23:	bsr.w	.Lfault\n"	// Placeholder
	".Lfault:	subi.l	#(M68KFaultTable_00 + 1*4 - 0*4),%sp@\n"
	"		jmp	M68KExceptionHelper\n"
);

extern ULONG M68KLevelTable_00[];
asm (
	"	.text\n"
	"	.align	4\n"
	"	.globl M68KLevelTable_00\n"
	"M68KLevelTable_00:\n"
	".Llevel_0:	bsr.w	.Llevel\n"	// Spurious Interrupt
	".Llevel_1:	bsr.w	.Llevel\n"	// Level 1 Interrupt
	".Llevel_2:	bsr.w	.Llevel\n"	// Level 2 Interrupt
	".Llevel_3:	bsr.w	.Llevel\n"	// Level 3 Interrupt
	".Llevel_4:	bsr.w	.Llevel\n"	// Level 4 Interrupt
	".Llevel_5:	bsr.w	.Llevel\n"	// Level 5 Interrupt
	".Llevel_6:	bsr.w	.Llevel\n"	// Level 6 Interrupt
	".Llevel_7:	bsr.w	.Llevel\n"	// Level 7 Interrupt
	".Llevel:	subi.l	#(M68KLevelTable_00 + 1*4 - 24*4),%sp@\n"
	"		jmp	M68KExceptionHelper\n"
);

extern ULONG M68KTrapTable_00[];
asm (
	"	.text\n"
	"	.align	4\n"
	"	.globl M68KTrapTable_00\n"
	"M68KTrapTable_00:\n"
	".Ltrap_0:	bsr.w	.Ltrap\n"	// TRAP #0
	".Ltrap_1:	bsr.w	.Ltrap\n"	// TRAP #1
	".Ltrap_2:	bsr.w	.Ltrap\n"	// ...
	".Ltrap_3:	bsr.w	.Ltrap\n"	// ...
	".Ltrap_4:	bsr.w	.Ltrap\n"	// ...
	".Ltrap_5:	bsr.w	.Ltrap\n"	// ...
	".Ltrap_6:	bsr.w	.Ltrap\n"	// ...
	".Ltrap_7:	bsr.w	.Ltrap\n"	// ...
	".Ltrap_8:	bsr.w	.Ltrap\n"	// ...
	".Ltrap_9:	bsr.w	.Ltrap\n"	// ...
	".Ltrap_10:	bsr.w	.Ltrap\n"	// ...
	".Ltrap_11:	bsr.w	.Ltrap\n"	// ...
	".Ltrap_12:	bsr.w	.Ltrap\n"	// ...
	".Ltrap_13:	bsr.w	.Ltrap\n"	// ...
	".Ltrap_14:	bsr.w	.Ltrap\n"	// ...
	".Ltrap_15:	bsr.w	.Ltrap\n"	// TRAP #15
	".Ltrap:	subi.l	#(M68KTrapTable_00 + 1*4 - 32*4),%sp@\n"
	"		jmp	M68KExceptionHelper\n"
);

/* 68000 Exceptions */
static void M68KExceptionInit_00(struct ExecBase *SysBase)
{
	APTR *exception = (APTR *)0;
	int i;

	/* Faults */
	for (i = 2; i < 24; i++)
	    exception[i] = &M68KFaultTable_00[i];

	/* Level interrupts */
	for (i = 0; i < 7; i++)
	    exception[i + 24] = &M68KLevelTable_00[i];

	/* NMI (exception[31]) is left unset, for debuggers */

	/* Traps */
	for (i = 0; i < 16; i++)
	    exception[i + 32] = &M68KTrapTable_00[i];
}

/* 68010 Traps */

/* For the 68010+, the lower 12 bits of the UWORD
 * at %sp@(6) contains the vector number.
 * Convenience!
 */
extern void M68KTrapHelper_10(void);
asm (
	"	.text\n"
	"	.align 4\n"
	"	.globl	M68KTrapHelper_10\n"
	"M68KTrapHelper_10:\n"
	"	move.w	%sp@(6),%sp@-\n"	// Copy the vector
	"	andi.w	#0x0fff,%sp@\n"		// Clear the upper bits
	"	clr.w	%sp@-\n"		// extend vector to long
	"	jmp	M68KExceptionHelper\n"
);

static void M68KExceptionInit_10(struct ExecBase *SysBase)
{
	APTR *exception = (APTR *)0;
	int i;

	/* We can use the same code for all M68010+ traps */
	for (i = 2; i < 64; i++) {
	    /* Don't touch the NMI exception (for debuggers) */
	    if (i == 31)
	    	continue;
	    exception[i] = M68KTrapHelper_10;
	}
}

/******************** Exceptions *****************/

/* The stack frame here:
 *     Return PC          ULONG@(6)
 *     Return SR          UWORD@(4)
 *     Exception Vector   ULONG@(0)
 * SP ->
 *
 * When we call M68KExceptionAction:
 *     Return PC                (4)
 *     Return SR                (2)
 *     TrapArg                  (4)
 *     TrapCode                 (4)
 *     D0-D1/A0-A1		(4 * 4)	<= NO TOUCHING!
 *     Pointer to trapcode/arg  (4)
 *     Exception Vector		(4)
 *
 * When we come back:
 *     Restore D0-D1/A0-A1
 *     Either execute the trap, or just return.
 *     RTE
 */
extern void M68KExceptionHelper(void);
asm (
        "       .text\n"
        "       .globl M68KExceptionHelper\n"
        "M68KExceptionHelper:\n"
        "       clr.l   %sp@-\n"                // Save space for tc
        "       movem.l %d0-%d1/%a0-%a1,%sp@-\n"// Save regs
        "       lea.l   %sp@(4*(4)),%a0\n"      // Get location of tc/ta
        "       move.l  %sp@(4*(4+1)),%d0\n"    // Get exception vector
        "       jsr     M68KExceptionAction\n"  // Call action routine
        "       movem.l %sp@+,%d0-%d1/%a0-%a1\n"// Restore regs
        "       tst.l   %sp@\n"                 // NULL trapcode? Just return
        "       beq.s   1f\n"
        "       rts\n"                          // Execute tc_TrapCode
        "1:\n"
        "       addq.l  #4,%sp\n"
        "0:\n"
        "       addq.l  #4,%sp\n"               //   Drop TrapCode parameter
        "       rte\n"                          //   And return
);

/* Default handler */
extern void Exec_MagicResetCode(void);
extern struct ExecBase *AbsExecBase;

struct M68KTrapCode {
    APTR  trapcode;
    ULONG traparg;
};

AROS_UFH2(VOID, M68KExceptionAction,
        AROS_UFHA(ULONG, vector, D0),
        AROS_UFHA(struct M68KTrapCode *, tc, A0))
{
    AROS_USERFUNC_INIT

    ULONG Id;
    VOID (*Handler)(ULONG id);

    if (vector & 1) {
    	/* vector is really a pointer to a M68KException table entry */
    	struct M68KException *Exception;

    	Exception = (APTR)(vector & ~1);

    	Id = Exception->Id;
    	Handler = Exception->Handler;
    } else {
    	Id = vector >> 2;
    	Handler = NULL;
    }

#if DEBUG
    if (SysBase == NULL || KernelBase == NULL) {
    	volatile LONG *LastAlert = (volatile LONG *)(64 * sizeof(LONG));
    	/* SysBase has been corrupted! Mark the alert,
    	 * and reboot.
    	 */
    	LastAlert[0] = (LONG)(AT_DeadEnd | AN_LibChkSum);
    	/* LastAlert[1] was already set by
    	 * Exec/Dispatch
    	 */
    	LastAlert[1] = 0; /* No SysBase? No Task. */
    	LastAlert[2] = 0;
    	LastAlert[3] = 0;

    	/* Set LastAlert marker */
    	*(volatile ULONG *)0 = 0x48454c50; /* 'HELP' */
    	Exec_MagicResetCode();
    	return;
    }
#endif

    tc->traparg = Id;
    if (!Handler) {
        Handler = FindTask(NULL)->tc_TrapCode;
        if (!Handler) {
            Handler = SysBase->TaskTrapCode;
        }
    }
    tc->trapcode = Handler;

    AROS_USERFUNC_EXIT
}

/* We assume that the caller has already set up
 * the exceptions to a 'reasonable' default. These
 * are only the overrides for AROS.
 */
void M68KExceptionInit(const struct M68KException *Table, struct ExecBase *SysBase)
{
	IPTR *exception = (IPTR *)0;	/* Exception base is at 0 */
	UWORD *jmptab;
	int i;
	int size;

	/* Initialize the Well Known Traps */
	if (SysBase->AttnFlags & AFF_68010) {
	    M68KExceptionInit_10(SysBase);
	} else {
	    M68KExceptionInit_00(SysBase);
	}

	if ((ULONG)Table & 1) {
	    /* Exception Table must be UWORD aligned! */
	    return;
	}

	for (size = 0; Table[size].Id > 0; size++);

	/* A little explanation. jmptab will be 
	 * constructed as follows:
	 *    move.l (i << 1) | 1, %sp@+
	 *      0x2f3c ((i << 1) | 1) >> 16) ((i << 1) | 1) & 0xffff)
	 *    jmp %pc@(((size - 1) - i) * (5 * sizeof(UWORD)) + 2)
	 *      0x4efa (((size - 1) - i) * (5 * sizeof(UWORD)) + 2)
	 *    ...
	 *    ...
	 *    jmp M68KExceptionHelper
	 *      0x4ef9 (M68KExceptionHelper >> 16) (M68KExceptionHelper & 0xffff)
	 *
	 * NOTICE: jmptab will never be freed! */
	jmptab = AllocMem(size * (5 * sizeof(UWORD)) + 3 * sizeof(UWORD), 0);

	for (i = 0; i < size; i++, jmptab += 5) {
		/* This little trick is why we want
		 * the Table UWORD aligned.
		 *
		 * See the rest of this in M68KExceptionHelper
		 */
		ULONG vecid = (ULONG)(&Table[i]) | 1;

		jmptab[0] = 0x2f3c;	// movel #...,%sp@-
		jmptab[1] = ((IPTR)(vecid) >> 16) & 0xffff;
		jmptab[2] = ((IPTR)(vecid) >>  0) & 0xffff;
		jmptab[3] = 0x4efa;	// jmp %pc@...
		jmptab[4] = ((size - 1) - i) * (5 * sizeof(UWORD)) + 2;
		exception[Table[i].Id] = (IPTR)(&jmptab[0]);
	}
	jmptab[0] = 0x4ef9;		// jmp ....
	jmptab[1] = ((IPTR)(M68KExceptionHelper) >> 16) & 0xffff;
	jmptab[2] = ((IPTR)(M68KExceptionHelper) >>  0) & 0xffff;

	/* We're all set up now! */
}
