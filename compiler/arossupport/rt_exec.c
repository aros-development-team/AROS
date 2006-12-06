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
#include <exec/execbase.h>
#include <stdlib.h>
#include <stdio.h>
#include "etask.h"
#endif
#include <exec/tasks.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <stdarg.h>
#include <proto/exec.h>
#include <proto/arossupport.h>
#include <proto/alib.h>

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

static const RTDesc RT_ExecResources[] =
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
};

void RT_InitExec (void)
{
    RT_Resources[RTT_ALLOCMEM] = &RT_ExecResources[0];
    RT_Resources[RTT_ALLOCVEC] = &RT_ExecResources[1];
    RT_Resources[RTT_PORT]     = &RT_ExecResources[2];
    RT_Resources[RTT_LIBRARY]  = &RT_ExecResources[3];
}

void RT_ExitExec (void)
{
}

/**************************************
	   RT Memory
**************************************/

static IPTR RT_AllocMem (RTData * rtd, MemoryResource * rt, va_list args, BOOL * success)
{
    AROS_GET_SYSBASE_OK
    rt->Size = va_arg (args, ULONG);
    rt->Flags = va_arg (args, ULONG);

    rt->Memory = AllocMem (rt->Size, rt->Flags);

    if (rt->Memory)
	*success = TRUE;

    return (IPTR)(rt->Memory);
} /* RT_AllocMem */

static IPTR RT_FreeMem (RTData * rtd, MemoryResource * rt)
{
    AROS_GET_SYSBASE_OK
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
    AROS_GET_SYSBASE_OK
    rt->Size = va_arg (args, ULONG);
    rt->Flags = va_arg (args, ULONG);

    rt->Memory = AllocVec (rt->Size, rt->Flags);

    if (rt->Memory)
	*success = TRUE;

    return (IPTR)(rt->Memory);
} /* RT_AllocVec */

static IPTR RT_FreeVec (RTData * rtd, MemoryResource * rt)
{
    AROS_GET_SYSBASE_OK
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
	    if (rt && rt->Node.Flags & RTNF_DONT_FREE)
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
    AROS_GET_SYSBASE_OK
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
    APTR prt = &rt;

    if (RT_Search (rtd, rtt, (RTNode **)prt, args) != RT_SEARCH_FOUND)
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
    AROS_GET_SYSBASE_OK
    rt->Name	= va_arg (args, STRPTR);
    rt->Version = va_arg (args, ULONG);

    rt->Lib = OpenLibrary (rt->Name, rt->Version);

    if (rt->Lib)
	*success = TRUE;

    return (IPTR)(rt->Lib);
} /* RT_OpenLibrary */

static IPTR RT_CloseLibrary (RTData * rtd, LibraryResource * rt)
{
    AROS_GET_SYSBASE_OK
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
