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

typedef long long LONGLONG;

/******************************************************************************
** Add-Routines ***************************************************************
******************************************************************************/

/*
** LONG      Samples
** LONG      ScaleLeft
** LONG      ScaleRight
** LONG	    *StartPointLeft
** LONG	    *StartPointRight
** void     *Src
** void    **Dst
** LONG	     FirstOffsetI
** Fixed64  *Offset
** Fixed64   Add
** BOOL      StopAtZero
*/

/*

Notes:

The fraction offset is divided by two in order to make sure that the
calculation of linearsample fits a LONG (0 =< offsetf <= 32767).

The routines could be faster, of course.  One idea is to split the for loop
into two loops in order to eliminate the FirstOffsetI test in the second loop.

*/ 

/*****************************************************************************/

/* Forward mixing code */

#define offseti ( (long) ( offset >> 32 ) )

#define offsetf ( offset & 0xffffffffULL )

LONG
AddLongMono( ADDARGS )
{
  LONG    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint = 0; // Make compiler happy
  LONG     lastpoint;

  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpoint = *StartPointLeft << 16;
    }
    else
    {
      startpoint = src[ offseti - 1 ];
    }

    endpoint = src[ offseti ];

    startpoint += (LONG) (((LONGLONG) (endpoint - startpoint) * offsetf ) >> 32);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += (LONG) ( ( (LONGLONG) ScaleLeft * startpoint ) >> 16 );

    offset += Add;
  }

  *StartPointLeft = endpoint >> 16;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddLongStereo( ADDARGS )
{
  LONG    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint = 0; // Make compiler happy
  LONG     lastpoint;

  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code
  
  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpoint = *StartPointLeft << 16;
    }
    else
    {
      startpoint = src[ offseti - 1 ];
    }

    endpoint = src[ offseti ];

    startpoint += (LONG) (((LONGLONG) (endpoint - startpoint) * offsetf ) >> 32);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += (LONG) ( ( (LONGLONG) ScaleLeft * startpoint ) >> 16 );
    *dst++ += (LONG) ( ( (LONGLONG) ScaleRight * startpoint ) >> 16 );

    offset += Add;
  }

  *StartPointLeft = endpoint >> 16;

  *Dst    = dst;
  *Offset = offset;

  return i;
}

LONG
AddLongsMono( ADDARGS )
{
  LONG    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL = 0, endpointR = 0; // Make compiler happy
  LONG     lastpointL, lastpointR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpointL = *StartPointLeft << 16;
      startpointR = *StartPointRight << 16;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 - 2 ];
      startpointR = src[ offseti * 2 + 1 - 2 ];
    }

    endpointL = src[ offseti * 2 + 0 ];
    endpointR = src[ offseti * 2 + 1 ];

    startpointL += (LONG) (((LONGLONG) (endpointL - startpointL) * offsetf ) >> 32);
    startpointR += (LONG) (((LONGLONG) (endpointR - startpointR) * offsetf ) >> 32);

    if( StopAtZero &&
        ( ( lastpointL < 0 && startpointL >= 0 ) ||
          ( lastpointR < 0 && startpointR >= 0 ) ||
          ( lastpointL > 0 && startpointL <= 0 ) ||
          ( lastpointR > 0 && startpointR <= 0 ) ) )
    {
      break;
    }

    lastpointL = startpointL;
    lastpointR = startpointR;

    *dst++ += (LONG) ( ( (LONGLONG) ScaleLeft * startpointL  ) >> 16 )
      + (LONG) ( ( (LONGLONG) ScaleRight * startpointR ) >> 16 );

    offset += Add;
  }

  *StartPointLeft = endpointL >> 16;
  *StartPointRight = endpointR >> 16;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddLongsStereo( ADDARGS )
{
  LONG    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL = 0, endpointR = 0; // Make compiler happy
  LONG     lastpointL, lastpointR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpointL = *StartPointLeft << 16;
      startpointR = *StartPointRight << 16;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 - 2 ];
      startpointR = src[ offseti * 2 + 1 - 2 ];
    }

    endpointL = src[ offseti * 2 + 0 ];
    endpointR = src[ offseti * 2 + 1 ];

    startpointL += (LONG) (((LONGLONG) (endpointL - startpointL) * offsetf ) >> 32);
    startpointR += (LONG) (((LONGLONG) (endpointR - startpointR) * offsetf ) >> 32);

    if( StopAtZero &&
        ( ( lastpointL < 0 && startpointL >= 0 ) ||
          ( lastpointR < 0 && startpointR >= 0 ) ||
          ( lastpointL > 0 && startpointL <= 0 ) ||
          ( lastpointR > 0 && startpointR <= 0 ) ) )
    {
      break;
    }

    lastpointL = startpointL;
    lastpointR = startpointR;

    *dst++ += (LONG) ( ( (LONGLONG) ScaleLeft * startpointL ) >> 16 );
    *dst++ += (LONG) ( ( (LONGLONG) ScaleRight * startpointR ) >> 16 );

    offset += Add;
  }

  *StartPointLeft = endpointL >> 16;
  *StartPointRight = endpointR >> 16;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
Add71Mono( ADDARGS )
{
  LONG    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL = 0, endpointR = 0; // Make compiler happy
  LONG     lastpointL, lastpointR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpointL = *StartPointLeft << 16;
      startpointR = *StartPointRight << 16;
    }
    else
    {
      startpointL = src[ offseti * 8 + 0 - 8 ];
      startpointR = src[ offseti * 8 + 1 - 8 ];
    }

    endpointL = src[ offseti * 8 + 0 ];
    endpointR = src[ offseti * 8 + 1 ];

    startpointL += (LONG) (((LONGLONG) (endpointL - startpointL) * offsetf ) >> 32);
    startpointR += (LONG) (((LONGLONG) (endpointR - startpointR) * offsetf ) >> 32);

    if( StopAtZero &&
        ( ( lastpointL < 0 && startpointL >= 0 ) ||
          ( lastpointR < 0 && startpointR >= 0 ) ||
          ( lastpointL > 0 && startpointL <= 0 ) ||
          ( lastpointR > 0 && startpointR <= 0 ) ) )
    {
      break;
    }

    lastpointL = startpointL;
    lastpointR = startpointR;

    *dst++ += (LONG) ( ( (LONGLONG) ScaleLeft * startpointL  ) >> 16 )
      + (LONG) ( ( (LONGLONG) ScaleRight * startpointR ) >> 16 );

    offset += Add;
  }

  *StartPointLeft = endpointL >> 16;
  *StartPointRight = endpointR >> 16;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
Add71Stereo( ADDARGS )
{
  LONG    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL = 0, endpointR = 0; // Make compiler happy
  LONG     lastpointL, lastpointR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpointL = *StartPointLeft << 16;
      startpointR = *StartPointRight << 16;
    }
    else
    {
      startpointL = src[ offseti * 8 + 0 - 8 ];
      startpointR = src[ offseti * 8 + 1 - 8 ];
    }

    endpointL = src[ offseti * 8 + 0 ];
    endpointR = src[ offseti * 8 + 1 ];

    startpointL += (LONG) (((LONGLONG) (endpointL - startpointL) * offsetf ) >> 32);
    startpointR += (LONG) (((LONGLONG) (endpointR - startpointR) * offsetf ) >> 32);

    if( StopAtZero &&
        ( ( lastpointL < 0 && startpointL >= 0 ) ||
          ( lastpointR < 0 && startpointR >= 0 ) ||
          ( lastpointL > 0 && startpointL <= 0 ) ||
          ( lastpointR > 0 && startpointR <= 0 ) ) )
    {
      break;
    }

    lastpointL = startpointL;
    lastpointR = startpointR;

    *dst++ += (LONG) ( ( (LONGLONG) ScaleLeft * startpointL ) >> 16 );
    *dst++ += (LONG) ( ( (LONGLONG) ScaleRight * startpointR ) >> 16 );

    offset += Add;
  }

  *StartPointLeft = endpointL >> 16;
  *StartPointRight = endpointR >> 16;

  *Dst    = dst;
  *Offset = offset;

  return i;
}

/*****************************************************************************/

/* Lofi mixing code */

LONG
AddLofiLongMono( ADDARGS )
{
  LONG    *src    = Src;
  WORD    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     lastpoint, sample;

  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    sample = src[ offseti ] >> 16;

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
AddLofiLongStereo( ADDARGS )
{
  LONG    *src    = Src;
  WORD    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     lastpoint, sample;

  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code
  
  for( i = 0; i < Samples; i++)
  {
    sample = src[ offseti ] >> 16;;

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
AddLofiLongsMono( ADDARGS )
{
  LONG    *src    = Src;
  WORD    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     lastpointL, lastpointR, sampleL, sampleR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    sampleL = src[ offseti * 2 + 0 ] >> 16;;
    sampleR = src[ offseti * 2 + 1 ] >> 16;;

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
AddLofiLongsStereo( ADDARGS )
{
  LONG    *src    = Src;
  WORD    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     lastpointL, lastpointR, sampleL, sampleR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    sampleL = src[ offseti * 2 + 0 ] >> 16;;
    sampleR = src[ offseti * 2 + 1 ] >> 16;;

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



#undef offsetf

/*****************************************************************************/

/* Backward mixing code */

#define offsetf ( (long) ( 32768 - ( (unsigned long) ( offset & 0xffffffffULL ) >> 17 ) ) )

LONG
AddLongMonoB( ADDARGS )
{
  LONG    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint = 0; // Make compiler happy
  LONG     lastpoint;

  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpoint = *StartPointLeft << 16;
    }
    else
    {
      startpoint = src[ offseti + 1 ];
    }

    endpoint = src[ offseti ];

    startpoint += (LONG) (((LONGLONG) (endpoint - startpoint) * offsetf ) >> 32);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += (LONG) ( ( (LONGLONG) ScaleLeft * startpoint ) >> 16 );

    offset -= Add;
  }

  *StartPointLeft = endpoint >> 16;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddLongStereoB( ADDARGS )
{
  LONG    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint = 0; // Make compiler happy
  LONG     lastpoint;

  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code
  
  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpoint = *StartPointLeft << 16;
    }
    else
    {
      startpoint = src[ offseti + 1 ];
    }

    endpoint = src[ offseti ];

    startpoint += (LONG) (((LONGLONG) (endpoint - startpoint) * offsetf ) >> 32);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += (LONG) ( ( (LONGLONG) ScaleLeft * startpoint ) >> 16 );
    *dst++ += (LONG) ( ( (LONGLONG) ScaleRight * startpoint ) >> 16 );

    offset -= Add;
  }

  *StartPointLeft = endpoint >> 16;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddLongsMonoB( ADDARGS )
{
  LONG    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL = 0, endpointR = 0; // Make compiler happy
  LONG     lastpointL, lastpointR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpointL = *StartPointLeft << 16;
      startpointR = *StartPointRight << 16;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 + 2 ];
      startpointR = src[ offseti * 2 + 1 + 2 ];
    }

    endpointL = src[ offseti * 2 + 0 ];
    endpointR = src[ offseti * 2 + 1 ];

    startpointL += (LONG) (((LONGLONG) (endpointL - startpointL) * offsetf ) >> 32);
    startpointR += (LONG) (((LONGLONG) (endpointR - startpointR) * offsetf ) >> 32);

    if( StopAtZero &&
        ( ( lastpointL < 0 && startpointL >= 0 ) ||
          ( lastpointR < 0 && startpointR >= 0 ) ||
          ( lastpointL > 0 && startpointL <= 0 ) ||
          ( lastpointR > 0 && startpointR <= 0 ) ) )
    {
      break;
    }

    lastpointL = startpointL;
    lastpointR = startpointR;

    *dst++ += (LONG) ( ( (LONGLONG) ScaleLeft * startpointL ) >> 16 ) +
      (LONG) ( ( (LONGLONG) ScaleRight * startpointR ) >> 16 );

    offset -= Add;
  }

  *StartPointLeft = endpointL >> 16;
  *StartPointRight = endpointR >> 16;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddLongsStereoB( ADDARGS )
{
  LONG    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL = 0, endpointR = 0; // Make compiler happy
  LONG     lastpointL, lastpointR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpointL = *StartPointLeft << 16;
      startpointR = *StartPointRight << 16;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 + 2 ];
      startpointR = src[ offseti * 2 + 1 + 2 ];
    }

    endpointL = src[ offseti * 2 + 0 ];
    endpointR = src[ offseti * 2 + 1 ];

    startpointL += (LONG) (((LONGLONG) (endpointL - startpointL) * offsetf ) >> 32);
    startpointR += (LONG) (((LONGLONG) (endpointR - startpointR) * offsetf ) >> 32);

    if( StopAtZero &&
        ( ( lastpointL < 0 && startpointL >= 0 ) ||
          ( lastpointR < 0 && startpointR >= 0 ) ||
          ( lastpointL > 0 && startpointL <= 0 ) ||
          ( lastpointR > 0 && startpointR <= 0 ) ) )
    {
      break;
    }

    lastpointL = startpointL;
    lastpointR = startpointR;

    *dst++ += (LONG) ( ( (LONGLONG) ScaleLeft * startpointL ) >> 16 );
    *dst++ += (LONG) ( ( (LONGLONG) ScaleRight * startpointR ) >> 16 );

    offset -= Add;
  }

  *StartPointLeft = endpointL >> 16;
  *StartPointRight = endpointR >> 16;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
Add71MonoB( ADDARGS )
{
  LONG    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL = 0, endpointR = 0; // Make compiler happy
  LONG     lastpointL, lastpointR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpointL = *StartPointLeft << 16;
      startpointR = *StartPointRight << 16;
    }
    else
    {
      startpointL = src[ offseti * 8 + 0 + 8 ];
      startpointR = src[ offseti * 8 + 1 + 8 ];
    }

    endpointL = src[ offseti * 8 + 0 ];
    endpointR = src[ offseti * 8 + 1 ];

    startpointL += (LONG) (((LONGLONG) (endpointL - startpointL) * offsetf ) >> 32);
    startpointR += (LONG) (((LONGLONG) (endpointR - startpointR) * offsetf ) >> 32);

    if( StopAtZero &&
        ( ( lastpointL < 0 && startpointL >= 0 ) ||
          ( lastpointR < 0 && startpointR >= 0 ) ||
          ( lastpointL > 0 && startpointL <= 0 ) ||
          ( lastpointR > 0 && startpointR <= 0 ) ) )
    {
      break;
    }

    lastpointL = startpointL;
    lastpointR = startpointR;

    *dst++ += (LONG) ( ( (LONGLONG) ScaleLeft * startpointL ) >> 16 ) +
      (LONG) ( ( (LONGLONG) ScaleRight * startpointR ) >> 16 );

    offset -= Add;
  }

  *StartPointLeft = endpointL >> 16;
  *StartPointRight = endpointR >> 16;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
Add71StereoB( ADDARGS )
{
  LONG    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL = 0, endpointR = 0; // Make compiler happy
  LONG     lastpointL, lastpointR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpointL = *StartPointLeft << 16;
      startpointR = *StartPointRight << 16;
    }
    else
    {
      startpointL = src[ offseti * 8 + 0 + 8 ];
      startpointR = src[ offseti * 8 + 1 + 8 ];
    }

    endpointL = src[ offseti * 8 + 0 ];
    endpointR = src[ offseti * 8 + 1 ];

    startpointL += (LONG) (((LONGLONG) (endpointL - startpointL) * offsetf ) >> 32);
    startpointR += (LONG) (((LONGLONG) (endpointR - startpointR) * offsetf ) >> 32);

    if( StopAtZero &&
        ( ( lastpointL < 0 && startpointL >= 0 ) ||
          ( lastpointR < 0 && startpointR >= 0 ) ||
          ( lastpointL > 0 && startpointL <= 0 ) ||
          ( lastpointR > 0 && startpointR <= 0 ) ) )
    {
      break;
    }

    lastpointL = startpointL;
    lastpointR = startpointR;

    *dst++ += (LONG) ( ( (LONGLONG) ScaleLeft * startpointL ) >> 16 );
    *dst++ += (LONG) ( ( (LONGLONG) ScaleRight * startpointR ) >> 16 );

    offset -= Add;
  }

  *StartPointLeft = endpointL >> 16;
  *StartPointRight = endpointR >> 16;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


/*****************************************************************************/

/* Lofi mixing code */

LONG
AddLofiLongMonoB( ADDARGS )
{
  LONG    *src    = Src;
  WORD    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     lastpoint, sample;

  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    sample = src[ offseti ] >> 16;;

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
AddLofiLongStereoB( ADDARGS )
{
  LONG    *src    = Src;
  WORD    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     lastpoint, sample;

  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code
  
  for( i = 0; i < Samples; i++)
  {
    sample = src[ offseti ] >> 16;;

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
AddLofiLongsMonoB( ADDARGS )
{
  LONG    *src    = Src;
  WORD    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     lastpointL, lastpointR, sampleL, sampleR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    sampleL = src[ offseti * 2 + 0 ] >> 16;;
    sampleR = src[ offseti * 2 + 1 ] >> 16;;

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
AddLofiLongsStereoB( ADDARGS )
{
  LONG    *src    = Src;
  WORD    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     lastpointL, lastpointR, sampleL, sampleR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    sampleL = src[ offseti * 2 + 0 ] >> 16;;
    sampleR = src[ offseti * 2 + 1 ] >> 16;;

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


#undef offseti
#undef offsetf

/*****************************************************************************/
