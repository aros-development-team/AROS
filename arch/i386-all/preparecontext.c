/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.7  1998/10/20 16:43:19  hkiel
    Amiga Research OS

    Revision 1.6  1996/12/06 11:07:54  aros
    Vector shuffle

    Revision 1.5  1996/11/21 10:49:42  aros
    Created macros AROS_SLIB_ENTRY() for assembler files, too, to solve naming
    problems.

    The #includes in the header *must* begin in the first column. Otherwise
    makedepend will ignore them (GCC works, though).

    Removed a couple of Logs

    Revision 1.4  1996/10/24 15:51:12  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.3  1996/08/13 14:04:57  digulla
    Replaced AROS_LA by AROS_LHA

    Revision 1.2  1996/08/01 17:41:26  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/types.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	AROS_LH3I(APTR, PrepareContext,

/*  SYNOPSIS */
	AROS_LHA(APTR, stackPointer, A0),
	AROS_LHA(APTR, entryPoint,   A1),
	AROS_LHA(APTR, fallBack,     A2),

/*  LOCATION */
	struct ExecBase *, SysBase, 6, Exec)

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
    AROS_LIBFUNC_INIT
    UBYTE *sp=(UBYTE *)stackPointer;
    int i;

    /* i386 version. No FPU supported yet. */

    /* Push fallback address */
    sp-=sizeof(APTR);
    *(APTR *)sp=fallBack;

    /* Now push the context. Prepare returnaddress (pc). */
    sp-=sizeof(APTR);
    *(APTR *)sp=entryPoint;

    /* Push 7 registers */
    for(i=0;i<7;i++)
    {
	sp-=sizeof(LONG);
	*(LONG *)sp=0;
    }

    return sp;
    AROS_LIBFUNC_EXIT
} /* PrepareContext */
