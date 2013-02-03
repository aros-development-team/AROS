/***************************************************************************

 NBitmap.mcc - New Bitmap MUI Custom Class
 Copyright (C) 2006 by Daniel Allsopp
 Copyright (C) 2007-2008 by NList Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 NList classes Support Site:  http://www.sf.net/projects/nlist-classes

 $Id$

***************************************************************************/

#include <proto/exec.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <cybergraphx/cybergraphics.h>

#include "private.h"

static LONG do_alpha(LONG a, LONG v)
{
  LONG tmp  = (a*v);
  return ((tmp<<8) + tmp + 32768)>>16;
}

ULONG _WPAA(APTR src, UWORD srcx, UWORD srcy, UWORD srcmod, struct RastPort *rp, UWORD destx, UWORD desty, UWORD width, UWORD height, ULONG globalalpha)
{
  ULONG pixels = 0;

  ENTER();

  if(width > 0 && height > 0)
  {
    ULONG *buf;

    if((buf = AllocVec(width * 4, MEMF_ANY)) != NULL)
    {
      ULONG x, y;

      // Incorrect but cant bother with alpha channel math for now
      globalalpha = 255 - (globalalpha >> 24);

      for(y = 0; y < height; y++)
      {
        ULONG *spix;
        ULONG *dpix;

        ReadPixelArray(buf, 0, 0, width * 4, rp, destx, desty + y, width, 1, RECTFMT_ARGB);

        spix = (ULONG *)((ULONG)src + (srcy + y) * srcmod + srcx * sizeof(ULONG));
        dpix = buf;

        for(x = 0; x < width; x++)
        {
          ULONG srcpix, dstpix, a, r, g, b;

          srcpix = *spix++;
          dstpix = *dpix;

          a = (srcpix >> 24) & 0xff;
          r = (srcpix >> 16) & 0xff;
          g = (srcpix >> 8) & 0xff;
          b = (srcpix >> 0) & 0xff;

          a = a - globalalpha;

          if(a > 0)
          {
            ULONG dest_r, dest_g, dest_b;

            dest_r = (dstpix >> 16) & 0xff;
            dest_g = (dstpix >> 8) & 0xff;
            dest_b = (dstpix >> 0) & 0xff;

            dest_r += do_alpha(a, r - dest_r);
            dest_g += do_alpha(a, g - dest_g);
            dest_b += do_alpha(a, b - dest_b);

            dstpix = 0xff000000 | dest_r << 16 | dest_g << 8 | dest_b;
          }

          *dpix++ = dstpix;
          pixels++;
        }

        WritePixelArray(buf, 0, 0, width * 4, rp, destx, desty + y, width, 1, RECTFMT_ARGB);
      }

      FreeVec(buf);
    }
  }

  RETURN(pixels);
  return pixels;
}
