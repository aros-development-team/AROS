/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function ScrollWindowRaster()
    Lang: english
*/
#include "intuition_intern.h"
#include "inputhandler.h"
#include "inputhandler_support.h"
#include <proto/graphics.h>

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH7(void, ScrollWindowRaster,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, win , A1),
	AROS_LHA(WORD           , dx  , D0),
	AROS_LHA(WORD           , dy  , D1),
	AROS_LHA(WORD           , xmin, D2),
	AROS_LHA(WORD           , ymin, D3),
	AROS_LHA(WORD           , xmax, D4),
	AROS_LHA(WORD           , ymax, D5),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 133, Intuition)

/*  FUNCTION
        Scrolls the content of the rectangle defined by (xmin,ymin)-
        (xmax,ymax) by (dx,dy) towards (0,0). This function calls 
        ScrollRasterBF(). 
        The advantage of this function over calling ScrollRasterBF() is
        that the window will be informed about damages. A damage happens
        if in a simple window parts from concelealed areas are scrolled
        to visible areas. The visible areas will be blank as simple
        windows store no data for concealed areas. 
        The blank parts that appear due to the scroll will be filled
        with EraseRect() and are not considered damaged areas. 

    INPUTS
        win       - pointer to window in which to scroll
        dx,dy     - scroll by (dx,dy) towards (0,0)
        xmin,ymin - upper left corner of the rectangle that will be
                    affected by the scroll
        xmax,ymax - lower rigfht corner of the rectangle that will be
                    affected by the scroll

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)


  ScrollRasterBF(win->RPort,
                 dx,
                 dy,
                 xmin,
                 ymin,
                 xmax,
                 ymax);
  /* Has there been damage to the layer? */
  if (0 != (win->RPort->Layer->Flags & LAYERREFRESH))
  {
    /* 
       Send a refresh message to the window if it doesn't already
       have one.
    */
    WindowNeedsRefresh(win, IntuitionBase);
  } 

  AROS_LIBFUNC_EXIT
} /* ScrollWindowRaster */
