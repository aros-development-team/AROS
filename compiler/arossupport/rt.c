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
#include <exec/ports.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/arossupport.h>
#include <proto/alib.h>
#include "etask.h"

#define RT_VERSION	1	/* Current version of RT */

#define HASH_BITS	4	/* Size of hash in bits and entries */
#define HASH_SIZE	(1L << HASH_BITS)

typedef struct
{
    const char * Function;
    const char * File;
    ULONG	 Line;
}
RTStack;

#define STACKDEPTH  256


/* This is what is behind etask->iet_RT */
typedef struct
{
    ULONG	   rtd_Version; /* RT_VERSION */
    struct MinList rtd_ResHash[RTT_MAX][HASH_SIZE];
    ULONG	   rtd_StackPtr;
    RTStack	   rtd_CallStack[STACKDEPTH];
} RTData;

static RTData * intRTD = NULL; /* Internal pointer in case no ETask is available */

typedef struct
{
    struct MinNode Node;
    const char	 * File;
    ULONG	   Line;
    ULONG	   Flags;
}
RTNode;

/* Private flags of resource nodes */
#define RTNF_DONT_FREE	    0x80000000 /* Resource must not be freed */

typedef struct
{
    RTNode Node;
    APTR   Resource;	/* This should be common to every resource */
}
Resource;

typedef struct
{
    RTNode Node;
    APTR   Memory;
    ULONG  Size;
    ULONG  Flags;
}
MemoryResource;

typedef struct
{
    RTNode	     Node;
    struct MsgPort * Port;
}
PortResource;

typedef struct
{
    RTNode	     Node;
    struct Library * Lib;
    STRPTR	     Name;
    ULONG	     Version;
}
LibraryResource;

typedef struct
{
    RTNode Node;
    BPTR   FH;
    STRPTR Path;
    ULONG  Mode;
}
FileResource;

typedef struct
{
    RTNode	    Node;
    struct Screen * Screen;
}
ScreenResource;

typedef struct
{
    RTNode	    Node;
    struct Window * Window;
}
WindowResource;

typedef struct __RTDesc RTDesc;

typedef IPTR (* RT_AllocFunc)  (RTData * rtd, RTNode *, va_list, BOOL * success);
typedef IPTR (* RT_FreeFunc)   (RTData * rtd, RTNode *);
typedef IPTR (* RT_SearchFunc) (RTData * rtd, int, RTNode **, va_list);
typedef IPTR (* RT_ShowError)  (RTData * rtd, int, RTNode *, IPTR, int, const char * file, ULONG line, va_list);
typedef IPTR (* RT_CheckFunc)  (RTData * rtd, int, const char * file, ULONG line, ULONG op, va_list);

#define HASH_BASE(rtn)  (((Resource *)rtn)->Resource)

#if HASH_BITS==4
#define CALCHASH(res)   \
    ((((ULONG)res) + (((ULONG)res)>>4) +(((ULONG)res)>>8) + (((ULONG)res)>>12) + \
     (((ULONG)res)>>16) + (((ULONG)res)>>20) +(((ULONG)res)>>24) + (((ULONG)res)>>28)) \
     & 0x0000000FL)
#endif

struct __RTDesc
{
    const ULONG    Size;
    RT_AllocFunc   AllocFunc;
    RT_FreeFunc    FreeFunc;
    RT_SearchFunc  SearchFunc;
    RT_ShowError   ShowError;
    RT_CheckFunc   CheckFunc;
};

#define GetRTData()                                                 \
	({                                                          \
	    struct IntETask * et;				    \
								    \
	    et = (struct IntETask *)GetETask (FindTask(NULL));      \
								    \
	    et							    \
		&& et->iet_RT					    \
		&& ((RTData *)et->iet_RT)->rtd_Version == RT_VERSION \
	    ? et->iet_RT					    \
	    : intRTD;						    \
	})
#define SetRTData(rtd)                                              \
	{							    \
	    struct IntETask * et;				    \
								    \
	    et = (struct IntETask *)GetETask (FindTask(NULL));      \
								    \
	    if (et)                                                 \
		et->iet_RT = rtd;				    \
	    else						    \
		intRTD = rtd;					    \
	}

static IPTR RT_Search (RTData * rtd, int rtt, RTNode **, va_list);

static IPTR RT_AllocMem (RTData * rtd, MemoryResource * rt, va_list args, BOOL * success);
static IPTR RT_FreeMem (RTData * rtd, MemoryResource * rt);
static IPTR RT_SearchMem (RTData * rtd, int rtt, MemoryResource ** rtptr, va_list args);
static IPTR RT_ShowErrorMem (RTData * rtd, int rtt, MemoryResource *, IPTR, int, const char * file, ULONG line, va_list);

static IPTR RT_AllocVec (RTData * rtd, MemoryResource * rt, va_list args, BOOL * success);
static IPTR RT_FreeVec (RTData * rtd, MemoryResource * rt);
static IPTR RT_SearchVec (RTData * rtd, int rtt, MemoryResource ** rtptr, va_list args);
static IPTR RT_ShowErrorVec (RTData * rtd, int, MemoryResource *, IPTR, int, const char * file, ULONG line, va_list);

static IPTR RT_CreatePort (RTData * rtd, PortResource * rt, va_list args, BOOL * success);
static IPTR RT_DeletePort (RTData * rtd, PortResource * rt);
static IPTR RT_ShowErrorPort (RTData * rtd, int, PortResource *, IPTR, int, const char * file, ULONG line, va_list);
static IPTR RT_CheckPort (RTData * rtd, int desc, const char * file, ULONG line, ULONG op, va_list args);

static IPTR RT_OpenLibrary (RTData * rtd, LibraryResource * rt, va_list args, BOOL * success);
static IPTR RT_CloseLibrary (RTData * rtd, LibraryResource * rt);
static IPTR RT_ShowErrorLib (RTData * rtd, int, LibraryResource *, IPTR, int, const char * file, ULONG line, va_list);

static IPTR RT_Open (RTData * rtd, FileResource * rt, va_list args, BOOL * success);
static IPTR RT_Close (RTData * rtd, FileResource * rt);
static IPTR RT_ShowErrorFile (RTData * rtd, int, FileResource *, IPTR, int, const char * file, ULONG line, va_list);
static IPTR RT_CheckFile (RTData * rtd, int desc, const char * file, ULONG line, ULONG op, va_list args);

static IPTR RT_OpenScreen (RTData * rtd, ScreenResource * rt, va_list args, BOOL * success);
static IPTR RT_CloseScreen (RTData * rtd, ScreenResource * rt);
static IPTR RT_ShowErrorScreen (RTData * rtd, int, ScreenResource *, IPTR, int, const char * file, ULONG line, va_list);
static IPTR RT_CheckScreen (RTData * rtd, int desc, const char * file, ULONG line, ULONG op, va_list args);

static IPTR RT_OpenWindow (RTData * rtd, WindowResource * rt, va_list args, BOOL * success);
static IPTR RT_CloseWindow (RTData * rtd, WindowResource * rt);
static IPTR RT_ShowErrorWindow (RTData * rtd, int, WindowResource *, IPTR, int, const char * file, ULONG line, va_list);
static IPTR RT_CheckWindow (RTData * rtd, int desc, const char * file, ULONG line, ULONG op, va_list args);

/* Return values of SearchFunc */
#define RT_SEARCH_FOUND 	    0
#define RT_SEARCH_NOT_FOUND	    1
#define RT_SEARCH_SIZE_MISMATCH     2

#define RT_FREE     0
#define RT_CHECK    1
#define RT_EXIT     2

static const RTDesc RT_Resources[RTT_MAX] =
{
    { /* RTT_ALLOCMEM */
	sizeof (MemoryResource),
	(RT_AllocFunc) RT_AllocMem,
	(RT_FreeFunc)  RT_FreeMem,
	(RT_SearchFunc)RT_SearchMem,
	(RT_ShowError) RT_ShowErrorMem,
	NULL, /* Check */
    },
    { /* RTT_ALLOCVEC */
	sizeof (MemoryResource),
	(RT_AllocFunc) RT_AllocVec,
	(RT_FreeFunc)  RT_FreeVec,
	(RT_SearchFunc)RT_SearchVec,
	(RT_ShowError) RT_ShowErrorVec,
	NULL, /* Check */
    },
    { /* RTT_PORT */
	sizeof (PortResource),
	(RT_AllocFunc) RT_CreatePort,
	(RT_FreeFunc)  RT_DeletePort,
	RT_Search,
	(RT_ShowError) RT_ShowErrorPort,
	(RT_CheckFunc) RT_CheckPort,
    },
    { /* RTT_LIBRARY */
	sizeof (LibraryResource),
	(RT_AllocFunc) RT_OpenLibrary,
	(RT_FreeFunc)  RT_CloseLibrary,
	RT_Search,
	(RT_ShowError) RT_ShowErrorLib,
	NULL, /* Check */
    },
    { /* RTT_FILE */
	sizeof (FileResource),
	(RT_AllocFunc) RT_Open,
	(RT_FreeFunc)  RT_Close,
	RT_Search,
	(RT_ShowError) RT_ShowErrorFile,
	(RT_CheckFunc) RT_CheckFile,
    },
    { /* RTT_SCREEN */
	sizeof (ScreenResource),
	(RT_AllocFunc) RT_OpenScreen,
	(RT_FreeFunc)  RT_CloseScreen,
	RT_Search,
	(RT_ShowError) RT_ShowErrorScreen,
	(RT_CheckFunc) RT_CheckScreen,
    },
    { /* RTT_WINDOW */
	sizeof (WindowResource),
	(RT_AllocFunc) RT_OpenWindow,
	(RT_FreeFunc)  RT_CloseWindow,
	RT_Search,
	(RT_ShowError) RT_ShowErrorWindow,
	(RT_CheckFunc) RT_CheckWindow,
    },
};


static void RT_FreeResource (RTData * rtd, int rtt, RTNode * rtnode);

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
    RTData * rtd;
    int      t, i;

    if ((rtd = GetRTData ()))
    {
	kprintf ("RT_Init() called twice %p\n", rtd);
	return;
    }

    if (!(rtd = AllocMem (sizeof (RTData), MEMF_ANY)) )
    {
	kprintf ("RT_Init(): No memory\n");
	return;
    }

    SetRTData (rtd);

    rtd->rtd_Version  = RT_VERSION;
    rtd->rtd_StackPtr = STACKDEPTH;

    for (t=0; t<RTT_MAX; t++)
    {
	for (i=0; i<HASH_SIZE; i++)
	{
	    NEWLIST(&rtd->rtd_ResHash[t][i]);
	}
    }

    kprintf ("RT_Init(): RT up and kicking in %s mode\n"
	, intRTD ? "internal" : "ETask"
    );
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
	    for (next=GetHead(&rtd->rtd_ResHash[t][i]); (rt=next); )
	    {
		next = GetSucc (rt);

		RT_FreeResource (rtd, t, rt);
	    }
	}
    }

    FreeMem (rtd, sizeof (RTData));
} /* RT_Exit */

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
    RTData * rtd;
    IPTR     ret;
    va_list  args;
    RTNode * rtnew;
    BOOL     success;

    if (!(rtd = GetRTData ()) )
	return FALSE;

    if (!(rtnew = AllocMem (RT_Resources[rtt].Size, MEMF_ANY)) )
    {
	kprintf ("RT_IntAdd: Out of memory\n");
	return FALSE;
    }

    rtnew->File = file;
    rtnew->Line = line;

    va_start (args, line);

    success = FALSE;

    ret = (*(RT_Resources[rtt].AllocFunc))
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
	FreeMem (rtnew, RT_Resources[rtt].Size);
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
    RTData   * rtd;
    va_list    args;
    Resource * rtnew;

    if (!(rtd = GetRTData ()) )
	return;

    if (!(rtnew = AllocMem (RT_Resources[rtt].Size, MEMF_ANY|MEMF_CLEAR)) )
    {
	kprintf ("RT_IntAdd: Out of memory\n");
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

    if (RT_Resources[rtt].CheckFunc)
    {
	ret = (*(RT_Resources[rtt].CheckFunc))
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
	ret = (*(RT_Resources[rtt].SearchFunc))
	(
	    rtd,
	    rtt,
	    &rt,
	    args
	);

	if (ret != RT_SEARCH_FOUND)
	{
	    ret = (*(RT_Resources[rtt].ShowError))
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
	line depend on the type of resource to be traced:

	RTT_ALLOCMEM:	  APTR		memPtr,
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
    RTData * rtd;
    IPTR     ret;
    va_list  args;
    RTNode * rt;

    if (!(rtd = GetRTData ()) )
	return FALSE;

    va_start (args, line);

    ret = (*(RT_Resources[rtt].SearchFunc))
    (
	rtd,
	rtt,
	&rt,
	args
    );

    if (ret == RT_SEARCH_FOUND && !(rt->Flags & RTNF_DONT_FREE))
    {
	ret = (*(RT_Resources[rtt].FreeFunc)) (rtd, rt);

	Remove ((struct Node *)rt);
	FreeMem (rt, RT_Resources[rtt].Size);
    }
    else
    {
	ret = (*(RT_Resources[rtt].ShowError))
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
    RTData * rtd;
    int t;

    if (!(rtd = GetRTData ()) )
	return;

    for (t=0; t<=rtd->rtd_StackPtr; t++)
    {
	kprintf ("    %s (%s:%d)\n"
	    , rtd->rtd_CallStack[t].Function
	    , rtd->rtd_CallStack[t].File
	    , rtd->rtd_CallStack[t].Line
	);
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
    if (!rtd && !(rtd = GetRTData ()))
	return;

    if (!(rtnode->Flags & RTNF_DONT_FREE) )
    {
	/* Print an error */
	(void) (*(RT_Resources[rtt].ShowError))
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
	(void) (*(RT_Resources[rtt].FreeFunc)) (rtd, rtnode);
    }

    /* Remove resource from list and free it */
    Remove ((struct Node *)rtnode);
    FreeMem (rtnode, RT_Resources[rtt].Size);

} /* RT_FreeResource */


/**************************************
	Utility functions
**************************************/

static char * StrDup (const char * str)
{
    char * copy;

    if ((copy = AllocVec (strlen (str)+1, MEMF_ANY)))
	strcpy (copy, str);

    return copy;
}

#define ALIGNED_PTR	0x00000001	/* Must be aligned */
#define NULL_PTR	0x00000002	/* May be NULL */

static BOOL CheckPtr (APTR ptr, ULONG flags)
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

static BOOL CheckArea (APTR ptr, ULONG size, ULONG flags)
{
    if
    (
	(size & 0x8000000)
	|| !CheckPtr (ptr+size-1, flags)
    )
	return FALSE;

    return TRUE;
} /* CheckArea */

static IPTR RT_Search (RTData * rtd, int rtt, RTNode ** rtptr, va_list args)
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


/**************************************
	   RT Memory
**************************************/

static IPTR RT_AllocMem (RTData * rtd, MemoryResource * rt, va_list args, BOOL * success)
{
    rt->Size = va_arg (args, ULONG);
    rt->Flags = va_arg (args, ULONG);

    rt->Memory = AllocMem (rt->Size, rt->Flags);

    if (rt->Memory)
	*success = TRUE;

    return (IPTR)(rt->Memory);
} /* RT_AllocMem */

static IPTR RT_FreeMem (RTData * rtd, MemoryResource * rt)
{
    FreeMem (rt->Memory, rt->Size);

    return TRUE;
} /* RT_FreeMem */

static IPTR RT_SearchMem (RTData * rtd, int rtt, MemoryResource ** rtptr,
	va_list args)
{
    MemoryResource * rt;
    APTR    memory;
    ULONG   size;

    memory = va_arg (args, APTR);
    size   = va_arg (args, ULONG);

    ForeachNode (&rtd->rtd_ResHash[rtt][CALCHASH(memory)], rt)
    {
	if (rt->Memory == memory)
	{
	    *rtptr = rt;

	    if (rt->Size != size)
		return RT_SEARCH_SIZE_MISMATCH;

	    return RT_SEARCH_FOUND;
	}
    }

    return RT_SEARCH_NOT_FOUND;
} /* RT_SearchMem */

static IPTR RT_ShowErrorMem (RTData * rtd, int rtt, MemoryResource * rt,
	IPTR ret, int mode, const char * file, ULONG line, va_list args)
{
    const char * modestr = (mode == RT_FREE) ? "Free" : "Check";
    APTR	 memory;
    ULONG	 size;

    if (mode != RT_EXIT)
    {
	memory = va_arg (args, APTR);
	size   = va_arg (args, ULONG);

	switch (ret)
	{
	case RT_SEARCH_FOUND:
	    if (rt->Node.Flags & RTNF_DONT_FREE)
	    {
		kprintf ("RT%s: Try to free read-only resource: Memory\n"
			"    %s at %s:%d\n"
			"    Added at %s:%d\n"
			"    MemPtr=%p\n"
		    , modestr
		    , modestr
		    , file, line
		    , rt->Node.File, rt->Node.Line
		    , rt->Memory
		);
	    }
	    break;

	case RT_SEARCH_NOT_FOUND:
	    kprintf ("RT%s: Memory not found\n"
		    "    %s at %s:%d\n"
		    "    MemPtr=%p Size=%ld\n"
		, modestr
		, modestr
		, file, line
		, memory, size
	    );
	    break;

	case RT_SEARCH_SIZE_MISMATCH:
	    kprintf ("RT%s: Size mismatch (Allocated=%ld, Check=%ld)\n"
		    "    %s at %s:%d\n"
		    "    AllocMem()'d at %s:%d\n"
		    "    MemPtr=%p Size=%ld Flags=%08lx\n"
		, modestr
		, rt->Size, size
		, modestr
		, file, line
		, rt->Node.File, rt->Node.Line
		, rt->Memory, rt->Size, rt->Flags
	    );
	    break;

	} /* switch */
    }
    else
    {
	kprintf ("RTExit: Memory was not freed\n"
		"    AllocMem()'d at %s:%d\n"
		"    MemPtr=%p Size=%ld Flags=%08lx\n"
	    , rt->Node.File, rt->Node.Line
	    , rt->Memory, rt->Size, rt->Flags
	);
    }

    return ret;
} /* RT_ShowErrorMem */

static IPTR RT_AllocVec (RTData * rtd, MemoryResource * rt, va_list args, BOOL * success)
{
    rt->Size = va_arg (args, ULONG);
    rt->Flags = va_arg (args, ULONG);

    rt->Memory = AllocVec (rt->Size, rt->Flags);

    if (rt->Memory)
	*success = TRUE;

    return (IPTR)(rt->Memory);
} /* RT_AllocVec */

static IPTR RT_FreeVec (RTData * rtd, MemoryResource * rt)
{
    if (rt)
	FreeVec (rt->Memory);

    return TRUE;
} /* RT_FreeVec */

static IPTR RT_SearchVec (RTData * rtd, int rtt, MemoryResource ** rtptr,
	va_list args)
{
    MemoryResource * rt;
    APTR    memory;

    memory = va_arg (args, APTR);

    if (!memory)
    {
	*rtptr = NULL;
	return RT_SEARCH_FOUND;
    }

    ForeachNode (&rtd->rtd_ResHash[rtt][CALCHASH(memory)], rt)
    {
	if (rt->Memory == memory)
	{
	    *rtptr = rt;

	    return RT_SEARCH_FOUND;
	}
    }

    return RT_SEARCH_NOT_FOUND;
}

static IPTR RT_ShowErrorVec (RTData * rtd, int rtt, MemoryResource * rt,
	IPTR ret, int mode, const char * file, ULONG line, va_list args)
{
    const char * modestr = (mode == RT_FREE) ? "Free" : "Check";
    APTR	 memory;
    ULONG	 size;

    if (mode != RT_EXIT)
    {
	memory = va_arg (args, APTR);
	size   = va_arg (args, ULONG);

	switch (ret)
	{
	case RT_SEARCH_FOUND:
	    if (rt->Node.Flags & RTNF_DONT_FREE)
	    {
		kprintf ("RT%s: Try to free read-only resource: Vec-Memory\n"
			"    %s at %s:%d\n"
			"    Added at %s:%d\n"
			"    MemPtr=%p\n"
		    , modestr
		    , modestr
		    , file, line
		    , rt->Node.File, rt->Node.Line
		    , rt->Memory
		);
	    }
	    break;


	case RT_SEARCH_NOT_FOUND:
	    kprintf ("RT%s: Memory not found\n"
		    "    %s at %s:%d\n"
		    "    MemPtr=%p Size=%ld\n"
		, modestr
		, modestr
		, file, line
		, memory, size
	    );
	    break;

	} /* switch */
    }
    else
    {
	kprintf ("RTExit: Memory was not freed\n"
		"    AllocVec()'d at %s:%d\n"
		"    MemPtr=%p Size=%ld Flags=%08lx\n"
	    , rt->Node.File, rt->Node.Line
	    , rt->Memory, rt->Size, rt->Flags
	);
    }

    return ret;
} /* RT_ShowErrorVec */


/**************************************
	       RT Ports
**************************************/

static IPTR RT_CreatePort (RTData * rtd, PortResource * rt, va_list args, BOOL * success)
{
    STRPTR name;
    LONG   pri;

    name = va_arg (args, STRPTR);
    pri  = va_arg (args, LONG);

    if (!CheckPtr (name, NULL_PTR))
    {
	kprintf ("CreatePort(): Illegal name pointer\n"
		"    name=%p at %s:%d\n"
	    , name
	    , rt->Node.File, rt->Node.Line
	);
	return 0ul;
    }

    rt->Port = CreatePort (name, pri);

    if (rt->Port)
	*success = TRUE;

    return (IPTR)(rt->Port);
} /* RT_CreatePort */

static IPTR RT_DeletePort (RTData * rtd, PortResource * rt)
{
    DeletePort (rt->Port);

    return TRUE;
} /* RT_ClosePort */

static IPTR RT_ShowErrorPort (RTData * rtd, int rtt, PortResource * rt,
	IPTR ret, int mode, const char * file, ULONG line, va_list args)
{
    if (mode != RT_EXIT)
    {
	const char     * modestr = (mode == RT_FREE) ? "Close" : "Check";
	struct MsgPort * port;

	port = va_arg (args, struct MsgPort *);

	switch (ret)
	{
	case RT_SEARCH_FOUND:
	    if (rt->Node.Flags & RTNF_DONT_FREE)
	    {
		kprintf ("RT%s: Try to free read-only resource: MsgPort\n"
			"    %s at %s:%d\n"
			"    Added at %s:%d\n"
			"    Port=%p (Name=%s Pri=%d)\n"
		    , modestr
		    , modestr
		    , file, line
		    , rt->Node.File, rt->Node.Line
		    , rt->Port
		    , rt->Port->mp_Node.ln_Name
			? rt->Port->mp_Node.ln_Name
			: NULL
		    , rt->Port->mp_Node.ln_Pri
		);
	    }
	    break;

	case RT_SEARCH_NOT_FOUND:
	    kprintf ("RT%s: Port not found\n"
		    "    %s at %s:%d\n"
		    "    Port=%p\n"
		, modestr
		, modestr
		, file, line
		, port
	    );
	    break;

	} /* switch */
    }
    else
    {
	kprintf ("RTExit: Port was not closed\n"
		"    Opened at %s:%d\n"
		"    Port=%p (Name=%s Pri=%d)\n"
	    , rt->Node.File, rt->Node.Line
	    , rt->Port
	    , rt->Port->mp_Node.ln_Name
		? rt->Port->mp_Node.ln_Name
		: NULL
	    , rt->Port->mp_Node.ln_Pri
	);
    }

    return ret;
} /* RT_ShowErrorPort */

static IPTR RT_CheckPort (RTData * rtd, int rtt,
			const char * file, ULONG line,
			ULONG op, va_list args)
{
    PortResource * rt;

    if (RT_Search (rtd, rtt, (RTNode **)&rt, args) != RT_SEARCH_FOUND)
	rt = NULL;

    switch (op)
    {
    case RTTO_PutMsg:
	{
	    struct MsgPort * port;
	    struct Message * message;

	    port    = va_arg (args, struct MsgPort *);
	    message = va_arg (args, struct Message *);

	    if (!rt)
	    {
		kprintf ("PutMsg(): Illegal port pointer\n"
			"    Port=%p Message=%p at %s:%d\n"
		    , port, message
		    , file, line
		);

		return -1;
	    }
	    else if (CheckPtr (message, 0))
	    {
		kprintf ("PutMsg(): Illegal message pointer\n"
			"    Port=%p Message=%p at %s:%d\n"
		    , port, message
		    , file, line
		);

		return -1;
	    }

	    PutMsg (port, message);

	    return 0;
	}

    case RTTO_GetMsg:
	{
	    struct MsgPort * port;

	    port = va_arg (args, struct MsgPort *);

	    if (!rt)
	    {
		kprintf ("GetMsg(): Illegal port pointer\n"
			"    Port=%p at %s:%d\n"
		    , port
		    , file, line
		);

		return 0L;
	    }

	    return (IPTR) GetMsg (port);
	}

    } /* switch (op) */

    return 0L;
} /* RT_CheckPort */


/**************************************
	    RT Libraries
**************************************/

static IPTR RT_OpenLibrary (RTData * rtd, LibraryResource * rt, va_list args, BOOL * success)
{
    rt->Name	= va_arg (args, STRPTR);
    rt->Version = va_arg (args, ULONG);

    rt->Lib = OpenLibrary (rt->Name, rt->Version);

    if (rt->Lib)
	*success = TRUE;

    return (IPTR)(rt->Lib);
} /* RT_OpenLibrary */

static IPTR RT_CloseLibrary (RTData * rtd, LibraryResource * rt)
{
    CloseLibrary (rt->Lib);

    return TRUE;
} /* RT_CloseLibrary */

static IPTR RT_ShowErrorLib (RTData * rtd, int rtt, LibraryResource * rt,
	IPTR ret, int mode, const char * file, ULONG line, va_list args)
{

    if (mode != RT_EXIT)
    {
	const char     * modestr = (mode == RT_FREE) ? "Free" : "Check";
	struct Library * base;

	base = va_arg (args, struct Library *);

	switch (ret)
	{
	case RT_SEARCH_FOUND:
	    if (rt->Node.Flags & RTNF_DONT_FREE)
	    {
		kprintf ("RT%s: Try to free read-only resource: Library\n"
			"    %s at %s:%d\n"
			"    Added at %s:%d\n"
			"    LibBase=%p\n"
		    , modestr
		    , modestr
		    , file, line
		    , rt->Node.File, rt->Node.Line
		    , rt->Lib
		);
	    }
	    break;

	case RT_SEARCH_NOT_FOUND:
	    kprintf ("RT%s: Library not found\n"
		    "    %s at %s:%d\n"
		    "    Base=%p\n"
		, modestr
		, modestr
		, file, line
		, base
	    );
	    break;

	} /* switch */
    }
    else
    {
	kprintf ("RTExit: Library was not closed\n"
		"    Opened at %s:%d\n"
		"    Base=%p Name=%s Version=%ld\n"
	    , rt->Node.File, rt->Node.Line
	    , rt->Lib, rt->Name, rt->Version
	);
    }

    return ret;
} /* RT_ShowErrorLib */


/**************************************
	      RT Files
**************************************/

static IPTR RT_Open (RTData * rtd, FileResource * rt, va_list args, BOOL * success)
{
    STRPTR path;

    path = va_arg (args, STRPTR);

    if (!CheckPtr (path, 0))
    {
	kprintf ("Open(): Illegal path\n"
		"    path=%p at %s:%d\n"
	    , path
	    , rt->Node.File, rt->Node.Line
	);
	return 0ul;
    }

    rt->Mode = va_arg (args, LONG);

    rt->FH = NULL;

    if
    (
	rt->Mode != MODE_OLDFILE
	&& rt->Mode != MODE_NEWFILE
	&& rt->Mode != MODE_READWRITE
    )
    {
	kprintf ("Open(): Illegal mode %d at %s:%d\n"
	    , rt->Mode
	    , rt->Node.File, rt->Node.Line
	);
    }
    else
    {
	rt->Path = StrDup (path);

	if (!rt->Path)
	{
	    kprintf ("Open(): RT: Out of memory\n");
	}
	else
	{
	    rt->FH = Open (rt->Path, rt->Mode);

	    if (!rt->FH)
		FreeVec (rt->Path);
	}
    }

    if (rt->FH)
	*success = TRUE;

    return (IPTR)(rt->FH);
} /* RT_Open */

static IPTR RT_Close (RTData * rtd, FileResource * rt)
{
    Close (rt->FH);
    FreeVec (rt->Path);

    return TRUE;
} /* RT_Close */

static const STRPTR GetFileMode (LONG mode)
{
    static char buffer[64];

    switch (mode)
    {
    case MODE_OLDFILE: return "MODE_OLDFILE";
    case MODE_NEWFILE: return "MODE_NEWFILE";
    case MODE_READWRITE: return "MODE_READWRITE";
    }

    sprintf (buffer, "<illegal mode %ld>", mode);

    return buffer;
} /* GetFileMode */

static IPTR RT_ShowErrorFile (RTData * rtd, int rtt, FileResource * rt,
	IPTR ret, int mode, const char * file, ULONG line, va_list args)
{
    if (mode != RT_EXIT)
    {
	const char * modestr = (mode == RT_FREE) ? "Close" : "Check";
	BPTR	     fh;

	fh = va_arg (args, BPTR);

	switch (ret)
	{
	case RT_SEARCH_FOUND:
	    if (rt->Node.Flags & RTNF_DONT_FREE)
	    {
		kprintf ("RT%s: Try to free read-only resource: File\n"
			"    %s at %s:%d\n"
			"    Added at %s:%d\n"
			"    FH=%p\n"
		    , modestr
		    , modestr
		    , file, line
		    , rt->Node.File, rt->Node.Line
		    , rt->FH
		);
	    }
	    break;

	case RT_SEARCH_NOT_FOUND:
	    kprintf ("RT%s: File not found\n"
		    "    %s at %s:%d\n"
		    "    FH=%p\n"
		, modestr
		, modestr
		, file, line
		, fh
	    );
	    break;

	} /* switch */
    }
    else
    {
	kprintf ("RTExit: File was not closed\n"
		"    Opened at %s:%d\n"
		"    FH=%p Path=%s Mode=%s\n"
	    , rt->Node.File, rt->Node.Line
	    , rt->FH, rt->Path, GetFileMode (rt->Mode)
	);
    }

    return ret;
} /* RT_ShowErrorFile */

static IPTR RT_CheckFile (RTData * rtd, int rtt,
			const char * file, ULONG line,
			ULONG op, va_list args)
{
    FileResource * rt;

    if (RT_Search (rtd, rtt, (RTNode **)&rt, args) != RT_SEARCH_FOUND)
	rt = NULL;

    switch (op)
    {
    case RTTO_Read:
	{
	    BPTR  fh;
	    APTR  buffer;
	    ULONG length;

	    fh	   = va_arg (args, BPTR);
	    buffer = va_arg (args, APTR);
	    length = va_arg (args, ULONG);

	    if (!rt)
	    {
		kprintf ("Read(): Illegal filehandle\n"
			"    fh=%p at %s:%d\n"
		    , fh
		    , file, line
		);

		return -1;
	    }
	    else if (!CheckPtr (buffer, 0))
	    {
		kprintf ("Read(): Illegal buffer\n"
			"    buffer=%p at %s:%d\n"
			"    FH=%p Path=%s Mode=%s\n"
			"    opened at %s:%d\n"
		    , buffer
		    , file, line
		    , rt->FH, rt->Path, GetFileMode (rt->Mode)
		    , rt->Node.File, rt->Node.Line
		);

		return -1;
	    }
	    else if (!CheckArea (buffer, length, 0))
	    {
		kprintf ("Read(): Illegal buffer\n"
			"    buffer=%p, size=%d at %s:%d\n"
			"    FH=%p Path=%s Mode=%s\n"
			"    opened at %s:%d\n"
		    , buffer, length
		    , file, line
		    , rt->FH, rt->Path, GetFileMode (rt->Mode)
		    , rt->Node.File, rt->Node.Line
		);

		return -1;
	    }

	    return Read (fh, buffer, length);
	}

    case RTTO_Write:
	{
	    BPTR  fh;
	    APTR  buffer;
	    ULONG length;

	    fh	   = va_arg (args, BPTR);
	    buffer = va_arg (args, APTR);
	    length = va_arg (args, ULONG);

	    if (!rt)
	    {
		kprintf ("Write(): Illegal filehandle\n"
			"    fh=%p at %s:%d\n"
		    , fh
		    , file, line
		);

		return -1;
	    }
	    else if (!CheckPtr (buffer, 0))
	    {
		kprintf ("Write(): Illegal buffer\n"
			"    buffer=%p at %s:%d\n"
			"    FH=%p Path=%s Mode=%s\n"
			"    opened at %s:%d\n"
		    , buffer
		    , file, line
		    , rt->FH, rt->Path, GetFileMode (rt->Mode)
		    , rt->Node.File, rt->Node.Line
		);

		return -1;
	    }
	    else if (!CheckArea (buffer, length, 0))
	    {
		kprintf ("Write(): Illegal buffer\n"
			"    buffer=%p, size=%d at %s:%d\n"
			"    FH=%p Path=%s Mode=%s\n"
			"    opened at %s:%d\n"
		    , buffer, length
		    , file, line
		    , rt->FH, rt->Path, GetFileMode (rt->Mode)
		    , rt->Node.File, rt->Node.Line
		);

		return -1;
	    }

	    return Write (fh, buffer, length);
	}

    }

    return 0L;
} /* RT_CheckFile */


/**************************************
	    RT Screens
**************************************/

static IPTR RT_OpenScreen (RTData * rtd, ScreenResource * rt, va_list args, BOOL * success)
{
    struct NewScreen * ns;
    struct TagItem   * tags = NULL;
    int op;

    op = va_arg (args, int);
    ns = va_arg (args, struct NewScreen *);

    switch (op)
    {
    case RTTO_OpenScreenTags:
	tags = (struct TagItem *)args;
	break;

    case RTTO_OpenScreenTagList:
	tags = va_arg (args, struct TagItem *);
	break;

    }

    if (!CheckPtr (ns, NULL_PTR))
    {
	kprintf ("OpenScreen(): Illegal NewScreen pointer\n"
		"    NewScreen=%p at %s:%d\n"
	    , ns
	    , rt->Node.File, rt->Node.Line
	);
	return 0ul;
    }
    else if (!CheckPtr (tags, NULL_PTR))
    {
	kprintf ("OpenScreenTagList(): Illegal TagItem pointer\n"
		"    tagList=%p at %s:%d\n"
	    , tags
	    , rt->Node.File, rt->Node.Line
	);
	return 0ul;
    }

    rt->Screen = OpenScreenTagList (ns, tags);

    if (rt->Screen)
	*success = TRUE;

    return (IPTR)(rt->Screen);
} /* RT_OpenScreen */

static IPTR RT_CloseScreen (RTData * rtd, ScreenResource * rt)
{
    if (rt->Screen->FirstWindow)
    {
	struct Window  * win;
	WindowResource * rtwin;

	kprintf ("CloseScreen(): There are still windows open on this screen\n"
		"    Screen=%p opened at %s:%d\n"
		, rt->Screen
		, rt->Node.File, rt->Node.Line
	);

	while ((win = rt->Screen->FirstWindow))
	{
	    if (RT_Search (rtd, RTT_WINDOW, (RTNode **)&rtwin, NULL) == RT_SEARCH_FOUND)
	    {
		RT_FreeResource (rtd, RTT_WINDOW, (RTNode *)rtwin);
	    }
	    else
	    {
		kprintf ("  Window=%p not tracked by the RT system\n"
		    , win
		);
		CloseWindow (win);
	    }
	}
    } /* Check for windows */

    /* Close the screen */
    CloseScreen (rt->Screen);

    return TRUE;
} /* RT_CloseScreen */

static IPTR RT_ShowErrorScreen (RTData * rtd, int rtt, ScreenResource * rt,
	IPTR ret, int mode, const char * file, ULONG line, va_list args)
{
    if (mode != RT_EXIT)
    {
	const char    * modestr = (mode == RT_FREE) ? "Close" : "Check";
	struct Screen * scr;

	scr = va_arg (args, struct Screen *);

	switch (ret)
	{
	case RT_SEARCH_FOUND:
	    if (rt->Node.Flags & RTNF_DONT_FREE)
	    {
		kprintf ("RT%s: Try to free read-only resource: Screen\n"
			"    %s at %s:%d\n"
			"    Added at %s:%d\n"
			"    Screen=%p\n"
		    , modestr
		    , modestr
		    , file, line
		    , rt->Node.File, rt->Node.Line
		    , rt->Screen
		);
	    }
	    break;

	case RT_SEARCH_NOT_FOUND:
	    kprintf ("RT%s: Screen not found\n"
		    "    %s at %s:%d\n"
		    "    Screen=%p\n"
		, modestr
		, modestr
		, file, line
		, scr
	    );
	    break;

	} /* switch */
    }
    else
    {
	kprintf ("RTExit: Screen was not closed\n"
		"    Opened at %s:%d\n"
		"    Screen=%p\n"
	    , rt->Node.File, rt->Node.Line
	    , rt->Screen
	);
    }

    return ret;
} /* RT_ShowErrorScreen */

static IPTR RT_CheckScreen (RTData * rtd, int rtt,
			const char * file, ULONG line,
			ULONG op, va_list args)
{
    ScreenResource * rt;

    if (RT_Search (rtd, rtt, (RTNode **)&rt, args) != RT_SEARCH_FOUND)
	rt = NULL;

    switch (op)
    {
    case RTTO_ScreenToFront:
	{
	    struct Screen * scr = va_arg (args, struct Screen *);

	    if (!rt)
	    {
		kprintf ("ScreenToFont(): Illegal window pointer\n"
			"    Screen=%p at %s:%d\n"
		    , scr
		    , file, line
		);

		return -1;
	    }

	    ScreenToFront (scr);

	    return 0;
	}

    case RTTO_ScreenToBack:
	{
	    struct Screen * scr = va_arg (args, struct Screen *);

	    if (!rt)
	    {
		kprintf ("ScreenToBack(): Illegal window pointer\n"
			"    Screen=%p at %s:%d\n"
		    , scr
		    , file, line
		);

		return -1;
	    }

	    ScreenToBack (scr);

	    return 0;
	}

    }

    return 0L;
} /* RT_CheckScreen */


/**************************************
	    RT Windows
**************************************/

static IPTR RT_OpenWindow (RTData * rtd, WindowResource * rt, va_list args, BOOL * success)
{
    struct NewWindow * nw;
    struct TagItem   * tags = NULL;
    int op;

    op = va_arg (args, int);
    nw = va_arg (args, struct NewWindow *);

    switch (op)
    {
    case RTTO_OpenWindowTags:
	tags = (struct TagItem *)args;
	break;

    case RTTO_OpenWindowTagList:
	tags = va_arg (args, struct TagItem *);
	break;

    }

    if (!CheckPtr (nw, NULL_PTR))
    {
	kprintf ("OpenWindow(): Illegal NewWindow pointer\n"
		"    NewWindow=%p at %s:%d\n"
	    , nw
	    , rt->Node.File, rt->Node.Line
	);
	return 0ul;
    }
    else if (!CheckPtr (tags, NULL_PTR))
    {
	kprintf ("OpenWindowTagList(): Illegal TagList pointer\n"
		"    tagList=%p at %s:%d\n"
	    , nw
	    , rt->Node.File, rt->Node.Line
	);
	return 0ul;
    }

    rt->Window = OpenWindowTagList (nw, tags);

    if (rt->Window->UserPort)
	RT_IntTrack (RTT_PORT, __FILE__, __LINE__, rt->Window->UserPort);

    if (rt->Window)
	*success = TRUE;

    return (IPTR)(rt->Window);
} /* RT_OpenWindow */

static IPTR RT_CloseWindow (RTData * rtd, WindowResource * rt)
{
    CloseWindow (rt->Window);

    return TRUE;
} /* RT_CloseWindow */

static IPTR RT_ShowErrorWindow (RTData * rtd, int rtt, WindowResource * rt,
	IPTR ret, int mode, const char * file, ULONG line, va_list args)
{
    if (mode != RT_EXIT)
    {
	const char    * modestr = (mode == RT_FREE) ? "Close" : "Check";
	struct Window * win;

	win = va_arg (args, struct Window *);

	switch (ret)
	{
	case RT_SEARCH_FOUND:
	    if (rt->Node.Flags & RTNF_DONT_FREE)
	    {
		kprintf ("RT%s: Try to free read-only resource: Window\n"
			"    %s at %s:%d\n"
			"    Added at %s:%d\n"
			"    Window=%p\n"
		    , modestr
		    , modestr
		    , file, line
		    , rt->Node.File, rt->Node.Line
		    , rt->Window
		);
	    }
	    break;

	case RT_SEARCH_NOT_FOUND:
	    kprintf ("RT%s: Window not found\n"
		    "    %s at %s:%d\n"
		    "    Window=%p\n"
		, modestr
		, modestr
		, file, line
		, win
	    );
	    break;

	} /* switch */
    }
    else
    {
	kprintf ("RTExit: Window was not closed\n"
		"    Opened at %s:%d\n"
		"    Window=%p\n"
	    , rt->Node.File, rt->Node.Line
	    , rt->Window
	);
    }

    return ret;
} /* RT_ShowErrorWindow */

static IPTR RT_CheckWindow (RTData * rtd, int rtt,
			const char * file, ULONG line,
			ULONG op, va_list args)
{
    WindowResource * rt;

    if (RT_Search (rtd, rtt, (RTNode **)&rt, args) != RT_SEARCH_FOUND)
	rt = NULL;

    switch (op)
    {
    case RTTO_WindowToFront:
	{
	    struct Window * win = va_arg (args, struct Window *);

	    if (!rt)
	    {
		kprintf ("WindowToFont(): Illegal window pointer\n"
			"    Window=%p at %s:%d\n"
		    , win
		    , file, line
		);

		return -1;
	    }

	    WindowToFront (win);

	    return 0;
	}

    case RTTO_WindowToBack:
	{
	    struct Window * win = va_arg (args, struct Window *);

	    if (!rt)
	    {
		kprintf ("WindowToBack(): Illegal window pointer\n"
			"    Window=%p at %s:%d\n"
		    , win
		    , file, line
		);

		return -1;
	    }

	    WindowToBack (win);

	    return 0;
	}

    } /* switch (op) */

    return 0L;
} /* RT_CheckWindow */


