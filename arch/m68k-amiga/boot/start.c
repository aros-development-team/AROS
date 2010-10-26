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

#include "exec_intern.h"
#include "kernel_romtags.h"

#define USE_GDBSTUB

extern const struct Resident Exec_resident;

extern void __clear_bss(const struct KernelBSS *bss);

/** Screen/Serial Debug **/

#define SERPER_BASE_PAL		3546895
#define SERPER_BASE_NTSC	3579545
#define SERPER			0x32
#define   SERPER_BAUD(base, x)	(((base)/(x)-1) & 0x7fff)	/* Baud rate */
#define SERDATR			0x18
#define   SERDATR_OVRUN		(1 << 15)	/* Overrun */
#define   SERDATR_RBF		(1 << 14)	/* Rx Buffer Full */
#define   SERDATR_TBE		(1 << 13)	/* Tx Buffer Empty */
#define   SERDATR_TSRE		(1 << 12)	/* Tx Shift Empty */
#define   SERDATR_RXD		(1 << 11)	/* Rx Pin */
#define   SERDATR_STP9		(1 <<  9)	/* Stop bit (if 9 data) */
#define   SERDATR_STP8		(1 <<  8)	/* Stop bit (if 8 data) */
#define   SERDATR_DB9_of(x)	((x) & 0x1ff)	/* 9-bit data */
#define   SERDATR_DB8_of(x)	((x) & 0xff)	/* 8-bit data */
#define ADKCON			0x9e
#define   ADKCON_SETCLR		(1 << 15)
#define   ADKCON_UARTBRK	(1 << 11)	/* Force break */
#define SERDAT			0x30
#define   SERDAT_STP9		(1 << 9)
#define   SERDAT_STP8		(1 << 8)
#define   SERDAT_DB9(x)		((x) & 0x1ff)
#define   SERDAT_DB8(x)		((x) & 0xff)

#define BPLCON0			0x100
#define BPL1DAT			0x110
#define COLOR00			0x180

static inline void reg_w(ULONG reg, UWORD val)
{
	volatile UWORD *r = (void *)(0xdff000 + reg);

	*r = val;
}

static inline UWORD reg_r(ULONG reg)
{
	volatile UWORD *r = (void *)(0xdff000 + reg);

	return *r;
}

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

void start(void)
{
	extern void *_bss;
	extern void *_bss_end;
	extern void *_stack;
	extern void *_stack_end;
	APTR *tmp;
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

        sysBase->SysStkUpper    = (APTR)&_stack_end-1;
        sysBase->SysStkLower    = (APTR)&_stack;
	sysBase->ChkBase=~(ULONG)sysBase;

	/* TODO: Actually check this! */
	sysBase->AttnFlags |= AFF_68020;

	/* Scan for all other ROM Tags */
	sysBase->ResModules = krnRomTagScanner(sysBase, kickrom);
	DebugPuts("[start] InitCode(RTF_SINGLETASK, 0)\n");
	InitCode(RTF_SINGLETASK, 0);

	/* We shouldn't get here */
	DebugPuts("[DOS Task failed to start]\n");
	for (;;)
		breakpoint();
}
