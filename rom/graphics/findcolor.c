/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Find the closest matching color in a colormap
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/view.h>

/*****************************************************************************

    NAME */
	#include <clib/graphics_protos.h>

	AROS_LH5(ULONG, FindColor,

/*  SYNOPSIS */
	AROS_LHA(struct ColorMap *, cm    , A3),
	AROS_LHA(ULONG            , r     , D1),
	AROS_LHA(ULONG            , g     , D2),
	AROS_LHA(ULONG            , b     , D3),
	AROS_LHA(ULONG            , maxpen, D4),

/*  LOCATION */
	struct GfxBase *, GfxBase, 168, Graphics)

/*  FUNCTION
        Find the closest matching color in the given colormap.

    INPUTS
        cm - colormap structure
        r  - red level   (32 bit left justified fraction)
        g  - green level (32 bit left justified fraction)
        b  - blue level  (32 bit left justified fraction)
        maxpen - the maximum entry in the color table to search.

    RESULT
        The pen number with the closest match will be returned.
        No new pens are allocated and therefore the returned pen
        should not be released via ReleasePen().

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  ULONG retval = 0;

  if (NULL != cm)
  {
    ULONG index = 0;
    ULONG best_distance = (ULONG)-1;
    ULONG distance;

    if (-1 == maxpen && NULL != cm->PalExtra)
    {    
      struct PaletteExtra * pe = cm->PalExtra;
      /*
      ** According to the autodocs maxpen=-1 and an installed PalExtra
      ** structure will limit the search to only those pens which could
      ** be rendered in. So which are these ones and which ones are the
      ** sprite colors in a color map?????
      */
#warning What do I do in this case?
      /* Currently I just search the list of shared pens */
      index = pe->pe_FirstShared;
      while (-1 != (BYTE)index)
      {
        distance = color_distance(cm,r,g,b,index);
        if (distance < best_distance)
        {
          best_distance = distance;
          retval = index;
        }
        index = pe->pe_AllocList[index];
      }
    }
    else
    {
      while (index <= maxpen && index < cm->Count)
      {
        distance = color_distance(cm,r,g,b,index);
        if (distance < best_distance)
        {
          best_distance = distance;
          retval = index;
        }
        index++;
      } 
    }
  }

  return retval;
  
  AROS_LIBFUNC_EXIT
} /* FindColor */
