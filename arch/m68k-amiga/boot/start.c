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

#include "kernel_romtags.h"
#include "kernel_base.h"

#include "exec_intern.h"

#include "amiga_hwreg.h"
#include "amiga_irq.h"

extern const struct Resident Exec_resident;

extern void __clear_bss(const struct KernelBSS *bss);

#define RGB(r,g,b)	((((r) & 0xf) << 8) | (((g) & 0xf) << 4) | (((b) & 0xf) << 0))

#define CODE_ROM_CHECK	RGB( 4,  4, 4)
#define CODE_RAM_CHECK	RGB( 9,  9, 9)
#define CODE_EXEC_CHECK	RGB( 1,  1, 1)
#define CODE_TRAP_FAIL	RGB(12, 12, 0)

void Early_ScreenCode(UWORD code)
{
	reg_w(BPLCON0, 0x0200);
	reg_w(BPL1DAT, 0x0000);
	reg_w(COLOR00, code);
}

void DebugInit()
{
	/* Set the debug UART to 115200 */
	reg_w(SERPER, SERPER_BAUD(SERPER_BASE_PAL, 115200));
}

int DebugPutChar(register int chr)
{
	if (chr == '\n')
		DebugPutChar('\r');
	while ((reg_r(SERDATR) & SERDATR_TBE) == 0);

	/* Output a char to the debug UART */
	reg_w(SERDAT, SERDAT_STP8 | SERDAT_DB8(chr));

	return 1;
}

int DebugMayGetChar(void)
{
	int c;

	if ((reg_r(SERDATR) & SERDATR_RBF) == 0)
	    return -1;

	c = SERDATR_DB8_of(reg_r(SERDATR));

	/* Clear RBF */
	reg_w(INTREQ, (1 << 11));

	return c;
}

#ifdef AROS_SERIAL_DEBUG

static void DebugPuts(register const char *buff)
{
	for (; *buff != 0; buff++)
		DebugPutChar(*buff);
}

static void DebugPutHex(const char *what, ULONG val)
{
	int i;
	DebugPuts(what);
	DebugPuts(": ");
	for (i = 0; i < 8; i ++) {
		DebugPutChar("0123456789abcdef"[(val >> (28 - (i * 4))) & 0xf]);
	}
	DebugPutChar('\n');
}

#define DEBUGPUTS(x) do { DebugPuts x; } while(0)
#define DEBUGPUTHEX(x) do { DebugPutHex x; } while(0)

#else

#define DEBUGPUTS(x)
#define DEBUGPUTHEX(x)

#endif

void Early_Alert(ULONG alert)
{
    const int bright = ((alert >> 4) & 1) ? 0xf : 0x7;
    const int color = 
    		RGB(((alert >> 2) & 1) * bright,
    		    ((alert >> 1) & 1) * bright,
    		    ((alert >> 0) & 1) * bright);

    DEBUGPUTHEX(("Early_Alert", alert));

    for (;;) {
    	volatile int i;
    	Early_ScreenCode(color);
    	for (i = 0; i < 100000; i++);
    	Early_ScreenCode(0x000);
    	for (i = 0; i < 100000; i++);

    	if (!(alert & AT_DeadEnd))
    	    break;
    }
}

/* Fatal trap for early problems */
extern void Exec_MagicResetCode(void);
static void __attribute__((interrupt)) Early_TrapHandler(void)
{
    volatile int i;
    Early_ScreenCode(CODE_TRAP_FAIL);

    /* If we have a valid KernelBase, then
     * we can run the debugger.
     */
    if (SysBase != NULL && KernelBase != NULL)
	Debug(0);
    else
    	Early_Alert(AT_DeadEnd | 1);

    /* Sleep for a while */
    for (i = 0; i < 100000; i++);

    /* Reset everything but the CPU, then restart
     * at the ROM exception vector
     */
    Exec_MagicResetCode();
}

extern void __attribute__((interrupt)) Exec_Supervisor_Trap (void);

/* detect CPU and FPU type, enable code cache (if supported)
 * does not enable data caches, it is bad idea on real hardware
 * without correct MMU tables (for example chip ram must be non-cacheable)
 */

void __attribute__((interrupt)) cpu_detect_trap_fpu(void);
asm (".chip 68060\n"
	"	.text\n"
	"	.globl cpu_detect_trap_fpu\n"
	"cpu_detect_trap_fpu:\n"
	"	move.l %sp,%a1\n"
	"	lea %sp@(-60),%sp\n"
	"	move.l %sp,%a0\n"
	"	clr.b (%a0)\n"
	"	fsave (%a0)\n"
	"	move.w #0x8000,%d0\n"
	"	move.b (%a0),%d0\n"
	"	move.l %a1,%sp\n"
	"	addq.l	#2,%sp@(2)\n" /* skip illegal */
	"	rte\n" /* return to cpu_detect() */
);

void __attribute__((interrupt)) cpu_detect_trap_f(void);
asm (
	"	.text\n"
	"	.globl cpu_detect_trap_f\n"
	"cpu_detect_trap_f:\n"
	"	move.l %a1,%sp\n"
	"	addq.l	#2,%sp@(2)\n" /* skip illegal */
	"	moveq #0,%d0\n"
	"	rte\n" /* return to cpu_detect() */
);

void __attribute__((interrupt)) cpu_detect_trap_priv(void);
asm (".chip 68060\n"
	"	.text\n"
	"	.globl cpu_detect_trap_priv\n"
	"cpu_detect_trap_priv:\n"
	"	move.w	#0x2001,%d0\n"
 		/* CACR is 68020+ */
	"	dc.l 0x4e7a0002\n" // movec	%cacr,%d0\n"
		/* 68020+ or better */
	"       move.w	#0x8000,%d0\n"
	"	dc.l 0x4e7b0002\n" // movec	%d0,%cacr\n"
 		/* enable 68040/060 code cache */
	"	dc.l 0x4e7a0002\n" // movec	%cacr,%d0\n"
 		/* bit 15 still set? */
	"	tst.w	%d0\n"
 		/* yes, it is 68040 or 68060 */
	"	bmi.s	0f\n"
 		/* enable 68020/030 code cache and 68030 data cache */
 	"	move.w	#0x0101,%d0\n"
	"	dc.l 0x4e7b0002\n" // movec	%d0,%cacr\n"
	"	dc.l 0x4e7a0002\n" // movec	%cacr,%d0\n"
 		/* data cache bit still set? */
	"	btst	#8,%d0\n"
	"	bne.s	1f\n" /* yes, it is 68030 */
		/* 68020 */
	"	move.w	#0x2003,%d0\n"
	"	illegal\n"
		/* 68030 */
	"1:	move.w	#0x0001,%d0\n"
		/* disable data cache, bad idea without correct MMU tables */
	"	dc.l 0x4e7b0002\n" // movec	%d0,%cacr\n"
	"	move.w	#0x2007,%d0\n"
	"	illegal\n"
		/* 68040 */
	"0:	move.w	#0x200f,%d0\n"
 		/* PCR is 68060 only */
	"	dc.l 0x4e7a0808\n" // movec	%pcr,%d0\n"
		/* 68060 */
	"	move.w	#0x0001,%d0\n"
 		/* enable supercalar, enable FPU */
 	"	dc.l 0x4e7b0808\n" // movec	%d0,%pcr\n"
	"	move.w	#0x208f,%d0\n"
	"	illegal\n"
);

void __attribute__((interrupt)) cpu_detect_trap_illg(void);
asm (
	"	.text\n"
	"	.globl cpu_detect_trap_illg\n"
	"cpu_detect_trap_illg:\n"
	"	addq.l	#8,%sp\n" /* remove illegal instruction stack frame */
	"	addq.l	#2,%sp@(2)\n" /* skip move sr,d0 */
	"	rte\n" /* return to cpu_detect() */
);

/* Detect CPU and FPU model */
static ULONG cpu_detect(void)
{
	volatile APTR *trap = NULL;
	APTR old_trap8, old_trap4, old_trap11;
	UWORD cpuret, fpuret;

	old_trap8 = trap[8];
	trap[8] = cpu_detect_trap_priv;
	old_trap4 = trap[4];
	trap[4] = cpu_detect_trap_illg;
	old_trap11 = trap[11];
	trap[11] = cpu_detect_trap_f;
	asm volatile (
		"move.w	%%sr,%%d0\n"
		"move.w	%%d0,%0\n"
		: "=m" (cpuret) : : "%d0" );
	trap[4] = cpu_detect_trap_fpu;
	asm volatile (
		"illegal\n" /* supervisor mode */
		"move.w	%%d0,%0\n"
		: "=m" (fpuret) : : "%d0", "%a0", "%a1" );
	trap[8] = old_trap8;
	trap[4] = old_trap4;
	trap[11] = old_trap11;

	cpuret &= 0xff;
	if (fpuret) {
		if (cpuret & (AFF_68040 | AFF_68060))
			cpuret |= AFF_FPU40 | AFF_68881 | AFF_68882;
		else if ((fpuret & 0x00ff) <= 0x1f)
			cpuret |= AFF_68881;
		else
			cpuret |= AFF_68881 | AFF_68882;
	}
	return cpuret;
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

static UWORD getsyschecksum(struct ExecBase *sysbase)
{
	UWORD sum = 0;
	UWORD *p = (UWORD*)&sysbase->SoftVer;
	while (p <= &sysbase->ChkSum)
		sum += *p++;
	return sum;
}
static void setsyschecksum(void)
{
	SysBase->ChkSum = 0;
	SysBase->ChkSum = getsyschecksum(SysBase) ^ 0xffff;
}
static BOOL issysbasevalid(struct ExecBase *sysbase)
{
	if (sysbase == NULL || (((ULONG)sysbase) & 0x80000001))
		return FALSE;
	if (sysbase->ChkBase != ~(IPTR)sysbase)
		return FALSE;
	return getsyschecksum(sysbase) == 0xffff;
}

extern void SuperstackSwap(void);
/* This calls the register-ABI library
 * routine Exec/InitCode, for use in NewStackSwap()
 */
static LONG doInitCode(ULONG startClass, ULONG version)
{
	/* Attempt to allocate a new supervisor stack */
	do {
   	    ULONG ss_stack_size = SysBase->SysStkUpper - SysBase->SysStkLower;
	    APTR ss_stack;

	    ss_stack = AllocMem(ss_stack_size, MEMF_ANY | MEMF_CLEAR);
	    if (ss_stack == NULL) {
	    	DEBUGPUTS(("Strange. Can't allocate a new system stack\n"));
	        break;
	    }

            SysBase->SysStkLower    = ss_stack;
            SysBase->SysStkUpper    = ss_stack + ss_stack_size;
	    setsyschecksum();

	    Supervisor(SuperstackSwap);
	} while (0);

	InitCode(startClass, version);

	return 0;
}

extern BYTE _rom_start;
extern BYTE _ext_start;
extern BYTE _bss;
extern BYTE _bss_end;

void start(IPTR chip_start, ULONG chip_size,
           IPTR fast_start, ULONG fast_size,
           IPTR ss_stack_upper, IPTR ss_stack_lower)
{
	volatile APTR *trap;
	int i;
	UWORD *kickrom[] = {
		(UWORD *)&_rom_start,
		(UWORD *)(&_rom_start + 0x80000),
		(UWORD *)0x00f00000,
		(UWORD *)0x00f80000,
		(UWORD *)&_ext_start,
		(UWORD *)(&_ext_start + 0x80000),
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
	struct MemHeader *mh;
	ULONG LastAlert[4] = { 0, 0, 0, 0};
	ULONG oldmem;
	struct ExecBase *oldsysbase = SysBase;
	ULONG rom_start = (ULONG)&_rom_start;
	ULONG rom_end = (ULONG)&_ext_start + 0x80000;
	ULONG romloader = 0;

	trap = (APTR *)(NULL);

	/* Set all the exceptions to the Early_TrapHandler
	 */
	for (i = 2; i < 64; i++)
	    trap[i] = Early_TrapHandler;

	/* Let the world know we exist
	 */
	DebugInit();
	DEBUGPUTS(("[reset]\n"));

	/* Zap out old SysBase if invalid */
	if (issysbasevalid(SysBase)) {
	    DEBUGPUTHEX(("[SysBase] was at", (ULONG)SysBase));
	    if ((ULONG)SysBase >= 0x100 && (ULONG)SysBase < 0x400) {
	    	/* ram loader fake sysbase */
	    	romloader = (ULONG)SysBase;
	    	SysBase = NULL;
		DEBUGPUTHEX(("[romloader] rom start:", rom_start));
		DEBUGPUTHEX(("[romloader] rom end  :", rom_end));
	    }
	    oldsysbase = NULL;
	} else {
	    DEBUGPUTHEX(("[SysBase] invalid at", (ULONG)SysBase));
	}

	if (fast_size != 0) {
		DEBUGPUTHEX(("Fast_Upper ",(ULONG)(fast_start + fast_size - 1)));
		DEBUGPUTHEX(("Fast_Lower ",(ULONG)fast_start));
	}
	DEBUGPUTHEX(("Chip_Upper ",(ULONG)(chip_start + chip_size - 1)));
	DEBUGPUTHEX(("Chip_Lower ",(ULONG)chip_start));
	DEBUGPUTHEX(("SS_Stack_Upper",(ULONG)(ss_stack_upper - 1)));
	DEBUGPUTHEX(("SS_Stack_Lower",(ULONG)ss_stack_lower));

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

	/* Clear the BSS */
	__clear_bss(&kbss[0]);
	DEBUGPUTHEX(("BSS lower", (ULONG)&_bss));
	DEBUGPUTHEX(("BSS upper", (ULONG)&_bss_end));

	Early_ScreenCode(CODE_RAM_CHECK);

	if (fast_size == 0) {
		krnCreateMemHeader("Chip Mem", -10,
				 (APTR)chip_start, chip_size,
				 MEMF_CHIP | MEMF_KICK | MEMF_PUBLIC | MEMF_LOCAL | MEMF_24BITDMA);
		mh = (APTR)chip_start;
	} else {
		krnCreateMemHeader("Fast Mem", -5,
				 (APTR)fast_start, fast_size,
				 MEMF_FAST | MEMF_KICK | MEMF_PUBLIC | MEMF_LOCAL | MEMF_24BITDMA);
		mh = (APTR)fast_start;
	}

	DEBUGPUTS(("[prep SysBase]\n"));

	Early_ScreenCode(CODE_EXEC_CHECK);

	PrepareExecBase(mh, NULL, NULL);

        SysBase->SysStkUpper    = (APTR)ss_stack_upper;
        SysBase->SysStkLower    = (APTR)ss_stack_lower;

        /* Mark what the last alert was */
        for (i = 0; i < 4; i++)
        	SysBase->LastAlert[i] = LastAlert[i];

	/* Determine CPU model */
	SysBase->AttnFlags |= cpu_detect();

#ifdef AROS_SERIAL_DEBUG
	DEBUGPUTS(("CPU: "));
	if (SysBase->AttnFlags & AFF_68060)
		DEBUGPUTS(("68060"));
	else if (SysBase->AttnFlags & AFF_68040)
		DEBUGPUTS(("68040"));
	else if (SysBase->AttnFlags & AFF_68030)
		DEBUGPUTS(("68030"));
	else if (SysBase->AttnFlags & AFF_68020)
		DEBUGPUTS(("68020"));
	else if (SysBase->AttnFlags & AFF_68010)
		DEBUGPUTS(("68010"));
	else
		DEBUGPUTS(("68000"));
	DEBUGPUTS((" FPU: "));
	if (SysBase->AttnFlags & AFF_FPU40) {
		if (SysBase->AttnFlags & AFF_68060)
			DEBUGPUTS(("68060"));
		else if (SysBase->AttnFlags & AFF_68040)
			DEBUGPUTS(("68040"));
		else
			DEBUGPUTS(("-"));
	} else if (SysBase->AttnFlags & AFF_68881)
		DEBUGPUTS(("68881"));
	else if (SysBase->AttnFlags & AFF_68882)
		DEBUGPUTS(("68882"));
	else
		DEBUGPUTS(("-"));
	DEBUGPUTS(("\n"));
#endif

	/* Inject code for GetCC, depending on CPU model */
	if (SysBase->AttnFlags & AFF_68010) {
		/* move.w %ccr,%d0; rts; nop */
		FAKE_IT(SysBase, Exec, GetCC, 88, 0x42c0, 0x4e75, 0x4e71);
	} else {
		/* move.w %sr,%d0; rts; nop */
		FAKE_IT(SysBase, Exec, GetCC, 88, 0x40c0, 0x4e75, 0x4e71);
	}

	for (i = 0; kickrom[i] != (UWORD *)~0; i+=2)
	    krnCreateROMHeader(mh, "Kickstart ROM", kickrom[i], kickrom[i+1]);

	/* Scan for all other ROM Tags */
	SysBase->ResModules = krnRomTagScanner(mh, kickrom);

	/* If we had Fast memory, don't forget to add
	 * Chip memory now!
	 */
	if (fast_size != 0) {
		krnCreateMemHeader("Chip Memory", -5,
				 (APTR)chip_start, chip_size,
				 MEMF_CHIP | MEMF_KICK | MEMF_PUBLIC | MEMF_LOCAL | MEMF_24BITDMA);
		mh = (APTR)chip_start;
		Enqueue(&SysBase->MemList,&mh->mh_Node);
	}

	oldmem = AvailMem(MEMF_FAST);

	/* Ok, let's start the system */
	DEBUGPUTS(("[start] InitCode(RTF_SINGLETASK, 0)\n"));
	InitCode(RTF_SINGLETASK, 0);

	/* Autoconfig ram expansions are now configured */

	if (oldsysbase && issysbasevalid(oldsysbase)) {
		/* sysbase found in autoconfig ram */
		SysBase = oldsysbase;
		DEBUGPUTHEX(("[SysBase] now valid at", (ULONG)SysBase));
		/* memory leak! we should free "temp" sysbase */
	} else if (AvailMem(MEMF_FAST) > oldmem + 256 * 1024 && (
		(TypeOfMem(SysBase) & MEMF_CHIP) ||
		((ULONG)SysBase >= 0x00a00000ul && (ULONG)SysBase < 0x01000000ul)
		)) {
			/* Move execbase to real fast if available now */
			PrepareExecBase(NULL, NULL, NULL);
			DEBUGPUTHEX(("[Sysbase] now at", (ULONG)SysBase));
	}
	
	/* total chipram */
	SysBase->MaxLocMem = (chip_size + 0xffff) & 0xffff0000;
	/* end of slow ram + 1 */
	SysBase->MaxExtMem = fast_size ? (APTR)(((0xc00000 + (fast_size + 0xffff)) & 0xffff0000)) : 0;

	/* reset proof our "rom" */
	if (romloader) {
		SysBase->ColdCapture = (APTR)(romloader - 8);
		DEBUGPUTHEX(("[romloader] coldcapture:", (ULONG)SysBase->ColdCapture));
	}

	setsyschecksum();

#ifdef THESE_ARE_KNOWN_SAFE_ASM_ROUTINES
	PRESERVE_ALL(SysBase, Exec, Disable, 20);
	PRESERVE_ALL(SysBase, Exec, Enable, 21);
	PRESERVE_ALL(SysBase, Exec, Forbid, 22);
#endif
	PRESERVE_ALL(SysBase, Exec, Permit, 23);
	PRESERVE_ALL(SysBase, Exec, ObtainSemaphore, 94);
	PRESERVE_ALL(SysBase, Exec, ReleaseSemaphore, 95);
	PRESERVE_ALL(SysBase, Exec, ObtainSemaphoreShared, 113);

	/* Functions that need sign extension */
	EXT_BYTE(SysBase, Exec, SetTaskPri, 50);
	EXT_BYTE(SysBase, Exec, AllocSignal, 55);

	/* Initialize IRQ subsystem */
	AmigaIRQInit(SysBase);

	/* Set privilege violation trap - we
	 * need this to support the Exec/Supervisor call
	 */
	trap[8] = Exec_Supervisor_Trap;

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
	DEBUGPUTS(("[DOS Task failed to start]\n"));
	for (;;)
	    Debug(0);
}
