#ifndef AROS_M68K_FPUCONTEXT_H
#define AROS_M68K_FPUCONTEXT_H

#include <aros/asmcall.h>

#include <aros/m68k/fenv.h>
#include <aros/m68k/_fpmath.h>

struct FpuContext {
	fenv_t fpenv;		/* User-visible status register */
	union IEEEl2bits fp[8];	/* FP registers */
	union {
		UBYTE fpu68881[0xb8];
		UBYTE fpu68882[0xd8];
		UBYTE fpu68040[0x60];
		UBYTE fpu68060[0x12];
	} fsave;		/* FSAVE context (CPU specific) */
};

/* Defined in arch/m68k-all/kernel/?.S */

AROS_UFP2(void, FpuSaveContext,
		AROS_UFPA(struct FpuContext *, fpu, A0),
		AROS_UFHA(UWORD, nulloffset, D0));

AROS_UFP2(void, FpuRestoreContext,
		AROS_UFPA(struct FpuContext *, fpu, A0),
		AROS_UFPA(UWORD, nulloffset, D0));


#endif
