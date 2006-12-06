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
#include <stdio.h>
#include <proto/alib.h>
#include "etask.h"
#endif
#include <proto/exec.h>
#include <proto/arossupport.h>
#include <proto/intuition.h>

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

static IPTR RT_OpenScreen (RTData * rtd, ScreenResource * rt, va_list args, BOOL * success);
static IPTR RT_CloseScreen (RTData * rtd, ScreenResource * rt);
static IPTR RT_ShowErrorScreen (RTData * rtd, int, ScreenResource *, IPTR, int, const char * file, ULONG line, va_list);
static IPTR RT_CheckScreen (RTData * rtd, int desc, const char * file, ULONG line, ULONG op, va_list args);

static IPTR RT_OpenWindow (RTData * rtd, WindowResource * rt, va_list args, BOOL * success);
static IPTR RT_CloseWindow (RTData * rtd, WindowResource * rt);
static IPTR RT_ShowErrorWindow (RTData * rtd, int, WindowResource *, IPTR, int, const char * file, ULONG line, va_list);
static IPTR RT_CheckWindow (RTData * rtd, int desc, const char * file, ULONG line, ULONG op, va_list args);

static const RTDesc RT_IntuitionResources[] =
{
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

void RT_InitIntuition (void)
{
    RT_Resources[RTT_SCREEN] = &RT_IntuitionResources[0];
    RT_Resources[RTT_WINDOW] = &RT_IntuitionResources[1];
}

void RT_ExitIntuition (void)
{
}

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
	APTR prtwin = &rtwin;

	kprintf ("CloseScreen(): There are still windows open on this screen\n"
		"    Screen=%p opened at %s:%d\n"
		, rt->Screen
		, rt->Node.File, rt->Node.Line
	);

	while ((win = rt->Screen->FirstWindow))
	{
	    if (RT_Search (rtd, RTT_WINDOW, (RTNode **)prtwin, NULL) == RT_SEARCH_FOUND)
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
    APTR prt = &rt;

    if (RT_Search (rtd, rtt, (RTNode **)prt, args) != RT_SEARCH_FOUND)
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
    APTR prt = &rt;

    if (RT_Search (rtd, rtt, (RTNode **)prt, args) != RT_SEARCH_FOUND)
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

