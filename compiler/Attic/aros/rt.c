/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1997/01/27 00:17:41  ldp
    Include proto instead of clib

    Revision 1.2  1996/12/10 13:59:45  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.1  1996/08/23 17:26:44  digulla
    Files with functions for RT and Purify


    Desc:
    Lang:
*/
#define AROS_ALMOST_COMPATIBLE

#define ENABLE_RT 0	/* no RT inside this file */
#include <aros/rt.h>
#undef RT_Init
#undef RT_Leave

#include <exec/lists.h>
#include <aros/system.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <stdarg.h>
#include <stdlib.h>
#include <proto/exec.h>
#include <proto/aros.h>

extern struct ExecBase * SysBase;

struct CallStack
{
    char * Function;
    char * File;
    int    Line;
};

#define KEEPDEPTH   6
#define RT_STACKDEPTH	256

struct RTNode
{
    struct Node      Node;
    char	   * File;
    int 	     Line;
    struct CallStack Stack[KEEPDEPTH];
};

struct CallStack RT_CallStack[RT_STACKDEPTH];
int		 RT_StackPtr;

struct MemoryResource
{
    struct RTNode Node;
    void	* Memory;
    ULONG	  Size;
};

#define MAX_RESOURCES	2
static struct List RT_Resources[MAX_RESOURCES];
static int RT_Sizes[MAX_RESOURCES] =
{
    sizeof (struct MemoryResource),
};

static void RT_ShowStack (struct RTNode *);
static void RT_FreeResource (int rtt, struct RTNode *);
static int InitWasCalled;

/* Silently global functions */
void RT_ShowRTStack (void);

/*****************************************************************************

    NAME */
#include <aros/rt.h>

	void RT_Init (

/*  SYNOPSIS */
	void)

/*  FUNCTION
	Initialize the resource tracking

    INPUTS
	none

    RESULT
	none

    NOTES
	This function is not part of a library and may thus be called
	any time.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	24-12-95    digulla created

******************************************************************************/
{
    int t;

    for (t=0; t<MAX_RESOURCES; t++)
	NEWLIST (&RT_Resources[t]);

    InitWasCalled = 1;

    FindTask(NULL)->tc_UserData = RT_ShowRTStack;
} /* RT_Init */


/*****************************************************************************

    NAME */
#include <aros/rt.h>

	void RT_IntAdd (

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
	This function is not part of a library and may thus be called
	any time.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	24-12-95    digulla created

******************************************************************************/
{
    va_list args;
    struct RTNode * rtnew;
    int t;

    if (!InitWasCalled)
	return;

    if (!(rtnew = malloc (RT_Sizes[rtt])) )
    {
	kprintf ("RT_IntAdd: Out of memory\n");
	return;
    }

    rtnew->File = file;
    rtnew->Line = line;


    if (RT_StackPtr < KEEPDEPTH)
    {
	for (t=0; t<=RT_StackPtr; t++)
	{
	    rtnew->Stack[t] = RT_CallStack[t];
	}

	for ( ; t < KEEPDEPTH; t++)
	    rtnew->Stack[t].Function = NULL;
    }
    else
    {
	for (t=0; t<KEEPDEPTH; t++)
	{
	    rtnew->Stack[t] = RT_CallStack[RT_StackPtr - (KEEPDEPTH-1) + t];
	}
    }

    va_start (args, line);

    switch (rtt)
    {
    case RTT_MEMORY: {
	struct MemoryResource * rt = (struct MemoryResource *) rtnew;

	rt->Memory = va_arg (args, APTR);
	rt->Size = va_arg (args, ULONG);

	break; }

    }

    va_end (args);
} /* RT_IntAdd */

/*****************************************************************************

    NAME */
#include <aros/rt.h>

	void RT_IntCheck (

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
	This function is not part of a library and may thus be called
	any time.

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
	return;

    va_start (args, line);

    switch (rtt)
    {
    case RTT_MEMORY: {
	struct MemoryResource * rt;
	APTR memory;
	ULONG size;

	memory = va_arg (args, APTR);
	size = va_arg (args, ULONG);

	for (rt=GetHead (&RT_Resources[rtt]); rt; rt=GetSucc(rt))
	{
	    if (rt->Memory == memory)
	    {
		if (rt->Size == size)
		    return;

		kprintf ("RT: Size mismatch (%ld)\n", size);
		kprintf ("    MemPtr=%p Size=%ld\n", rt->Memory, rt->Size);
		RT_ShowStack (&rt->Node);

		return;
	    }
	}

	kprintf ("RT: Memory not found %p, Size=%ld\n", memory, size);
	kprintf ("    %s:%d\n", file, line);
	RT_ShowRTStack ();

	break; }

    }

    va_end (args);
} /* RT_IntCheck */

/*****************************************************************************

    NAME */
#include <aros/rt.h>

	void RT_IntFree (

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
	This function is not part of a library and may thus be called
	any time.

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
	return;

    va_start (args, line);

    switch (rtt)
    {
    case RTT_MEMORY: {
	struct MemoryResource * rt;
	APTR memory;
	ULONG size;

	memory = va_arg (args, APTR);
	size = va_arg (args, ULONG);

	for (rt=GetHead (&RT_Resources[rtt]); rt; rt=GetSucc(rt))
	{
	    if (rt->Memory == memory)
	    {
		if (rt->Size == size)
		{
		    Remove ((struct Node *)rt);
		    free (rt);

		    return;
		}

		kprintf ("RT: Size mismatch (%ld)\n", size);
		kprintf ("    MemPtr=%p Size=%ld\n", rt->Memory, rt->Size);
		RT_ShowStack (&rt->Node);

		return;
	    }
	}

	kprintf ("RT: Memory not found %p, Size=%ld\n", memory, size);
	kprintf ("    %s:%d\n", file, line);
	RT_ShowRTStack ();

	break; }

    }

    va_end (args);
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
	This function is not part of a library and may thus be called
	any time.

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

    if (RT_StackPtr == RT_STACKDEPTH)
	return;

    RT_CallStack[RT_StackPtr].Function = function;
    RT_CallStack[RT_StackPtr].File     = file;
    RT_CallStack[RT_StackPtr].Line     = line;

    RT_StackPtr ++;
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
	This function is not part of a library and may thus be called
	any time.

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

    if (!RT_StackPtr)
	return;

    RT_StackPtr --;

    RT_CallStack[RT_StackPtr].Function = NULL;
    RT_CallStack[RT_StackPtr].File     = NULL;
    RT_CallStack[RT_StackPtr].Line     = 0;
} /* RT_IntLeave */


/*****************************************************************************

    NAME */
#include <aros/rt.h>

	void RT_ShowStack (

/*  SYNOPSIS */
	struct RTNode * node)

/*  FUNCTION
	Prints the contents of the callstack stored in the node.

    INPUTS
	node - The node with the callstack to be printed

    RESULT
	none

    NOTES
	This function is not part of a library and may thus be called
	any time.

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

    for (t=0; t<KEEPDEPTH && node->Stack[t].Function; t++)
	kprintf ("    %s (%s:%d)\n"
	    , node->Stack[t].Function
	    , node->Stack[t].File
	    , node->Stack[t].Line
	);
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
	This function is not part of a library and may thus be called
	any time.

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
	struct RTNode * rtnode)

/*  FUNCTION
	Free a resource after the task that allocated it, died. Also
	print a nasty message about this.

    INPUTS
	rt - The node with the resource to be freed.

    RESULT
	none

    NOTES
	This function is not part of a library and may thus be called
	any time.

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
    case RTT_MEMORY: {
	struct MemoryResource * rt = (struct MemoryResource *)rtnode;

	/* Show the problem */
	kprintf ("RT: Freeing memory %p, Size=%ld, allocated at\n"
	    , rt->Memory
	    , rt->Size
	);
	RT_ShowStack (rtnode);

	/* free the resource */
	FreeMem (rt->Memory, rt->Size);

	break; }

    }

} /* RT_FreeResource */


