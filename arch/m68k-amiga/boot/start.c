/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: m68k-amiga bootstrap to exec.
    Lang: english
 */

#include <aros/kernel.h>
#include <aros/debug.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include "memory.h"

#include "exec_intern.h"
#include "kernel_romtags.h"

#include "amiga_hwreg.h"
#include "amiga_irq.h"

extern const struct Resident Exec_resident;

extern void __clear_bss(const struct KernelBSS *bss);

#define RGB(r,g,b)	((((r) & 0xf) << 8) | (((g) & 0xf) << 4) | (((b) & 0xf) << 0))

#define CODE_ROM_CHECK	RGB( 4,  4, 4)
#define CODE_RAM_CHECK	RGB( 9,  9, 9)
#define CODE_EXEC_CHECK	RGB( 1,  1, 1)
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

int DebugMayGetChar(void)
{
	if ((reg_r(SERDATR) & SERDATR_RBF) == 0)
	    return -1;

	return SERDATR_DB8_of(reg_r(SERDATR));
}

static __attribute__((interrupt)) void Exec_FatalException(void)
{
	volatile int i;

	Exec_ScreenCode(CODE_TRAP_FAIL);

    Debug(0);

	/* Reset everything but the CPU, then restart
	 * at the ROM exception vector
	 */
	asm volatile (
	    "nop\n"
	    "nop\n"
	    "move.l #4,%a0\n"
	    "reset\n"
	    "jmp    (%a0)\n");
}

static void DebugPuts(register const char *buff)
{
	for (; *buff != 0; buff++)
		DebugPutChar(*buff);
}

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
	volatile APTR *trap = NULL;
	APTR old_trap8;
	UWORD ret;

	old_trap8 = trap[8];
	trap[8] = cpu_detect_trap;
	asm volatile (	
		"move.w	%%sr,%%d0\n"
		"move.w	%%d0,%0\n"
		: "=m" (ret) : : "%d0" );
	trap[8] = old_trap8;

	if (ret & 0x2000)
		return AFF_68010;
	else 
		return 0;
}
	
#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
/* Create a sign extending call stub:
 * foo:
 *   jsr AROS_SLIB_ENTRY(funcname, libname)
 *     0x4eb9 .... ....
 *   ext.w %d0	// EXT_BYTE only
 *     0x4880	
 *   ext.l %d0	// EXT_BYTE and EXT_WORD
 *     0x48c0
 *   rts
 *     0x4e75
 */
#define EXT_BYTE(lib, libname, funcname, funcid) \
	do { \
		UWORD *asmcall; \
		IPTR func = (IPTR)__AROS_GETJUMPVEC(lib, funcid)->vec; \
		asmcall = AllocMem(6 * sizeof(UWORD), MEMF_PUBLIC); \
		/* NOTE: 'asmcall' will intentionally never be freed */ \
		asmcall[0] = 0x4eb9; \
		asmcall[1] = (func >> 16) & 0xffff; \
		asmcall[2] = (func >>  0) & 0xffff; \
		asmcall[3] = 0x4880; \
		asmcall[4] = 0x48c0; \
		asmcall[5] = 0x4e75; \
		/* Insert into the library's jumptable */ \
		__AROS_SETVECADDR(lib, funcid, asmcall); \
	} while (0)
#define EXT_WORD(lib, libname, funcname, funcid) \
	do { \
		UWORD *asmcall; \
		IPTR func = (IPTR)__AROS_GETJUMPVEC(lib, funcid)->vec; \
		asmcall = AllocMem(5 * sizeof(UWORD), MEMF_PUBLIC); \
		/* NOTE: 'asmcall' will intentionally never be freed */ \
		asmcall[0] = 0x4eb9; \
		asmcall[1] = (func >> 16) & 0xffff; \
		asmcall[2] = (func >>  0) & 0xffff; \
		asmcall[3] = 0x48c0; \
		asmcall[4] = 0x4e75; \
		/* Insert into the library's jumptable */ \
		__AROS_SETVECADDR(lib, funcid, asmcall); \
	} while (0)
/*
 * Create a register preserving call stub:
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
/* Inject arbitrary code into the jump table
 * Used for GetCC and nano-stubs
 */
#define FAKE_IT(lib, libname, funcname, funcid, ...) \
	do { \
		UWORD *asmcall = (UWORD *)__AROS_GETJUMPVEC(lib, funcid); \
		const UWORD code[] = { __VA_ARGS__ }; \
		asmcall[0] = code[0]; \
		asmcall[1] = code[1]; \
		asmcall[2] = code[2]; \
	} while (0)
/* Inject a 'move.w #value,%d0; rts" sequence into the
 * jump table, to fake an private syscall.
 */
#define FAKE_ID(lib, libname, funcname, funcid, value) \
	FAKE_IT(lib, libname, funcname, funcid, 0x303c, value, 0x4e75)
#else
/* Not needed on EABI */
#define PRESERVE_ALL(lib, libname, funcname, funcid) do { } while (0)
#define EXT_BYTE(lib, libname, funcname, funcid) do { } while (0)
#define EXT_WORD(lib, libname, funcname, funcid) do { } while (0)
#define FAKE_IT(lib, libname, funcname, funcid, ...) do { } while (0)
#define FAKE_ID(lib, libname, funcname, funcid, value) do { } while (0)
#endif

static struct MemHeader *SetupMemory(CONST_STRPTR name, BYTE pri,
				      ULONG start, ULONG size, UWORD flags)
{
	struct MemHeader *mh;
	ULONG aligned_start;

	/* Align the start in MEMCHUNK_TOTAL sections */
	aligned_start = (start + MEMCHUNK_TOTAL - 1) & ~(MEMCHUNK_TOTAL-1);
	size -= (aligned_start - start);
	start = aligned_start;

	mh = (APTR)start;
	mh->mh_Node.ln_Succ    = NULL;
	mh->mh_Node.ln_Pred    = NULL;
	mh->mh_Node.ln_Type    = NT_MEMORY;
	mh->mh_Node.ln_Name    = (STRPTR)name;
	mh->mh_Node.ln_Pri     = pri;
	mh->mh_Attributes      = flags | MEMF_KICK | MEMF_PUBLIC | MEMF_LOCAL | MEMF_24BITDMA;
	mh->mh_First           = (struct MemChunk *)(start+MEMHEADER_TOTAL);
	mh->mh_First->mc_Next  = NULL;
	mh->mh_First->mc_Bytes = size - MEMHEADER_TOTAL;

	mh->mh_Lower           = mh->mh_First;
	mh->mh_Upper           = (APTR)(start + size);
	mh->mh_Free            = mh->mh_First->mc_Bytes;

	return mh;
}

static LONG doInitCode(ULONG startClass, ULONG version)
{
    InitCode(startClass, version);

    return 0;
}

void start(IPTR chip_start, ULONG chip_size,
           IPTR fast_start, ULONG fast_size,
           IPTR ss_stack_upper, IPTR ss_stack_lower)
{
	extern void *_bss;
	extern void *_bss_end;
	volatile APTR *trap;
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
	struct MemHeader *mh;
	ULONG LastAlert[4] = { 0, 0, 0, 0};

	trap = (APTR *)(NULL);
	trap[1] = NULL;	/* Zap out old SysBase */

	/* Let the world know we exist
	 */
	DebugInit();
	DebugPuts("[reset]\n");

	if (fast_size != 0) {
		DebugPutHex("Fast_Upper ",(ULONG)(fast_start + fast_size - 1));
		DebugPutHex("Fast_Lower ",(ULONG)fast_start);
	}
	DebugPutHex("Chip_Upper ",(ULONG)(chip_start + chip_size - 1));
	DebugPutHex("Chip_Lower ",(ULONG)chip_start);
	DebugPutHex("SS_Stack_Upper",(ULONG)(ss_stack_upper - 1));
	DebugPutHex("SS_Stack_Lower",(ULONG)ss_stack_lower);

	/* Look for 'HELP' at address 0 - we're recovering
	 * from a fatal alert
	 */
	if (trap[0] == (APTR)0x48454c50) {
		for (i = 0; i < 4; i++)
			LastAlert[i] = (ULONG)trap[64 + i];
	}

	/* Clear last alert area */
	trap[0] = 0;
	for (i = 0; i < 4; i++)
		trap[64 + i] = 0;

	/* Fill exception table with a stub that will
	 * reset the ROM
	 */
	for (i = 2; i < 64; i++)
		trap[i] = Exec_FatalException;

	/* Clear the BSS */
	__clear_bss(&kbss[0]);
	DebugPuts("[bss clear]\n");

	/* Set privilige violation trap - we
	 * need this to support the Exec/Supervisor call
	 */
	trap[8] = Exec_Supervisor_Trap;

	DebugPuts("[prep RAM]\n");
	Exec_ScreenCode(CODE_RAM_CHECK);

	if (fast_size == 0) {
		mh = SetupMemory("Chip Mem", -10,
				 chip_start, chip_size, MEMF_CHIP);
	} else {
		mh = SetupMemory("Fast Mem", -5,
				 fast_start, fast_size, MEMF_FAST);
	}

	DebugPuts("[prep SysBase]\n");
	Exec_ScreenCode(CODE_EXEC_CHECK);

	sysBase = PrepareExecBase(mh, NULL, NULL);
	DebugPutHex("PrepareExecBase [ret]",(ULONG)sysBase);
	*((APTR *)(NULL + 0x4)) = sysBase;
	DebugPuts("[init SysBase]\n");

        sysBase->SysStkUpper    = (APTR)ss_stack_upper;
        sysBase->SysStkLower    = (APTR)ss_stack_lower;

        /* Mark what the last alert was */
        for (i = 0; i < 4; i++)
        	sysBase->LastAlert[i] = LastAlert[i];

	/* Determine CPU model */
	sysBase->AttnFlags |= cpu_detect();

	/* Fix up functions that need 'preserves all registers'
	 * semantics. This AllocMem()s a little wrapper routine
	 * that pushes the %d0-%d1/%a0-%a1 registers before
	 * calling the routine.
	 */
#ifdef THESE_ARE_KNOWN_SAFE_ASM_ROUTINES
	PRESERVE_ALL(sysBase, Exec, Disable, 20);
	PRESERVE_ALL(sysBase, Exec, Enable, 21);
	PRESERVE_ALL(sysBase, Exec, Forbid, 22);
#endif
	PRESERVE_ALL(sysBase, Exec, Permit, 23);
	PRESERVE_ALL(sysBase, Exec, ObtainSemaphore, 94);
	PRESERVE_ALL(sysBase, Exec, ReleaseSemaphore, 95);
	PRESERVE_ALL(sysBase, Exec, ObtainSemaphoreShared, 113);

	/* Functions that need sign extension */
	EXT_BYTE(sysBase, Exec, SetTaskPri, 50);
	EXT_BYTE(sysBase, Exec, AllocSignal, 55);
	EXT_BYTE(sysBase, Exec, OpenDevice, 74);
	EXT_BYTE(sysBase, Exec, DoIO, 76);
	EXT_BYTE(sysBase, Exec, WaitIO, 79);

	EXT_WORD(sysBase, Exec, GetCC, 88);

	/* Inject code for GetCC, depending on CPU model */
	if (sysBase->AttnFlags & AFF_68010) {
		/* move.w %ccr,%d0; rts; nop */
		FAKE_IT(sysBase, Exec, GetCC, 88, 0x42c0, 0x4e75, 0x4e71);
	} else {
		/* move.w %sr,%d0; rts; nop */
		FAKE_IT(sysBase, Exec, GetCC, 88, 0x40c0, 0x4e75, 0x4e71);
	}

	DebugPutHex("GayleID", ReadGayle());

	/* If we had Fast memory, don't forget to add
	 * Chip memory now!
	 */
	if (fast_size != 0) {
		mh = SetupMemory("Chip Memory", -5,
				 chip_start, chip_size, MEMF_CHIP);
		if (mh != NULL)
			Enqueue(&sysBase->MemList,&mh->mh_Node);
	}

	/* Initialize IRQ subsystem */
	AmigaIRQInit(sysBase);

	/* Scan for all other ROM Tags */
	sysBase->ResModules = krnRomTagScanner(sysBase, kickrom);
	DebugPuts("[start] InitCode(RTF_SINGLETASK, 0)\n");

	/* Ok, let's start the system */
	InitCode(RTF_SINGLETASK, 0);

	/* Attempt to allocate a real stack, and switch to it. */
	do {
	    struct StackSwapStruct sss;
	    struct StackSwapArgs ssa;
	    const ULONG size = AROS_STACKSIZE * sizeof(ULONG);
	
	    sss.stk_Lower = AllocMem(size, MEMF_PUBLIC);
	    if (sss.stk_Lower == NULL) {
		bug("Can't allocate a new stack for Exec... Strange.\n");
		break;
	    }
	    sss.stk_Upper = sss.stk_Lower + size;
	    sss.stk_Pointer = sss.stk_Upper;

	    ssa.Args[0] = RTF_COLDSTART;
	    ssa.Args[1] = 0;

	    NewStackSwap(&sss, doInitCode, &ssa);
	} while (0);

	/* We shouldn't get here */
	DebugPuts("[DOS Task failed to start]\n");
	for (;;)
	    Debug(0);
}
