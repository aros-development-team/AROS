#ifndef AROS_M68K_FPUCONTEXT_H
#define AROS_M68K_FPUCONTEXT_H

#include <aros/asmcall.h>

#include <aros/m68k/fenv.h>
#include <aros/m68k/_fpmath.h>

struct FpuContext {
	fenv_t fpenv;		/* User-visible status register */
	union IEEEl2bits fp[8];	/* FP registers */
	union {
		UBYTE mc68881[0xb8];
		UBYTE mc68882[0xd8];
		UBYTE mc68040[0x60];
		UBYTE mc68060[0x12];
	} fsave;		/* FSAVE context (CPU specific) */
};

/* Defined in arch/m68k-all/kernel/?.S */

AROS_UFP1(void, FpuSaveContext,
		AROS_UFPA(struct FpuContext *, fpu, A0));

AROS_UFP1(void, FpuRestoreContext,
		AROS_UFPA(struct FpuContext *, fpu, A0));


#endif
