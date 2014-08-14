/*
 * $Id$
 *
 * Copyright (C) 1993-1999 by Jochen Wiedmann and Marcin Orlowski
 * Copyright (C) 2002-2010 by the FlexCat Open Source Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "flexcat.h"

/// Swappers...

unsigned short (*SwapWord)(unsigned short r) = NULL;
uint32 (*SwapLong)(uint32 r) = NULL;

unsigned short SwapWord21(unsigned short r)
{
  return (unsigned short)(((r & 0xff00) >> 8) | ((r & 0x00ff) << 8));
}

unsigned short SwapWord12(unsigned short r)
{
  return r;
}

uint32 SwapLong4321(uint32 r)
{
  return (((r & 0xff000000) >> 24) | ((r & 0x00ff0000) >> 8) | ((r & 0x0000ff00) << 8) | ((r & 0x000000ff) << 24));
}
uint32 SwapLong1234(uint32 r)
{
  return r;
}
///
/// SwapChoose

int SwapChoose(void)
{
  unsigned short w;
  uint32 d;

  strncpy((char *)&w, "\1\2", 2);
  strncpy((char *)&d, "\1\2\3\4", 4);

  if(w == 0x0201)
    SwapWord = SwapWord21;
  else if(w == 0x0102)
    SwapWord = SwapWord12;
  else
    return 0;

  if(d == 0x04030201)
    SwapLong = SwapLong4321;
  else if(d == 0x01020304)
    SwapLong = SwapLong1234;
  else
    return 0;

  return 1;
}
///
