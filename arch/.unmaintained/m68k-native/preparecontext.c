/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:41:36  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/types.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */

	__AROS_LH3I(APTR, PrepareContext,

/*  SYNOPSIS */
	__AROS_LA(APTR, stackPointer, A0),
	__AROS_LA(APTR, entryPoint,   A1),
	__AROS_LA(APTR, fallBack,     A2),

/*  LOCATION */
	struct ExecBase *, SysBase, 9, Exec)

/*  FUNCTION
	Allocates the space required to hold a new set of registers on the
	Stack given by stackPointer and clears the area except for pc which
	is set to the address given by entryPoint.

    INPUTS
	stackPointer - Pointer to a scpecific stack
	entryPoint   - Address of the function to call when the new context
		       becomes active.
	fallBack     - Address to be called when the entryPoint function ended
		       with an rts.

    RESULT
	The new Stackpointer with the underlying context.

    NOTES
	This function is for internal use by exec only.

	This function is processor dependant.

    EXAMPLE

    BUGS

    SEE ALSO
	SwitchTasks()

    INTERNALS

    HISTORY

******************************************************************************/
{
    __AROS_FUNC_INIT
    UBYTE *sp=(UBYTE *)stackPointer;
    int i;

    /*
	mc68000 version. As long as no FPU is in use this works for the
	other mc680xx brands as well.
    */

    /* Push fallback address */
    sp-=sizeof(APTR);
    *(APTR *)sp=fallBack;

    /* Now push the context. First a5. */
    sp-=sizeof(LONG);
    *(LONG *)sp=0;

    /* Then a reverse rte (ccr & pc). */
    sp-=sizeof(WORD);
    *(WORD *)sp=0;
    sp-=sizeof(APTR);
    *(APTR *)sp=entryPoint;

    /* Push 14 registers */
    for(i=0;i<14;i++)
    {
	sp-=sizeof(LONG);
	*(LONG *)sp=0;
    }

    return sp;
    __AROS_FUNC_EXIT
}
