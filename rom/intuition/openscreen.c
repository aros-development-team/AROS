/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/09/21 14:11:39  digulla
    Open and close screens


    Desc:
    Lang: english
*/
#include "intuition_intern.h"
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <clib/graphics_protos.h>

#ifndef DEBUG_OpenScreen
#   define DEBUG_OpenScreen 0
#endif
#if DEBUG_OpenScreen
#   undef DEBUG
#   define DEBUG 1
#endif
#include <aros/debug.h>

/*****************************************************************************

    NAME */
	#include <intuition/screens.h>
	#include <clib/intuition_protos.h>

	__AROS_LH1(struct Screen *, OpenScreen,

/*  SYNOPSIS */
	__AROS_LHA(struct NewScreen *, newScreen, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 33, Intuition)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    struct Screen * screen;
#define COPY(x)     screen->x = newScreen->x

    D(bug("OpenScreen (%p = { Left=%d Top=%d Width=%d Height=%d Depth=%d })\n"
	, newScreen
	, newScreen->LeftEdge
	, newScreen->TopEdge
	, newScreen->Width
	, newScreen->Height
	, newScreen->Depth
    ));

    if ((screen = AllocMem (sizeof (struct Screen), MEMF_ANY | MEMF_CLEAR)))
    {
	COPY(LeftEdge);
	COPY(TopEdge);
	COPY(Width);
	COPY(Height);
	COPY(DetailPen);
	COPY(BlockPen);
	COPY(Font);
	COPY(DefaultTitle);

	screen->Flags = newScreen->Type;

	screen->BitMap.Depth = newScreen->Depth;

	screen->BarHeight = 0;
	screen->BarVBorder = 0;
	screen->BarHBorder = 0;
	screen->MenuVBorder = 0;
	screen->MenuHBorder = 0;

	screen->WBorTop = 0;
	screen->WBorLeft = 0;
	screen->WBorRight = 0;
	screen->WBorBottom = 0;

	InitRastPort (&screen->RastPort);

	screen->RastPort.BitMap = &screen->BitMap;

	screen->Title = newScreen->DefaultTitle;

	screen->NextScreen = IntuitionBase->FirstScreen;
	IntuitionBase->FirstScreen =
	    IntuitionBase->ActiveScreen = screen;
    }

    ReturnPtr ("OpenScreen", struct Screen *, screen);
    __AROS_FUNC_EXIT
} /* OpenScreen */
