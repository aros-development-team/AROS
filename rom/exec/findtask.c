/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1997/01/01 03:46:10  ldp
    Committed Amiga native (support) code

    Changed clib to proto

    Revision 1.6  1996/12/10 13:51:45  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.5  1996/10/24 15:50:49  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:02  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:11  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(struct Task *, FindTask,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, name, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 49, Exec)

/*  FUNCTION
	Find a task with a given name or get the address of the current task.
	Finding the address of the current task is a very quick function
	call, but finding a special task is a very CPU intensive instruction.
	Note that generally a task may already be gone when this function
	returns.

    INPUTS
	name - Pointer to name or NULL for current task.

    RESULT
	Address of task structure found.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task *ret;

    /* Quick return for a quick argument */
    if(name==NULL)
	return SysBase->ThisTask;

    /* Always protect task lists with a Disable(). */
    Disable();

    /* First look into the ready list. */
    ret=(struct Task *)FindName(&SysBase->TaskReady,name);
    if(ret==NULL)
    {
	/* Then into the waiting list. */
	ret=(struct Task *)FindName(&SysBase->TaskWait,name);
	if(ret==NULL)
	{
	    /*
		Finally test the current task. Note that generally
		you know the name of your own task - so it is close
		to nonsense to look for it this way.
	    */
	    char *s1=SysBase->ThisTask->tc_Node.ln_Name;
	    char *s2=name;

	    /* Check as long as the names are identical. */
	    while(*s1++==*s2)
		/* Terminator found? */
		if(!*s2++)
		{
		    /* Got it. */
		    ret=SysBase->ThisTask;
		    break;
		}
	}
    }

    /* Return whatever I found. */
    Enable();
    return ret;
    AROS_LIBFUNC_EXIT
} /* FindTask */

