/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Basic functions for ressource tracking
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
#define ENABLE_RT 0	/* no RT inside this file */
#define RT_INTERNAL 1
#include <aros/rt.h>

#include <exec/lists.h>
#include <aros/system.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <stdarg.h>
#include <stdlib.h>
#include <proto/exec.h>
#include <proto/arossupport.h>

static BOOL InitWasCalled;

typedef struct
{
    struct MinNode Node;
    const char	 * File;
    ULONG	   Line;
}
RTNode;

typedef struct
{
    RTNode Node;
    APTR   Memory;
    ULONG  Size;
    ULONG  Flags;
}
MemoryResource;

static ULONG RT_Sizes[] =
{
    sizeof (MemoryResource),
};


typedef struct
{
    const char * Function;
    const char * File;
    ULONG	 Line;
}
RTStack;

#define STACKDEPTH  256

ULONG RT_StackPtr = STACKDEPTH;
static RTStack RT_CallStack[STACKDEPTH];

struct MinList rtMemList;

static void RT_FreeResource (int rtt, RTNode * rtnode);

/*****************************************************************************

    NAME */
#include <aros/rt.h>

	void RT_Init (

/*  SYNOPSIS */
	void)

/*  FUNCTION
	Initialize the resource tracking.

    INPUTS
	none

    RESULT
	none

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	24-12-95    digulla created

******************************************************************************/
{
    if (InitWasCalled)
	return;

    InitWasCalled = 1;

    NEWLIST(&rtMemList);
} /* RT_Init */

/*****************************************************************************

    NAME */
#include <aros/rt.h>

	void RT_Exit (

/*  SYNOPSIS */
	void)

/*  FUNCTION
	Stops the resource tracking. All resources which are still allocated
	are printed and then released.

    INPUTS
	none

    RESULT
	none

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	24-12-95    digulla created

******************************************************************************/
{
    RTNode * rt;

    if (!InitWasCalled)
	return;

    ForeachNode (&rtMemList, rt)
	RT_FreeResource (RTT_MEMORY, rt);
} /* RT_Exit */

/*****************************************************************************

    NAME */
#include <aros/rt.h>

	IPTR RT_IntAdd (

/*  SYNOPSIS */
	int    rtt,
	char * file,
	int    line,
	...)

/*  FUNCTION
	Adds a resource to be tracked. The arguments after
	line depend on the type of resource to be traced:

	RTT_MEMORY:	APTR	      memPtr,
			ULONG	      size)

    INPUTS
	rtt - Type of the resource
	file - The file RT_IntAdd was called it
	line - The line of the file
	task - The task to be added
	memPtr - Pointer to a piece of memory to be tracked
	size - The size of the memory beginning at memPtr

    RESULT
	none

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	24-12-95    digulla created

******************************************************************************/
{
    va_list args;
    RTNode * rtnew;

    if (!InitWasCalled)
	return FALSE;

    if (!(rtnew = AllocMem (RT_Sizes[rtt], MEMF_ANY)) )
    {
	kprintf ("RT_IntAdd: Out of memory\n");
	return FALSE;
    }

    rtnew->File = file;
    rtnew->Line = line;

    va_start (args, line);

    switch (rtt)
    {
	case RTT_MEMORY:
	{
	    MemoryResource * rt = (MemoryResource *)rtnew;

	    rt->Size = va_arg (args, ULONG);
	    rt->Flags = va_arg (args, ULONG);

	    rt->Memory = AllocMem (rt->Size, rt->Flags);

	    if (!rt->Memory)
	    {
		FreeMem (rtnew, RT_Sizes[rtt]);
		return NULL;
	    }
kprintf ("Allocated mem at %s:%d (%s:%d)\n", file, line, rt->Node.File, rt->Node.Line);
	    AddTail ((struct List *)&rtMemList, (struct Node *)rtnew);

	    return (IPTR)(rt->Memory);
	}
    } /* switch */

    va_end (args);

    return 0;
} /* RT_IntAdd */

/*****************************************************************************

    NAME */
#include <aros/rt.h>

	IPTR RT_IntCheck (

/*  SYNOPSIS */
	int    rtt,
	char * file,
	int    line,
	...)

/*  FUNCTION
	Checks a resource. Will print an error if the resource is not found
	or has already been freed or if the type of the resource doesn't
	match. The arguments after line depend on the type of resource to
	be traced:

	RTT_MEMORY:	APTR	      memPtr,
			ULONG	      size)

    INPUTS
	rtt - Type of the resource
	file - The file RT_IntCheck was called it
	line - The line of the file
	task - The task to be added
	memPtr - Pointer to a piece of memory to be tracked
	size - The size of the memory beginning at memPtr

    RESULT
	none

    NOTES
	All strings must be static.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	24-12-95    digulla created

******************************************************************************/
{
    va_list args;

    if (!InitWasCalled)
	return FALSE;

    va_start (args, line);

    switch (rtt)
    {
	case RTT_MEMORY:
	{
	    MemoryResource * rt;
	    APTR memory;
	    ULONG size;

	    memory = va_arg (args, APTR);
	    size = va_arg (args, ULONG);

	    ForeachNode (&rtMemList, rt)
	    {
		if (rt->Memory == memory)
		{
		    if (rt->Size == size)
			return TRUE;

		    kprintf ("RTCheck: Size mismatch (Allocated=%ld, Check=%ld)\n"
			    "    Check at %s:%d\n"
			    "    Allocated at %s:%d\n"
			    "    MemPtr=%p Size=%ld Flags=%08lx\n"
			, rt->Size, size
			, file, line
			, rt->Node.File, rt->Node.Line
			, rt->Memory, rt->Size, rt->Flags
		    );

		    return FALSE;
		}
	    }

	    kprintf ("RTCheck: Memory not found\n"
		    "    Check at %s:%d\n"
		    "    MemPtr=%p Size=%ld\n"
		, file, line
		, memory, size
	    );

	    return FALSE;
	}
    }

    va_end (args);

    return FALSE;
} /* RT_IntCheck */

/*****************************************************************************

    NAME */
#include <aros/rt.h>

	IPTR RT_IntFree (

/*  SYNOPSIS */
	int    rtt,
	char * file,
	int    line,
	...)

/*  FUNCTION
	Stops tracing of a resource. The arguments after
	line depend on the type of resource to be traced:

	RTT_MEMORY:	APTR	      memPtr,
			ULONG	      size)

    INPUTS
	rtt - Type of the resource
	file - The file RT_IntAdd was called it
	line - The line of the file
	task - The task to be added
	memPtr - Pointer to a piece of memory to be tracked
	size - The size of the memory beginning at memPtr

    RESULT
	none

    NOTES
	All strings must be static.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	24-12-95    digulla created

******************************************************************************/
{
    va_list args;

    if (!InitWasCalled)
	return FALSE;

    va_start (args, line);

    switch (rtt)
    {
	case RTT_MEMORY:
	{
	    MemoryResource * rt;
	    APTR memory;
	    ULONG size;

	    memory = va_arg (args, APTR);
	    size = va_arg (args, ULONG);

	    ForeachNode (&rtMemList, rt)
	    {
		if (rt->Memory == memory)
		{
		    if (rt->Size == size)
		    {
			Remove ((struct Node *)rt);
			FreeMem (rt, RT_Sizes[rtt]);
			FreeMem (memory, size);
			return TRUE;
		    }

		    kprintf ("RTFree: Size mismatch (Allocated=%ld, Check=%ld)\n"
			    "    Free at %s:%d\n"
			    "    Allocated at %s:%d\n"
			    "    MemPtr=%p Size=%ld Flags=%08lx\n"
			, rt->Size, size
			, file, line
			, rt->Node.File, rt->Node.Line
			, rt->Memory, rt->Size, rt->Flags
		    );

		    return FALSE;
		}
	    }

	    kprintf ("RTFree: Memory not found\n"
		    "    Free at %s:%d\n"
		    "    MemPtr=%p Size=%ld\n"
		, file, line
		, memory, size
	    );

	    return FALSE;
	}
    } /* switch */

    va_end (args);

    return FALSE;
} /* RT_IntFree */


/*****************************************************************************

    NAME */
#include <aros/rt.h>

	void RT_IntEnter (

/*  SYNOPSIS */
	char * function,
	char * file,
	int    line)

/*  FUNCTION
	Tells the RT that a new function is about to be entered. This is used
	to make it easier to track an error.

    INPUTS
	function - name of the function to be entered
	file - file with the call of the function
	line - Line-number of the call

    RESULT
	none

    NOTES
	All strings must be static.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	24-12-95    digulla created

******************************************************************************/
{
    if (!InitWasCalled)
	return;

    if (RT_StackPtr == 0)
	return;

    -- RT_StackPtr;

    RT_CallStack[RT_StackPtr].Function = function;
    RT_CallStack[RT_StackPtr].File     = file;
    RT_CallStack[RT_StackPtr].Line     = line;
} /* RT_IntEnter */


/*****************************************************************************

    NAME */
#include <aros/rt.h>

	void RT_IntLeave (

/*  SYNOPSIS */
	void)

/*  FUNCTION
	Tells the RT that a the function has been left.

    INPUTS
	none

    RESULT
	none

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	24-12-95    digulla created

******************************************************************************/
{
    if (!InitWasCalled)
	return;

    if (RT_StackPtr == STACKDEPTH)
	return;

    RT_CallStack[RT_StackPtr].Function = NULL;
    RT_CallStack[RT_StackPtr].File     = NULL;
    RT_CallStack[RT_StackPtr].Line     = 0;

    RT_StackPtr ++;
} /* RT_IntLeave */


/*****************************************************************************

    NAME */
#include <aros/rt.h>

	void RT_ShowStack (

/*  SYNOPSIS */
	RTNode * node)

/*  FUNCTION
	Prints the contents of the callstack stored in the node.

    INPUTS
	node - The node with the callstack to be printed

    RESULT
	none

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	24-12-95    digulla created

******************************************************************************/
{
#if 0
    int t;

    if (!InitWasCalled)
	return;

    for (t=0; t<KEEPDEPTH && node->Stack[t].Function; t++)
	kprintf ("    %s (%s:%d)\n"
	    , node->Stack[t].Function
	    , node->Stack[t].File
	    , node->Stack[t].Line
	);
#endif
} /* RT_ShowStack */


/*****************************************************************************

    NAME */
#include <aros/rt.h>

	void RT_ShowRTStack (

/*  SYNOPSIS */
	void)

/*  FUNCTION
	Prints the contents of the callstack built by RT_Enter() and
	RT_Leave().

    INPUTS
	none

    RESULT
	none

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	24-12-95    digulla created

******************************************************************************/
{
    int t;

    if (!InitWasCalled)
	return;

    for (t=0; t<=RT_StackPtr; t++)
    {
	kprintf ("    %s (%s:%d)\n"
	    , RT_CallStack[t].Function
	    , RT_CallStack[t].File
	    , RT_CallStack[t].Line
	);
    }
} /* RT_ShowRTStack */


/*****************************************************************************

    NAME */
#include <aros/rt.h>

	void RT_FreeResource (

/*  SYNOPSIS */
	int rtt,
	RTNode * rtnode)

/*  FUNCTION
	Free a resource after the task that allocated it, died. Also
	print a nasty message about this.

    INPUTS
	rt - The node with the resource to be freed.

    RESULT
	none

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	Don't free the node or remove it from the list. This is done
	elsewhere.

    HISTORY
	24-12-95    digulla created

******************************************************************************/
{
    if (!InitWasCalled)
	return;

    switch (rtt)
    {
	case RTT_MEMORY:
	{
	    MemoryResource * rt = (MemoryResource *)rtnode;

	    /* Show the problem */
	    kprintf ("RTExit: Freeing memory\n"
		    "    Allocated at %s:%d\n"
		    "    MemPtr=%p Size=%ld Flags=%08lx\n"
		, rt->Node.File, rt->Node.Line
		, rt->Memory, rt->Size, rt->Flags
	    );

	    /* free the resource */
	    FreeMem (rt->Memory, rt->Size);

	    break;
	}
    }

} /* RT_FreeResource */


