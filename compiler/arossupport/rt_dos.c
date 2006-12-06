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
#include <exec/memory.h>
#include <exec/execbase.h>
#include <stdarg.h>
#include <stdlib.h>
#include "etask.h"
#endif
#include <stdio.h>
#include <proto/exec.h>
#include <proto/arossupport.h>
#include <proto/dos.h>
#include <proto/alib.h>

typedef struct
{
    RTNode Node;
    BPTR   FH;
    STRPTR Path;
    ULONG  Mode;
}
FileResource;

static IPTR RT_Open (RTData * rtd, FileResource * rt, va_list args, BOOL * success);
static IPTR RT_Close (RTData * rtd, FileResource * rt);
static IPTR RT_ShowErrorFile (RTData * rtd, int, FileResource *, IPTR, int, const char * file, ULONG line, va_list);
static IPTR RT_CheckFile (RTData * rtd, int desc, const char * file, ULONG line, ULONG op, va_list args);

static const RTDesc RT_DosResources[] =
{
    { /* RTT_FILE */
	sizeof (FileResource),
	(RT_AllocFunc) RT_Open,
	(RT_FreeFunc)  RT_Close,
	RT_Search,
	(RT_ShowError) RT_ShowErrorFile,
	(RT_CheckFunc) RT_CheckFile,
    },
};

void RT_InitDos (void)
{
    RT_Resources[RTT_FILE] = &RT_DosResources[0];
}

void RT_ExitDos (void)
{
}

/**************************************
	      RT Files
**************************************/

static IPTR RT_Open (RTData * rtd, FileResource * rt, va_list args, BOOL * success)
{
    AROS_GET_SYSBASE_OK
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
    AROS_GET_SYSBASE_OK
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
    APTR prt = &rt;

    if (RT_Search (rtd, rtt, (RTNode **)prt, args) != RT_SEARCH_FOUND)
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
