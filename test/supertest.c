/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:41:40  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/types.h>
#include <clib/exec_protos.h>
#include <stdio.h>

#ifdef __GNUC__
asm("
	.globl	_a
_a:
	movew	sr,d0
	rts
	.globl	_b
_b:
	movew	sp@,d0
	rte
");
#endif

extern UWORD a(void);
extern UWORD b(void);

int main(void)
{
    APTR ssp;
    UWORD ar;
    printf("%04x\n",GetCC());
    printf("%04lx\n",SetSR(0,0));
    ssp=SuperState();
    ar=a();
    UserState(ssp);
    printf("%04x\n",a());
    printf("%04x\n",(UWORD)Supervisor((ULONG_FUNC)&b));
    return 0;
}

