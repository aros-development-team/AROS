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
#include <stdio.h>
#include <proto/exec.h>
#include <proto/dos.h>
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
    APTR   Resource;
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

typedef struct __RTDesc RTDesc;

typedef IPTR (* RT_AllocFunc) (RTNode *, va_list, BOOL * success);
typedef IPTR (* RT_FreeFunc) (RTNode *);
typedef IPTR (* RT_SearchFunc) (RTDesc *, RTNode **, va_list);
typedef IPTR (* RT_ShowError) (RTDesc *, RTNode *, IPTR, int, const char * file, ULONG line, va_list);
typedef IPTR (* RT_CheckFunc) (RTDesc *, const char * file, ULONG line, ULONG op, va_list);

struct __RTDesc
{
    const ULONG    Size;
    RT_AllocFunc   AllocFunc;
    RT_FreeFunc    FreeFunc;
    RT_SearchFunc  SearchFunc;
    RT_ShowError   ShowError;
    RT_CheckFunc   CheckFunc;
    struct MinList ResList;
};

static IPTR RT_Search (RTDesc *, RTNode **, va_list);

static IPTR RT_AllocMem (MemoryResource * rt, va_list args, BOOL * success);
static IPTR RT_FreeMem (MemoryResource * rt);
static IPTR RT_SearchMem (RTDesc * desc, MemoryResource ** rtptr, va_list args);
static IPTR RT_ShowErrorMem (RTDesc *, MemoryResource *, IPTR, int, const char * file, ULONG line, va_list);

static IPTR RT_AllocVec (MemoryResource * rt, va_list args, BOOL * success);
static IPTR RT_FreeVec (MemoryResource * rt);
static IPTR RT_ShowErrorVec (RTDesc *, MemoryResource *, IPTR, int, const char * file, ULONG line, va_list);

static IPTR RT_OpenLibrary (LibraryResource * rt, va_list args, BOOL * success);
static IPTR RT_CloseLibrary (LibraryResource * rt);
static IPTR RT_ShowErrorLib (RTDesc *, LibraryResource *, IPTR, int, const char * file, ULONG line, va_list);

static IPTR RT_Open (FileResource * rt, va_list args, BOOL * success);
static IPTR RT_Close (FileResource * rt);
static IPTR RT_ShowErrorFile (RTDesc *, FileResource *, IPTR, int, const char * file, ULONG line, va_list);
static IPTR RT_CheckFile (RTDesc * desc, const char * file, ULONG line, ULONG op, va_list args);

/* Return values of SearchFunc */
#define RT_SEARCH_FOUND 	    0
#define RT_SEARCH_NOT_FOUND	    1
#define RT_SEARCH_SIZE_MISMATCH     2

#define RT_FREE     0
#define RT_CHECK    1
#define RT_EXIT     2

RTDesc RT_Resources[RTT_MAX] =
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
	RT_Search,
	(RT_ShowError) RT_ShowErrorVec,
	NULL, /* Check */
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
    int t;

    if (InitWasCalled)
	return;

    InitWasCalled = 1;

    for (t=0; t<RTT_MAX; t++)
	NEWLIST(&RT_Resources[t].ResList);
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
    RTNode * rt, * next;
    int      t;

    if (!InitWasCalled)
	return;

    for (t=RTT_MAX-1; t>=0; t--)
    {
	for (next=GetHead(&RT_Resources[t].ResList); (rt=next); )
	{
	    next = GetSucc (rt);

	    RT_FreeResource (t, rt);
	}
    }
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
    IPTR     ret;
    va_list  args;
    RTNode * rtnew;
    BOOL     success;

    if (!InitWasCalled)
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
	rtnew,
	args,
	&success
    );

    va_end (args);

    if (success)
    {
	AddTail ((struct List *)&RT_Resources[rtt].ResList,
	    (struct Node *)rtnew
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
    IPTR     ret;
    va_list  args;
    RTNode * rt;

    if (!InitWasCalled)
	return FALSE;

    va_start (args, op);

    if (RT_Resources[rtt].CheckFunc)
    {
	ret = (*(RT_Resources[rtt].CheckFunc))
	(
	    &RT_Resources[rtt],
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
	    &RT_Resources[rtt],
	    &rt,
	    args
	);

	if (ret != RT_SEARCH_FOUND)
	{
	    ret = (*(RT_Resources[rtt].ShowError))
	    (
		&RT_Resources[rtt],
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
    IPTR     ret;
    va_list  args;
    RTNode * rt;

    if (!InitWasCalled)
	return FALSE;

    va_start (args, line);

    ret = (*(RT_Resources[rtt].SearchFunc))
    (
	&RT_Resources[rtt],
	&rt,
	args
    );

    if (ret == RT_SEARCH_FOUND)
    {
	ret = (*(RT_Resources[rtt].FreeFunc)) (rt);

	Remove ((struct Node *)rt);
	FreeMem (rt, RT_Resources[rtt].Size);
    }
    else
    {
	ret = (*(RT_Resources[rtt].ShowError))
	(
	    &RT_Resources[rtt],
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

    /* Print an error */
    (void) (*(RT_Resources[rtt].ShowError))
    (
	&RT_Resources[rtt],
	rtnode,
	0UL,
	RT_EXIT,
	NULL,
	0L,
	NULL
    );

    /* free the resource */
    (void) (*(RT_Resources[rtt].FreeFunc)) (rtnode);

    /* Remove resource from list and free it */
    Remove ((struct Node *)rtnode);
    FreeMem (rtnode, RT_Resources[rtt].Size);

} /* RT_FreeResource */

static IPTR RT_AllocMem (MemoryResource * rt, va_list args, BOOL * success)
{
    rt->Size = va_arg (args, ULONG);
    rt->Flags = va_arg (args, ULONG);

    rt->Memory = AllocMem (rt->Size, rt->Flags);

    if (rt->Memory)
	*success = TRUE;

    return (IPTR)(rt->Memory);
} /* RT_AllocMem */

static IPTR RT_FreeMem (MemoryResource * rt)
{
    FreeMem (rt->Memory, rt->Size);

    return TRUE;
} /* RT_FreeMem */

static IPTR RT_SearchMem (RTDesc * desc, MemoryResource ** rtptr,
	va_list args)
{
    MemoryResource * rt;
    APTR    memory;
    ULONG   size;

    memory = va_arg (args, APTR);
    size   = va_arg (args, ULONG);

    ForeachNode (&desc->ResList, rt)
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

static IPTR RT_ShowErrorMem (RTDesc * desc, MemoryResource * rt,
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

static IPTR RT_AllocVec (MemoryResource * rt, va_list args, BOOL * success)
{
    rt->Size = va_arg (args, ULONG);
    rt->Flags = va_arg (args, ULONG);

    rt->Memory = AllocVec (rt->Size, rt->Flags);

    if (rt->Memory)
	*success = TRUE;

    return (IPTR)(rt->Memory);
} /* RT_AllocVec */

static IPTR RT_FreeVec (MemoryResource * rt)
{
    FreeVec (rt->Memory);

    return TRUE;
} /* RT_FreeVec */

static IPTR RT_ShowErrorVec (RTDesc * desc, MemoryResource * rt,
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

static IPTR RT_OpenLibrary (LibraryResource * rt, va_list args, BOOL * success)
{
    rt->Name	= va_arg (args, STRPTR);
    rt->Version = va_arg (args, ULONG);

    rt->Lib = OpenLibrary (rt->Name, rt->Version);

    if (rt->Lib)
	*success = TRUE;

    return (IPTR)(rt->Lib);
} /* RT_OpenLibrary */

static IPTR RT_CloseLibrary (LibraryResource * rt)
{
    CloseLibrary (rt->Lib);

    return TRUE;
} /* RT_CloseLibrary */

static IPTR RT_ShowErrorLib (RTDesc * desc, LibraryResource * rt,
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
}

static BOOL CheckArea (APTR ptr, ULONG size, ULONG flags)
{
    if
    (
	(size & 0x8000000)
	|| !CheckPtr (ptr+size-1, flags)
    )
	return FALSE;

    return TRUE;
}

static IPTR RT_Open (FileResource * rt, va_list args, BOOL * success)
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

static IPTR RT_Close (FileResource * rt)
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

static IPTR RT_ShowErrorFile (RTDesc * desc, FileResource * rt,
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

static IPTR RT_CheckFile (RTDesc * desc,
			const char * file, ULONG line,
			ULONG op, va_list args)
{
    FileResource * rt;

    if (RT_Search (desc, (RTNode **)&rt, args) != RT_SEARCH_FOUND)
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

static IPTR RT_Search (RTDesc * desc, RTNode ** rtptr, va_list args)
{
    Resource * rt;
    APTR     * res;

    res = va_arg (args, APTR);

    ForeachNode (&desc->ResList, rt)
    {
	if (rt->Resource == res)
	{
	    *rtptr = (RTNode *)rt;

	    return RT_SEARCH_FOUND;
	}
    }

    return RT_SEARCH_NOT_FOUND;
}

