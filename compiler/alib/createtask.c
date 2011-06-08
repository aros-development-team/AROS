/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create a new Amiga task
    Lang: english
*/

#include <exec/memory.h>
#include <exec/execbase.h>
#include <proto/exec.h>

struct newMemList
{
  struct Node	  nml_Node;
  UWORD 	  nml_NumEntries;
  struct MemEntry nml_ME[2];
};

static const struct newMemList MemTemplate =
{
    { 0, },
    2,
    {
	{ { MEMF_CLEAR|MEMF_PUBLIC }, sizeof(struct Task) },
	{ { MEMF_CLEAR		   }, 0 		  }
    }
};


/*****************************************************************************

    NAME */
#include <exec/tasks.h>
#include <proto/alib.h>

	struct Task * CreateTask (

/*  SYNOPSIS */
	STRPTR name,
	LONG   pri,
	APTR   initpc,
	ULONG  stacksize)

/*  FUNCTION
	Create a new task.

    INPUTS
	name - Name of the task. The string is not copied. Note that
	    task names' need not be unique.
	pri - The initial priority of the task (normally 0)
	initpc - The address of the first instruction of the
	    task. In most cases, this is the address of a
	    function.
	stacksize - The size of the stack for the task. Always
	    keep in mind that the size of the stack must include
	    the amount of stack which is needed by the routines
	    called by the task.

    RESULT
	A pointer to the new task or NULL on failure.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    return NewCreateTask(TASKTAG_NAME, name,
    		         TASKTAG_PRI, pri,
    		         TASKTAG_PC, initpc,
    		         TASKTAG_STACKSIZE, stacksize,
    		         TAG_END);
} /* CreateTask */
