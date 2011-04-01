#ifndef AROS_M68K_CPUCONTEXT_H
#define AROS_M68K_CPUCONTEXT_H

/*
 * We don't need ULONG Flags in this context, since the
 *  SysBase->AttnFlags provides the CPU type information.
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
