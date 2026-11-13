#ifndef AROS_M68K_AMMXCONTEXT_H
#define AROS_M68K_AMMXCONTEXT_H

#include <aros/asmcall.h>
#include <exec/types.h>

struct AMMXContext {
    ULONG DnHigh[8];
    UQUAD En[24];
    ULONG Bn[8];
};

/* Defined in arch/m68k-all/kernel/?.S */

AROS_UFP1(void, AMMXSaveContext,
		AROS_UFPA(struct AMMXContext *, ammx, A0));

AROS_UFP1(void, AMMXRestoreContext,
		AROS_UFPA(struct AMMXContext *, ammx, A0));


#endif
