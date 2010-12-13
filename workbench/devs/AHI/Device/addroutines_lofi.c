/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1996-2005 Martin Blom <martin@blom.org>
     
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
     
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
     
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge,
     MA 02139, USA.
*/

#include <config.h>

#include "addroutines.h"

/******************************************************************************
** Add-Routines ***************************************************************
******************************************************************************/

/* Forward mixing code */

#define offseti ( (long) ( offset >> 32 ) )

LONG
AddLofiByteMono( ADDARGS )
{
  BYTE    *src    = Src;
  WORD    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     lastpoint, sample;

  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    sample = src[ offseti ] << 8;

    if( StopAtZero &&
        ( ( lastpoint < 0 && sample >= 0 ) ||
          ( lastpoint > 0 && sample <= 0 ) ) )
    {
      break;
    }

    lastpoint = sample;

    *dst++ += ( ScaleLeft * sample ) >> 16;

    offset += Add;
  }

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddLofiByteStereo( ADDARGS )
{
  BYTE    *src    = Src;
  WORD    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     lastpoint, sample;

  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code
  
  for( i = 0; i < Samples; i++)
  {
    sample = src[ offseti ] << 8;

    if( StopAtZero &&
        ( ( lastpoint < 0 && sample >= 0 ) ||
          ( lastpoint > 0 && sample <= 0 ) ) )
    {
      break;
    }

    lastpoint = sample;

    *dst++ += ( ScaleLeft * sample ) >> 16;
    *dst++ += ( ScaleRight * sample ) >> 16;

    offset += Add;
  }

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddLofiBytesMono( ADDARGS )
{
  BYTE    *src    = Src;
  WORD    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     lastpointL, lastpointR, sampleL, sampleR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    sampleL = src[ offseti * 2 + 0 ] << 8;
    sampleR = src[ offseti * 2 + 1 ] << 8;

    if( StopAtZero &&
        ( ( lastpointL < 0 && sampleL >= 0 ) ||
          ( lastpointR < 0 && sampleR >= 0 ) ||
          ( lastpointL > 0 && sampleL <= 0 ) ||
          ( lastpointR > 0 && sampleR <= 0 ) ) )
    {
      break;
    }

    lastpointL = sampleL;
    lastpointR = sampleR;

    *dst++ += ( ScaleLeft * sampleL + ScaleRight * sampleR ) >> 16;

    offset += Add;
  }

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddLofiBytesStereo( ADDARGS )
{
  BYTE    *src    = Src;
  WORD    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     lastpointL, lastpointR, sampleL, sampleR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    sampleL = src[ offseti * 2 + 0 ] << 8;
    sampleR = src[ offseti * 2 + 1 ] << 8;

    if( StopAtZero &&
        ( ( lastpointL < 0 && sampleL >= 0 ) ||
          ( lastpointR < 0 && sampleR >= 0 ) ||
          ( lastpointL > 0 && sampleL <= 0 ) ||
          ( lastpointR > 0 && sampleR <= 0 ) ) )
    {
      break;
    }

    lastpointL = sampleL;
    lastpointR = sampleR;

    *dst++ += ( ScaleLeft * sampleL ) >> 16;
    *dst++ += ( ScaleRight * sampleR ) >> 16;

    offset += Add;
  }

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddLofiWordMono( ADDARGS )
{
  WORD    *src    = Src;
  WORD    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     lastpoint, sample;

  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    sample = src[ offseti ];

    if( StopAtZero &&
        ( ( lastpoint < 0 && sample >= 0 ) ||
          ( lastpoint > 0 && sample <= 0 ) ) )
    {
      break;
    }

    lastpoint = sample;

    *dst++ += ( ScaleLeft * sample ) >> 16;

    offset += Add;
  }

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddLofiWordStereo( ADDARGS )
{
  WORD    *src    = Src;
  WORD    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     lastpoint, sample;

  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code
  
  for( i = 0; i < Samples; i++)
  {
    sample = src[ offseti ];

    if( StopAtZero &&
        ( ( lastpoint < 0 && sample >= 0 ) ||
          ( lastpoint > 0 && sample <= 0 ) ) )
    {
      break;
    }

    lastpoint = sample;

    *dst++ += ( ScaleLeft * sample ) >> 16;
    *dst++ += ( ScaleRight * sample ) >> 16;

    offset += Add;
  }

  *Dst    = dst;
  *Offset = offset;

  return i;
}

LONG
AddLofiWordsMono( ADDARGS )
{
  WORD    *src    = Src;
  WORD    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     lastpointL, lastpointR, sampleL, sampleR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    sampleL = src[ offseti * 2 + 0 ];
    sampleR = src[ offseti * 2 + 1 ];

    if( StopAtZero &&
        ( ( lastpointL < 0 && sampleL >= 0 ) ||
          ( lastpointR < 0 && sampleR >= 0 ) ||
          ( lastpointL > 0 && sampleL <= 0 ) ||
          ( lastpointR > 0 && sampleR <= 0 ) ) )
    {
      break;
    }

    lastpointL = sampleL;
    lastpointR = sampleR;

    *dst++ += ( ScaleLeft * sampleL + ScaleRight * sampleR ) >> 16;

    offset += Add;
  }

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddLofiWordsStereo( ADDARGS )
{
  WORD    *src    = Src;
  WORD    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     lastpointL, lastpointR, sampleL, sampleR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    sampleL = src[ offseti * 2 + 0 ];
    sampleR = src[ offseti * 2 + 1 ];

    if( StopAtZero &&
        ( ( lastpointL < 0 && sampleL >= 0 ) ||
          ( lastpointR < 0 && sampleR >= 0 ) ||
          ( lastpointL > 0 && sampleL <= 0 ) ||
          ( lastpointR > 0 && sampleR <= 0 ) ) )
    {
      break;
    }

    lastpointL = sampleL;
    lastpointR = sampleR;

    *dst++ += ( ScaleLeft * sampleL ) >> 16;
    *dst++ += ( ScaleRight * sampleR ) >> 16;

    offset += Add;
  }

  *Dst    = dst;
  *Offset = offset;

  return i;
}

/*****************************************************************************/

/* Backward mixing code */

LONG
AddLofiByteMonoB( ADDARGS )
{
  BYTE    *src    = Src;
  WORD    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     lastpoint, sample;

  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    sample = src[ offseti ] << 8;

    if( StopAtZero &&
        ( ( lastpoint < 0 && sample >= 0 ) ||
          ( lastpoint > 0 && sample <= 0 ) ) )
    {
      break;
    }

    lastpoint = sample;

    *dst++ += ( ScaleLeft * sample ) >> 16;

    offset -= Add;
  }

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddLofiByteStereoB( ADDARGS )
{
  BYTE    *src    = Src;
  WORD    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     lastpoint, sample;

  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code
  
  for( i = 0; i < Samples; i++)
  {
    sample = src[ offseti ] << 8;

    if( StopAtZero &&
        ( ( lastpoint < 0 && sample >= 0 ) ||
          ( lastpoint > 0 && sample <= 0 ) ) )
    {
      break;
    }

    lastpoint = sample;

    *dst++ += ( ScaleLeft * sample ) >> 16;
    *dst++ += ( ScaleRight * sample ) >> 16;

    offset -= Add;
  }

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddLofiBytesMonoB( ADDARGS )
{
  BYTE    *src    = Src;
  WORD    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     lastpointL, lastpointR, sampleL, sampleR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    sampleL = src[ offseti * 2 + 0 ] << 8;
    sampleR = src[ offseti * 2 + 1 ] << 8;

    if( StopAtZero &&
        ( ( lastpointL < 0 && sampleL >= 0 ) ||
          ( lastpointR < 0 && sampleR >= 0 ) ||
          ( lastpointL > 0 && sampleL <= 0 ) ||
          ( lastpointR > 0 && sampleR <= 0 ) ) )
    {
      break;
    }

    lastpointL = sampleL;
    lastpointR = sampleR;

    *dst++ += ( ScaleLeft * sampleL + ScaleRight * sampleR ) >> 16;

    offset -= Add;
  }

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddLofiBytesStereoB( ADDARGS )
{
  BYTE    *src    = Src;
  WORD    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     lastpointL, lastpointR, sampleL, sampleR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    sampleL = src[ offseti * 2 + 0 ] << 8;
    sampleR = src[ offseti * 2 + 1 ] << 8;

    if( StopAtZero &&
        ( ( lastpointL < 0 && sampleL >= 0 ) ||
          ( lastpointR < 0 && sampleR >= 0 ) ||
          ( lastpointL > 0 && sampleL <= 0 ) ||
          ( lastpointR > 0 && sampleR <= 0 ) ) )
    {
      break;
    }

    lastpointL = sampleL;
    lastpointR = sampleR;

    *dst++ += ( ScaleLeft * sampleL ) >> 16;
    *dst++ += ( ScaleRight * sampleR ) >> 16;

    offset -= Add;
  }

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddLofiWordMonoB( ADDARGS )
{
  WORD    *src    = Src;
  WORD    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     lastpoint, sample;

  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    sample = src[ offseti ];

    if( StopAtZero &&
        ( ( lastpoint < 0 && sample >= 0 ) ||
          ( lastpoint > 0 && sample <= 0 ) ) )
    {
      break;
    }

    lastpoint = sample;

    *dst++ += ( ScaleLeft * sample ) >> 16;

    offset -= Add;
  }

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddLofiWordStereoB( ADDARGS )
{
  WORD    *src    = Src;
  WORD    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     lastpoint, sample;

  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code
  
  for( i = 0; i < Samples; i++)
  {
    sample = src[ offseti ];

    if( StopAtZero &&
        ( ( lastpoint < 0 && sample >= 0 ) ||
          ( lastpoint > 0 && sample <= 0 ) ) )
    {
      break;
    }

    lastpoint = sample;

    *dst++ += ( ScaleLeft * sample ) >> 16;
    *dst++ += ( ScaleRight * sample ) >> 16;

    offset -= Add;
  }

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddLofiWordsMonoB( ADDARGS )
{
  WORD    *src    = Src;
  WORD    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     lastpointL, lastpointR, sampleL, sampleR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    sampleL = src[ offseti * 2 + 0 ];
    sampleR = src[ offseti * 2 + 1 ];

    if( StopAtZero &&
        ( ( lastpointL < 0 && sampleL >= 0 ) ||
          ( lastpointR < 0 && sampleR >= 0 ) ||
          ( lastpointL > 0 && sampleL <= 0 ) ||
          ( lastpointR > 0 && sampleR <= 0 ) ) )
    {
      break;
    }

    lastpointL = sampleL;
    lastpointR = sampleR;

    *dst++ += ( ScaleLeft * sampleL + ScaleRight * sampleR ) >> 16;

    offset -= Add;
  }

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddLofiWordsStereoB( ADDARGS )
{
  WORD    *src    = Src;
  WORD    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     lastpointL, lastpointR, sampleL, sampleR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    sampleL = src[ offseti * 2 + 0 ];
    sampleR = src[ offseti * 2 + 1 ];

    if( StopAtZero &&
        ( ( lastpointL < 0 && sampleL >= 0 ) ||
          ( lastpointR < 0 && sampleR >= 0 ) ||
          ( lastpointL > 0 && sampleL <= 0 ) ||
          ( lastpointR > 0 && sampleR <= 0 ) ) )
    {
      break;
    }

    lastpointL = sampleL;
    lastpointR = sampleR;

    *dst++ += ( ScaleLeft * sampleL ) >> 16;
    *dst++ += ( ScaleRight * sampleR ) >> 16;

    offset -= Add;
  }

  *Dst    = dst;
  *Offset = offset;

  return i;
}

/*****************************************************************************/
