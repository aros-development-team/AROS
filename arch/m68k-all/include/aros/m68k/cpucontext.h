#ifndef AROS_M68K_CPUCONTEXT_H
#define AROS_M68K_CPUCONTEXT_H

/*
 * Preliminary and subject to change! Do not rely on this for now, please.
 * At least the following needs to be done:
 * 1. Add ULONG Flags in the beginning, to allow expansions (see other CPUs)
 * 2. Add FPU context, either directly or as a pointer.
 */

struct ExceptionContext
{
	ULONG d[8];
	IPTR  a[8];
	IPTR  trapcode;
	ULONG traparg;
	UWORD sr;
	IPTR  pc;
};

#endif
