/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: m68k-amiga bootstrap to exec.
    Lang: english
 */

#include <aros/kernel.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "kernel_romtags.h"

#include "amiga_hwreg.h"
#include "amiga_irq.h"

extern const struct Resident Exec_resident;

extern void __clear_bss(const struct KernelBSS *bss);

#define RGB(r,g,b)	((((r) & 0xf) << 8) | (((g) & 0xf) << 4) | (((b) & 0xf) << 0))

#define CODE_ROM_CHECK	RGB( 4,  4, 4)
#define CODE_RAM_CHECK	RGB( 9,  9, 9)
#define CODE_EXEC_CHECK	RGB( 9,  0, 9)
#define CODE_TRAP_FAIL	RGB(12, 12, 0)

static void Exec_ScreenCode(UWORD code)
{
	reg_w(BPLCON0, 0x0200);
	reg_w(BPL1DAT, 0x0000);
	reg_w(COLOR00, code);
}

void DebugInit()
{
	/* Set the debug UART to 9600 */
	reg_w(SERPER, SERPER_BAUD(SERPER_BASE_PAL, 9600));
}

int DebugPutChar(register int chr)
{
	while ((reg_r(SERDATR) & SERDATR_TBE) == 0);

	/* Output a char to the debug UART */
	reg_w(SERDAT, SERDAT_STP8 | SERDAT_DB8(chr));

	return 1;
}

int DebugGetChar(void)
{
	while ((reg_r(SERDATR) & SERDATR_RBF) == 0);

	return SERDATR_DB8_of(reg_r(SERDATR));
}

#ifdef USE_GDBSTUB
#define mc68020
#include "m68k-gdbstub.c"
#endif

static __attribute__((interrupt)) void Exec_FatalException(void)
{
	volatile int i;

	Exec_ScreenCode(CODE_TRAP_FAIL);

	/* FIXME: Idle loop delay
	 * We should really wait for a number of
	 * verical retrace intervals
	 */
	for (i = 0; i < 150000; i++);

	/* Reset everything but the CPU, then restart
	 * at the ROM exception vector
	 */
	asm("reset\n"
	    "move.l #4,%a0\n"
	    "jmp    (%a0)\n");
}

static void DebugPuts(register const char *buff)
{
	for (; *buff != 0; buff++)
		DebugPutChar(*buff);
}

extern void *_ram_start;
#define MEM_START	((ULONG)(&_ram_start))
#define MEM_SIZE        (0x00200000-MEM_START)

void DebugPutHex(const char *what, ULONG val)
{
	int i;
	DebugPuts(what);
	DebugPuts(": ");
	for (i = 0; i < 8; i ++) {
		DebugPutChar("0123456789abcdef"[(val >> (28 - (i * 4))) & 0xf]);
	}
	DebugPutChar('\n');
}

extern void __attribute__((interrupt)) Exec_Supervisor_Trap (void);

void __attribute__((interrupt)) cpu_detect_trap(void);
asm (
	"	.text\n"
	"	.globl cpu_detect_trap\n"
	"cpu_detect_trap:\n"
	"	addq.l	#2,%sp@(2)\n"
	"	move.w	%sr,%d0\n"
	"	rte\n"
);

/* Detect 68000 vs 68010/68020 */
ULONG cpu_detect(void)
{
	volatile APTR *trap = (NULL + 8);
	APTR old_trap6;
	UWORD ret;

	old_trap6 = trap[6];
	trap[6] = cpu_detect_trap;
	asm volatile (	
		"move.w	%%sr,%%d0\n"
		"move.w	%%d0,%0\n"
		: "=m" (ret) : : "%d0" );
	trap[6] = old_trap6;

	if (ret & 0x2000)
		return AFF_68010;
	else 
		return 0;
}
	
#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
/*
 * Create a call stub like so:
 * foo:
 *   movem.l %d0-%d1/%a0-%a1,%sp@-
 *     0x48e7 0xc0c0
 *   jsr AROS_SLIB_ENTRY(funcname, libname)
 *     0x4eb9 .... ....
 *   movem.l %sp@+,%d0-%d1/%d0-%a1
 *     0x4cdf 0x0303
 *   rts
 *     0x4e75
 */
#define PRESERVE_ALL(lib, libname, funcname, funcid) \
	do { \
		UWORD *asmcall; \
		IPTR func = (IPTR)__AROS_GETJUMPVEC(lib, funcid)->vec; \
		asmcall = AllocMem(8 * sizeof(UWORD), MEMF_PUBLIC); \
		/* NOTE: 'asmcall' will intentionally never be freed */ \
		asmcall[0] = 0x48e7; \
		asmcall[1] = 0xc0c0; \
		asmcall[2] = 0x4eb9; \
		asmcall[3] = (func >> 16) & 0xffff; \
		asmcall[4] = (func >>  0) & 0xffff; \
		asmcall[5] = 0x4cdf; \
		asmcall[6] = 0x0303; \
		asmcall[7] = 0x4e75; \
		/* Insert into the library's jumptable */ \
		__AROS_SETVECADDR(lib, funcid, asmcall); \
	} while (0)
/* Inject a 'move.w #value,%d0; rts" sequence into the
 * jump table, to fake an private syscall.
 */
#define FAKE_ID(lib, funcid, value) \
	do { \
		UWORD *asmcall = (UWORD *)__AROS_GETJUMPVEC(lib, funcid); \
		asmcall[0] = 0x4660;	/* move.w #...,%d0 */ \
		asmcall[1] = ((ULONG)(value) >>  0) & 0xffff; \
		asmcall[2] = 0x4e75;	/* rts */ \
	} while (0);

#else
/* Not needed on EABI */
#define PRESERVE_ALL(lib, libname, funcname, funcid) do { } while (0)
#define FAKE_ID(lib, funcid, value) do { } while (0)
#endif

void start(void)
{
	extern void *_bss;
	extern void *_bss_end;
	extern void *_ss_stack_upper;
	extern void *_ss_stack_lower;
	volatile APTR *tmp;
	int i;
	UWORD *kickrom[] = {
		(UWORD *)0x00f80000,
		(UWORD *)0x01000000,
		(UWORD *)0x00f00000,
		(UWORD *)0x00f80000,
		(UWORD *)0x00e00000,
		(UWORD *)0x00e80000,
		(UWORD *)~0,
		(UWORD *)~0,
	};
	const struct KernelBSS kbss[2] = {
		{
			.addr = &_bss,
			.len = &_bss_end - &_bss,
		}, {
			.addr = 0,
			.len = 0,
		}
	};

	struct ExecBase *sysBase;
	struct MemChunk *mc;
	struct MemHeader ChipRAM;
	struct MemHeader *mh = &ChipRAM;

	*((APTR *)(NULL + 0x4)) = NULL;

	/* Let the world know we exist
	 */
	DebugInit();
	DebugPuts("[reset]\n");

	/* Fill exception table with a stub that will
	 * reset the ROM
	 */
	tmp = (APTR *)(NULL + 0x8);
	for (i = 0; i < 46; i++)
		tmp[i] = Exec_FatalException;

	/* Clear the BSS */
	__clear_bss(&kbss[0]);
	DebugPuts("[bss clear]\n");

#ifdef USE_GDBSTUB
	/* Must be after the BSS clear! */
	gdbstub();
#endif

	/* Set privilige violation trap - we
	 * need this to support the Exec/Supervisor call
	 */
	tmp[6] = Exec_Supervisor_Trap;

	Exec_ScreenCode(CODE_RAM_CHECK);

	DebugPuts("[prep SysBase]\n");
	Exec_ScreenCode(CODE_EXEC_CHECK);
	mc = (struct MemChunk *)(NULL + MEM_START);
	mc->mc_Next = NULL;
	mc->mc_Bytes = MEM_SIZE;

	mh->mh_Node.ln_Succ    = NULL;
	mh->mh_Node.ln_Pred    = NULL;
	mh->mh_Node.ln_Type    = NT_MEMORY;
	mh->mh_Node.ln_Name    = "chip memory";
	mh->mh_Node.ln_Pri     = -5;
	mh->mh_Attributes      = MEMF_CHIP | MEMF_PUBLIC | MEMF_LOCAL | MEMF_24BITDMA | MEMF_KICK;
	mh->mh_First           = mc;
	mh->mh_Lower           = (APTR)mc;
	mh->mh_Upper           = ((APTR)mc) + mc->mc_Bytes;
	mh->mh_Free            = mc->mc_Bytes;
	sysBase = PrepareExecBase(mh, NULL, NULL);
	DebugPutHex("PrepareExecBase [ret]",(ULONG)sysBase);
	*((APTR *)(NULL + 0x4)) = sysBase;
	DebugPuts("[init SysBase]\n");

	/* Fix up functions that need 'preserves all registers'
	 * semantics. This AllocMem()s a little wrapper routine
	 * that pushes the %d0-%d1/%a0-%a1 registers before
	 * calling the routine.
	 */
#ifdef THESE_ARE_KNOWN_SAFE_ASM_ROUTINES
	PRESERVE_ALL(SysBase, Exec, Disable, 20);
	PRESERVE_ALL(SysBase, Exec, Enable, 21);
	PRESERVE_ALL(SysBase, Exec, Forbid, 22);
#endif
	PRESERVE_ALL(SysBase, Exec, Permit, 23);
	PRESERVE_ALL(SysBase, Exec, ObtainSemaphore, 94);
	PRESERVE_ALL(SysBase, Exec, ReleaseSemaphore, 95);
	PRESERVE_ALL(SysBase, Exec, ObtainSemaphoreShared, 113);

	/* Needed for card.resource */
	FAKE_ID(SysBase, 136, 0x0000);

        sysBase->SysStkUpper    = (APTR)(&_ss_stack_upper)-1;
        sysBase->SysStkLower    = (APTR)&_ss_stack_lower;

	/* Determine CPU model */
	sysBase->AttnFlags |= cpu_detect();

	/* Initialize IRQ subsystem */
	AmigaIRQInit(sysBase);

	/* Scan for all other ROM Tags */
	sysBase->ResModules = krnRomTagScanner(sysBase, kickrom);
	DebugPuts("[start] InitCode(RTF_SINGLETASK, 0)\n");
	InitCode(RTF_SINGLETASK, 0);

	/* We shouldn't get here */
	DebugPuts("[DOS Task failed to start]\n");
	for (;;)
		breakpoint();
}
