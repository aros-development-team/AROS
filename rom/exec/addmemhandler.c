/*****************************************************************************

    NAME */
	#include <exec/execbase.h>
	#include <clib/exec_protos.h>

__AROS_LH1(void, AddMemHandler,

/*  SYNOPSIS */
	__AROS_LA(struct Interrupt *, memHandler, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 129, Exec)

/*  FUNCTION
	Add some function to be called if the system is low on memory.

    INPUTS
	memHandler - An Interrupt structure to add to the low memory
		     handler list.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	8-10-95    created by m. fleischer

******************************************************************************/
{
    __AROS_FUNC_INIT
    /* Protect the low memory handler list */
    Forbid();
	/* Nothing spectacular: Just add the new node */
	Enqueue((struct List *)&SysBase->ex_MemHandlers,&memHandler->is_Node);
    Permit();
    __AROS_FUNC_EXIT
} /* AddMemHandler */

