/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function Draw()
    Lang: english
*/

#include <graphics/rastport.h>
#include <proto/graphics.h>

/*****************************************************************************

    NAME */

	AROS_LH3(void, Draw,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(LONG             , x, D0),
	AROS_LHA(LONG             , y, D1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 41, Graphics)

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
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

#if 1
  driver_Draw(rp, x, y, GfxBase);
#else
  LONG x_end = x;
  LONG y_end = y;
  LONG x_step = 0, y_step = 0;
  LONG dx = 1, dy = 1;
  LONG _x, _y;
  LONG steps, counter;
  
  EnterFunc(bug("driver_Draw(rp=%p, x=%d, y=%d)\n", rp, x, y));
  

    if (!CorrectDriverData (rp, GfxBase))
    	return;

  if (rp->cp_x != x)
  {
    if (rp->cp_x > x)
    {
      x_step = -1;
      dx = rp->cp_x - x;
    }
    else
    {
      x_step = 1;
      dx = x - rp->cp_x;
    }
  }

  if (rp->cp_y != y)
  {
    if (rp->cp_y > y)
    {
      y_step = -1;
      dy = rp->cp_y - y;
    }
    else
    {
      y_step = 1;
      dy = y - rp->cp_y;
    }
  }


  _x = 0;
  _y = 0;
  x = rp->cp_x;
  y = rp->cp_y;
  rp->cp_x = x_end;
  rp->cp_y = y_end;

  if (dx > dy)
    steps = dx;
  else
    steps = dy;
    
  counter = 0;  
  while (counter <= steps)
  {
    counter++;
    WritePixel(rp, x, y);

    if (dx > dy)
    {
      x += x_step;
      /* _x += dx; unnecessary in this case */
      _y += dy;
      if (_y >= dx)
      {
        _y -= dx;
        y += y_step;
      }
    }
    else
    {
      y += y_step;
      _x += dx;
      /* _y += dy; unnecessary in this case */
      if (_x >= dy)
      {
        _x -= dy;
        x += x_step;
      }
    }
  }

#endif

  AROS_LIBFUNC_EXIT
} /* Draw */
