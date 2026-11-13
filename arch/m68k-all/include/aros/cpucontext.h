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
	UWORD sr;
	IPTR  pc;
} __packed;

/* Special hack for setting the 'Z' condition code upon exit
 * for m68k architectures.
 */
#define AROS_INTFUNC_INIT inline ULONG _handler(void) {
#define AROS_INTFUNC_EXIT }; register ULONG _res asm ("d0") = _handler();     \
                             asm volatile ("tst.l %0\n" : : "r" (_res)); \
                             return _res; /* gcc only generates movem/unlk/rts */   \
                             AROS_USERFUNC_EXIT }

#endif
