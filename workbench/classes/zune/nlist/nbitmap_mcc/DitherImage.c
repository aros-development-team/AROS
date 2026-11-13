/***************************************************************************

 NBitmap.mcc - New Bitmap MUI Custom Class
 Copyright (C) 2006 by Daniel Allsopp
 Copyright (C) 2007-2013 by NList Open Source Team

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
#include <proto/utility.h>
#include <mui/NBitmap_mcc.h>

#include "private.h"
#include "DitherImage.h"
#include "Debug.h"

#ifndef MEMF_SHARED
  #define MEMF_SHARED MEMF_ANY
#endif

#define RAWIDTH(w)                      ((((UWORD)(w))+15)>>3 & 0xFFFE)

APTR DitherImageA(CONST_APTR data, struct TagItem *tags)
{
  struct TagItem *tag;
  uint32 width = 0;
  uint32 height = 0;
  uint32 format = 0;
  const uint32 *colorMap = NULL;
  const int32 *penMap = NULL;
  APTR result = NULL;
  uint8 **maskPtr = NULL;

  ENTER();

  while((tag = NextTagItem((APTR)&tags)) != NULL)
  {
    switch(tag->ti_Tag)
    {
      case DITHERA_Width:
        width = tag->ti_Data;
      break;

      case DITHERA_Height:
        height = tag->ti_Data;
      break;

      case DITHERA_Format:
        format = tag->ti_Data;
      break;

      case DITHERA_ColorMap:
        colorMap = (uint32 *)tag->ti_Data;
      break;

      case DITHERA_PenMap:
        penMap = (int32 *)tag->ti_Data;
      break;

      case DITHERA_MaskPlane:
        maskPtr = (uint8 **)tag->ti_Data;
      break;
    }
  }

  if(data != NULL && colorMap != NULL && width > 0 && height > 0)
  {
    // the 8bit chunky data don't need to reside in chip memory
    if((result = AllocVecShared(width * height, MEMF_ANY)) != NULL)
    {
      uint8 *mask = NULL;
      uint8 *mPtr = NULL;
      uint32 y;
      uint8 *dataPtr = (uint8 *)data;
      uint8 *resultPtr = (uint8 *)result;

      // only ARGB raw data contain transparency data, hence we need to
      // allocate a mask plane only for these and only if the calling
      // function is interested in a mask at all
      if(format == MUIV_NBitmap_Type_ARGB32 && maskPtr != NULL)
      {
        // the mask MUST reside in chip memory
        mask = AllocVec(RAWIDTH(width) * height, MEMF_CLEAR|MEMF_CHIP);
        *maskPtr = mask;
        mPtr = mask;
      }

      for(y = 0; y < height; y++)
      {
        uint32 x;
        uint8 bitMask = 0x80;

        for(x = 0; x < width; x++)
        {
          uint8 a, r, g, b;
          int32 i;
          int32 bestIndex;
          uint32 bestError;

          // obtain the pixel's A, R, G and B values from the raw data
          switch(format)
          {
            case MUIV_NBitmap_Type_CLUT8:
              a = (colorMap[dataPtr[0]] >> 24) & 0xff;
              r = (colorMap[dataPtr[0]] >> 16) & 0xff;
              g = (colorMap[dataPtr[0]] >>  8) & 0xff;
              b = (colorMap[dataPtr[0]] >>  0) & 0xff;
              dataPtr += 1;
            break;

            case MUIV_NBitmap_Type_RGB24:
              a = 0xff;
              r = dataPtr[0];
              g = dataPtr[1];
              b = dataPtr[2];
              dataPtr += 3;
            break;

            case MUIV_NBitmap_Type_ARGB32:
              a = dataPtr[0];
              r = dataPtr[1];
              g = dataPtr[2];
              b = dataPtr[3];
              dataPtr += 4;
            break;

            default:
              a = 0x00;
              r = 0x00;
              g = 0x00;
              b = 0x00;
            break;
          }

          // now calculate the best matching color from the given color map
          bestIndex = -1;
          bestError = 0xffffffffUL;

          for(i = 0; i < 256; i++)
          {
            int32 dr, dg, db;
            uint32 error;

            // calculate the geometric difference to the current color
            dr = (int32)((colorMap[i] >> 16) & 0xff) - (int32)r;
            dg = (int32)((colorMap[i] >>  8) & 0xff) - (int32)g;
            db = (int32)((colorMap[i] >>  0) & 0xff) - (int32)b;
            error = dr * dr + dg * dg + db * db;

            if(bestError > error)
            {
              // remember this as the best matching color so far
              bestError = error;
              bestIndex = i;

              // bail out if we found an exact match
              if(error == 0x00000000UL)
                break;
            }
          }

          // put the calculated color number into the destination LUT8 image
          // using an additional pen map if available
          if(bestIndex != -1)
          {
            if(penMap != NULL)
              *resultPtr++ = penMap[bestIndex];
            else
              *resultPtr++ = bestIndex;
          }
          else
          {
            // no matching color found, use color 0
            // can this happen at all?
            *resultPtr++ = 0;
          }

          if(mPtr != NULL)
          {
            // if we have a mask and the alpha value is >= 0x80 the
            // pixel is treated as non-transparent
            if(a >= 0x80)
              mPtr[x/8] |= bitMask;

            bitMask >>= 1;
            if(bitMask == 0x00)
              bitMask = 0x80;
          }
        }

        // advance the mask pointer by one line
        if(mPtr != NULL)
          mPtr += RAWIDTH(width);
      }
    }
  }

  RETURN(result);
  return result;
}

#ifdef __AROS__
APTR STDARGS VARARGS68K DitherImage(CONST_APTR data, Tag tag1, ...)
{
  AROS_SLOWSTACKTAGS_PRE_AS(tag1, APTR)
  retval = DitherImageA(data, AROS_SLOWSTACKTAGS_ARG(tag1));
  AROS_SLOWSTACKTAGS_POST
}
#else
#if !defined(__PPC__)
APTR STDARGS VARARGS68K DitherImage(CONST_APTR data, ...)
{
  APTR ret;
  VA_LIST args;

  VA_START(args, data);
  ret = DitherImageA(data, (struct TagItem *)VA_ARG(args, IPTR));
  VA_END(args);

  return ret;
}
#endif
#endif

void FreeDitheredImage(APTR image, APTR mask)
{
  ENTER();

  if(image != NULL)
    FreeVec(image);
  if(mask != NULL)
    FreeVec(mask);

  LEAVE();
}
