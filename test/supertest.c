/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.4  1998/10/20 16:46:39  hkiel
    Amiga Research OS

    Revision 1.3  1998/04/13 22:50:02  hkiel
    Include <proto/exec.h>

    Revision 1.2  1996/08/01 17:41:40  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <proto/exec.h>
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

