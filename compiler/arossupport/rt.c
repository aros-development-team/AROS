/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Basic functions for ressource tracking
    Lang: english
*/
#include "rt.h"
#if 0
#define ENABLE_RT 0	/* no RT inside this file */
#define RT_INTERNAL 1
#include <aros/rt.h>

#include <exec/lists.h>
#include <aros/system.h>
#include <exec/tasks.h>
#include <exec/ports.h>
#include <exec/execbase.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/alib.h>
#include "etask.h"
#endif
#include <string.h>
#include <exec/memory.h>
#include <proto/arossupport.h>
#include <proto/exec.h>

/* iaint: Use the debugging macros from now on. */
#include <aros/debug.h>

RTData * intRTD = NULL; /* Internal pointer in case no ETask is available */
RTDesc const * RT_Resources[RTT_MAX];

/*****************************************************************************

    NAME */
	#include <aros/rt.h>

	void RT_IntInitB (

/*  SYNOPSIS */
	void)

/*  FUNCTION
	Begin initialization of resource tracking.

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
    AROS_GET_SYSBASE_OK
    RTData * rtd;

    if ((rtd = GetRTData ()))
    {
	D(bug ("RT_Init() called twice %p\n", rtd) );
	return;
    }

    if (!(rtd = AllocMem (sizeof (RTData), MEMF_ANY)) )
    {
	D(bug ("RT_Init(): No memory\n") );
	return;
    }

    SetRTData (rtd);

    rtd->rtd_Version  = RT_VERSION;
    rtd->rtd_StackPtr = STACKDEPTH;
} /* RT_IntInitB */

/*****************************************************************************

    NAME */
	#include <aros/rt.h>

	void RT_IntInitE (

/*  SYNOPSIS */
	void)

/*  FUNCTION
	End initialization of resource tracking.

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
    RTData * rtd;
    int      t, i;

    rtd = GetRTData ();

    for (t=0; t<RTT_MAX; t++)
    {
	for (i=0; i<HASH_SIZE; i++)
	{
	    NEWLIST(&rtd->rtd_ResHash[t][i]);
	}
    }

    D(bug ("RT_Init(): RT up and kicking in %s mode\n"
	, intRTD ? "internal" : "ETask"
    ) );
} /* RT_IntInitE */

/*****************************************************************************

    NAME */
	#include <aros/rt.h>

	void RT_IntExitB (

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
	Free the resources from back to front. This allows to sort the
	resources (ie. windows on a screen are closed before the screen,
	etc).

    HISTORY
	24-12-95    digulla created

******************************************************************************/
{
    RTData * rtd;
    RTNode * rt, * next;
    int      t, i;

    if (!(rtd = GetRTData ()) )
	return;

    for (t=RTT_MAX-1; t>=0; t--)
    {
	for (i=0; i<HASH_SIZE; i++)
	{
	    for (next=(RTNode *)GetHead(&rtd->rtd_ResHash[t][i]); (rt=next); )
	    {
		next = (RTNode *)GetSucc (rt);

		RT_FreeResource (rtd, t, rt);
	    }
	}
    }
} /* RT_IntExitB */

/*****************************************************************************

    NAME */
	#include <aros/rt.h>

	void RT_IntExitE (

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
	Free the resources from back to front. This allows to sort the
	resources (ie. windows on a screen are closed before the screen,
	etc).

    HISTORY
	24-12-95    digulla created

******************************************************************************/
{
    AROS_GET_SYSBASE_OK
    RTData * rtd;

    if (!(rtd = GetRTData ()) )
	return;

    FreeMem (rtd, sizeof (RTData));

    SetRTData (NULL);
} /* RT_IntExitE */

/*****************************************************************************

    NAME */
	#include <aros/rt.h>

	IPTR RT_IntAdd (

/*  SYNOPSIS */
	int	     rtt,
	const char * file,
	int	     line,
	...)

/*  FUNCTION
	Adds a resource to be tracked. The arguments after
	line depend on the type of resource to be traced:

	RTT_ALLOCMEM:	  APTR		memPtr,
			  ULONG 	size)

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
    AROS_GET_SYSBASE_OK
    RTData * rtd;
    IPTR     ret;
    va_list  args;
    RTNode * rtnew;
    BOOL     success;

    if (!(rtd = GetRTData ()) )
	return FALSE;

    if (!(rtnew = AllocMem (GetRTSize(rtt), MEMF_ANY)) )
    {
	D(bug ("RT_IntAdd: Out of memory\n") );
	return FALSE;
    }

    rtnew->File = file;
    rtnew->Line = line;
    rtnew->Flags = 0;
    
    va_start (args, line);

    success = FALSE;

    ret = (*GetRTAllocFunc(rtt))
    (
	rtd,
	rtnew,
	args,
	&success
    );

    va_end (args);

    if (success)
    {
	AddTail ((struct List *)&rtd->rtd_ResHash[rtt][CALCHASH(HASH_BASE(rtnew))]
	    ,(struct Node *)rtnew
	);
    }
    else
    {
	FreeMem (rtnew, GetRTSize(rtt));
    }

    return ret;
} /* RT_IntAdd */

/*****************************************************************************

    NAME */
	#include <aros/rt.h>

	void RT_IntTrack (

/*  SYNOPSIS */
	int	     rtt,
	const char * file,
	int	     line,
	APTR	     res,
	...)

/*  FUNCTION
	Adds a resource to be tracked. The arguments after
	line depend on the type of resource to be traced.
	The resource›is marked as "must not be freed by the
	user".

    INPUTS
	rtt - Type of the resource
	file - The file RT_IntAdd was called it
	line - The line of the file
	res - Pointer to the resouce

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
    AROS_GET_SYSBASE_OK
    RTData   * rtd;
    va_list    args;
    Resource * rtnew;

    if (!(rtd = GetRTData ()) )
	return;

    if (!(rtnew = AllocMem (GetRTSize(rtt), MEMF_ANY|MEMF_CLEAR)) )
    {
	D(bug("RT_IntAdd: Out of memory\n"));
    }
    else
    {
	rtnew->Node.File  = file;
	rtnew->Node.Line  = line;
	rtnew->Node.Flags = RTNF_DONT_FREE;
	rtnew->Resource   = res;

	va_start (args, res);

	va_end (args);

	AddTail ((struct List *)&rtd->rtd_ResHash[rtt][CALCHASH(HASH_BASE(rtnew))]
	    , (struct Node *)rtnew
	);
    }
} /* RT_IntTrack */

/*****************************************************************************

    NAME */
	#include <aros/rt.h>

	IPTR RT_IntCheck (

/*  SYNOPSIS */
	int	     rtt,
	const char * file,
	int	     line,
	int	     op,
	...)

/*  FUNCTION
	Checks a resource. Will print an error if the resource is not found
	or has already been freed or if the type of the resource doesn't
	match. The arguments after line depend on the type of resource to
	be traced:

	RTT_ALLOCMEM:	  APTR		memPtr,
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
    RTData * rtd;
    RTNode * rt;
    va_list  args;
    IPTR     ret;

    if (!(rtd = GetRTData ()) )
	return FALSE;

    va_start (args, op);

    if (GetRTCheckFunc(rtt))
    {
	ret = (*(GetRTCheckFunc(rtt)))
	(
	    rtd,
	    rtt,
	    file,
	    line,
	    op,
	    args
	);
    }
    else
    {
	ret = (*(GetRTSearchFunc(rtt)))
	(
	    rtd,
	    rtt,
	    &rt,
	    args
	);

	if (ret != RT_SEARCH_FOUND)
	{
	    ret = (*(GetRTShowError(rtt)))
	    (
		rtd,
		rtt,
		rt,
		ret,
		RT_CHECK,
		file,
		line,
		args
	    );
	}
    }

    va_end (args);

    return ret;
} /* RT_IntCheck */

/*****************************************************************************

    NAME */
	#include <aros/rt.h>

	IPTR RT_IntFree (

/*  SYNOPSIS */
	int	     rtt,
	const char * file,
	int	     line,
	...)

/*  FUNCTION
	Stops tracing of a resource. The arguments after
	line depend on the type of resource to be traced.

    INPUTS
	rtt - Type of the resource
	file - The file RT_IntAdd was called it
	line - The line of the file

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
    RTData * rtd;
    IPTR     ret;
    va_list  args;
    RTNode * rt;

    if (!(rtd = GetRTData ()) )
	return FALSE;

    va_start (args, line);

    ret = (*(GetRTSearchFunc(rtt)))
    (
	rtd,
	rtt,
	&rt,
	args
    );

    if (ret == RT_SEARCH_FOUND && rt && !(rt->Flags & RTNF_DONT_FREE))
    {
	ret = (*(GetRTFreeFunc(rtt))) (rtd, rt);

	Remove ((struct Node *)rt);
	FreeMem (rt, GetRTSize(rtt));
    }
    else
    {
	ret = (*(GetRTShowError(rtt)))
	(
	    rtd,
	    rtt,
	    rt,
	    ret,
	    RT_FREE,
	    file,
	    line,
	    args
	);
    }

    va_end (args);

    return ret;
} /* RT_IntFree */


/*****************************************************************************

    NAME */
	#include <aros/rt.h>

	void RT_IntEnter (

/*  SYNOPSIS */
	const char * function,
	const char * file,
	int	     line)

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
    RTData * rtd;

    if (!(rtd = GetRTData ()) )
	return;

    if (rtd->rtd_StackPtr == 0)
	return;

    -- rtd->rtd_StackPtr;

    rtd->rtd_CallStack[rtd->rtd_StackPtr].Function = function;
    rtd->rtd_CallStack[rtd->rtd_StackPtr].File	   = file;
    rtd->rtd_CallStack[rtd->rtd_StackPtr].Line	   = line;
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
    RTData * rtd;

    if (!(rtd = GetRTData ()) )
	return;

    if (rtd->rtd_StackPtr == STACKDEPTH)
	return;

    rtd->rtd_CallStack[rtd->rtd_StackPtr].Function = NULL;
    rtd->rtd_CallStack[rtd->rtd_StackPtr].File	   = NULL;
    rtd->rtd_CallStack[rtd->rtd_StackPtr].Line	   = 0;

    rtd->rtd_StackPtr ++;
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
    RTData * rtd;
    int      t;

    if (!(rtd = GetRTData ()) )
	return;

    for (t=0; t<KEEPDEPTH && node->Stack[t].Function; t++)
	D(bug(("    %s (%s:%d)\n"
	    , node->Stack[t].Function
	    , node->Stack[t].File
	    , node->Stack[t].Line
	));
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
    RTData * rtd;
    int t;

    if (!(rtd = GetRTData ()) )
	return;

    for (t=0; t<=rtd->rtd_StackPtr; t++)
    {
	D(bug("    %s (%s:%d)\n"
	    , rtd->rtd_CallStack[t].Function
	    , rtd->rtd_CallStack[t].File
	    , rtd->rtd_CallStack[t].Line
	));
    }
} /* RT_ShowRTStack */


/*****************************************************************************

    NAME */
	#include <aros/rt.h>

	void RT_FreeResource (

/*  SYNOPSIS */
	RTData * rtd,
	int	 rtt,
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
    AROS_GET_SYSBASE_OK
    if (!rtd && !(rtd = GetRTData ()))
	return;

    if (!(rtnode->Flags & RTNF_DONT_FREE) )
    {
	/* Print an error */
	(void) (*(GetRTShowError(rtt)))
	(
	    rtd,
	    rtt,
	    rtnode,
	    0UL,
	    RT_EXIT,
	    NULL,
	    0L,
	    NULL
	);

	/* free the resource */
	(void) (*(GetRTFreeFunc(rtt))) (rtd, rtnode);
    }

    /* Remove resource from list and free it */
    Remove ((struct Node *)rtnode);
    FreeMem (rtnode, GetRTSize(rtt));

} /* RT_FreeResource */


/**************************************
	Utility functions
**************************************/

BOOL CheckPtr (APTR ptr, ULONG flags)
{
    if
    (
	(!ptr && !(flags & NULL_PTR))
	|| (((IPTR)ptr & 3) && (flags & ALIGNED_PTR))
    )
    {
	return FALSE;
    }


    return TRUE;
} /* CheckPtr */

BOOL CheckArea (APTR ptr, ULONG size, ULONG flags)
{
    if
    (
	(size & 0x8000000)
	|| !CheckPtr (ptr+size-1, flags)
    )
	return FALSE;

    return TRUE;
} /* CheckArea */

IPTR RT_Search (RTData * rtd, int rtt, RTNode ** rtptr, va_list args)
{
    Resource * rt;
    APTR     * res;

    res = va_arg (args, APTR);

    ForeachNode (&rtd->rtd_ResHash[rtt][CALCHASH(res)], rt)
    {
	if (rt->Resource == res)
	{
	    *rtptr = (RTNode *)rt;

	    return RT_SEARCH_FOUND;
	}
    }

    return RT_SEARCH_NOT_FOUND;
} /* RT_Search */
