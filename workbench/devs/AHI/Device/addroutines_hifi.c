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

#define offsetf ( (long) ( (unsigned long) ( offset & 0xffffffffULL ) >> 17) )

LONG
AddByteMono( ADDARGS )
{
  BYTE    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint = 0; // Make compiler happy
  LONG     lastpoint;

  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpoint = *StartPointLeft;
    }
    else
    {
      startpoint = src[ offseti - 1 ] << 8;
    }

    endpoint = src[ offseti ] << 8;

    startpoint += (((endpoint - startpoint) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += ScaleLeft * startpoint;

    offset += Add;
  }

  *StartPointLeft = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddByteStereo( ADDARGS )
{
  BYTE    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint = 0; // Make compiler happy
  LONG     lastpoint;

  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code
  
  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpoint = *StartPointLeft;
    }
    else
    {
      startpoint = src[ offseti - 1 ] << 8;
    }

    endpoint = src[ offseti ] << 8;

    startpoint += (((endpoint - startpoint) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += ScaleLeft * startpoint;
    *dst++ += ScaleRight * startpoint;

    offset += Add;
  }

  *StartPointLeft = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddBytesMono( ADDARGS )
{
  BYTE    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL = 0, endpointR = 0; // Make compiler happy
  LONG     lastpointL, lastpointR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpointL = *StartPointLeft;
      startpointR = *StartPointRight;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 - 2 ] << 8;
      startpointR = src[ offseti * 2 + 1 - 2 ] << 8;
    }

    endpointL = src[ offseti * 2 + 0 ] << 8;
    endpointR = src[ offseti * 2 + 1 ] << 8;

    startpointL += (((endpointL - startpointL) * offsetf ) >> 15);
    startpointR += (((endpointR - startpointR) * offsetf ) >> 15);

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

    *dst++ += ScaleLeft * startpointL + ScaleRight * startpointR;

    offset += Add;
  }

  *StartPointLeft = endpointL;
  *StartPointRight = endpointR;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddBytesStereo( ADDARGS )
{
  BYTE    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL = 0, endpointR = 0; // Make compiler happy
  LONG     lastpointL, lastpointR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpointL = *StartPointLeft;
      startpointR = *StartPointRight;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 - 2 ] << 8;
      startpointR = src[ offseti * 2 + 1 - 2 ] << 8;
    }

    endpointL = src[ offseti * 2 + 0 ] << 8;
    endpointR = src[ offseti * 2 + 1 ] << 8;

    startpointL += (((endpointL - startpointL) * offsetf ) >> 15);
    startpointR += (((endpointR - startpointR) * offsetf ) >> 15);

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

    *dst++ += ScaleLeft * startpointL;
    *dst++ += ScaleRight * startpointR;

    offset += Add;
  }

  *StartPointLeft = endpointL;
  *StartPointRight = endpointR;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddWordMono( ADDARGS )
{
  WORD    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint = 0; // Make compiler happy
  LONG     lastpoint;

  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpoint = *StartPointLeft;
    }
    else
    {
      startpoint = src[ offseti - 1 ];
    }

    endpoint = src[ offseti ];

    startpoint += (((endpoint - startpoint) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += ScaleLeft * startpoint;

    offset += Add;
  }

  *StartPointLeft = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddWordStereo( ADDARGS )
{
  WORD    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint = 0; // Make compiler happy
  LONG     lastpoint;

  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code
  
  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpoint = *StartPointLeft;
    }
    else
    {
      startpoint = src[ offseti - 1 ];
    }

    endpoint = src[ offseti ];

    startpoint += (((endpoint - startpoint) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += ScaleLeft * startpoint;
    *dst++ += ScaleRight * startpoint;

    offset += Add;
  }

  *StartPointLeft = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}

LONG
AddWordsMono( ADDARGS )
{
  WORD    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL = 0, endpointR = 0; // Make compiler happy
  LONG     lastpointL, lastpointR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpointL = *StartPointLeft;
      startpointR = *StartPointRight;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 - 2 ];
      startpointR = src[ offseti * 2 + 1 - 2 ];
    }

    endpointL = src[ offseti * 2 + 0 ];
    endpointR = src[ offseti * 2 + 1 ];

    startpointL += (((endpointL - startpointL) * offsetf ) >> 15);
    startpointR += (((endpointR - startpointR) * offsetf ) >> 15);

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

    *dst++ += ScaleLeft * startpointL + ScaleRight * startpointR;

    offset += Add;
  }

  *StartPointLeft = endpointL;
  *StartPointRight = endpointR;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddWordsStereo( ADDARGS )
{
  WORD    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL = 0, endpointR = 0; // Make compiler happy
  LONG     lastpointL, lastpointR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpointL = *StartPointLeft;
      startpointR = *StartPointRight;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 - 2 ];
      startpointR = src[ offseti * 2 + 1 - 2 ];
    }

    endpointL = src[ offseti * 2 + 0 ];
    endpointR = src[ offseti * 2 + 1 ];

    startpointL += (((endpointL - startpointL) * offsetf ) >> 15);
    startpointR += (((endpointR - startpointR) * offsetf ) >> 15);

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

    *dst++ += ScaleLeft * startpointL;
    *dst++ += ScaleRight * startpointR;

    offset += Add;
  }

  *StartPointLeft = endpointL;
  *StartPointRight = endpointR;

  *Dst    = dst;
  *Offset = offset;

  return i;
}

#undef offsetf

/*****************************************************************************/

/* Backward mixing code */

#define offsetf ( (long) ( 32768 - ( (unsigned long) ( offset & 0xffffffffULL ) >> 17 ) ) )

LONG
AddByteMonoB( ADDARGS )
{
  BYTE    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint = 0; // Make compiler happy
  LONG     lastpoint;

  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpoint = *StartPointLeft;
    }
    else
    {
      startpoint = src[ offseti + 1 ] << 8;
    }

    endpoint = src[ offseti ] << 8;

    startpoint += (((endpoint - startpoint) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += ScaleLeft * startpoint;

    offset -= Add;
  }

  *StartPointLeft = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddByteStereoB( ADDARGS )
{
  BYTE    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint = 0; // Make compiler happy
  LONG     lastpoint;

  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code
  
  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpoint = *StartPointLeft;
    }
    else
    {
      startpoint = src[ offseti + 1 ] << 8;
    }

    endpoint = src[ offseti ] << 8;

    startpoint += (((endpoint - startpoint) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += ScaleLeft * startpoint;
    *dst++ += ScaleRight * startpoint;

    offset -= Add;
  }

  *StartPointLeft = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddBytesMonoB( ADDARGS )
{
  BYTE    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL = 0, endpointR = 0; // Make compiler happy
  LONG     lastpointL, lastpointR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpointL = *StartPointLeft;
      startpointR = *StartPointRight;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 + 2 ] << 8;
      startpointR = src[ offseti * 2 + 1 + 2 ] << 8;
    }

    endpointL = src[ offseti * 2 + 0 ] << 8;
    endpointR = src[ offseti * 2 + 1 ] << 8;

    startpointL += (((endpointL - startpointL) * offsetf ) >> 15);
    startpointR += (((endpointR - startpointR) * offsetf ) >> 15);

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

    *dst++ += ScaleLeft * startpointL + ScaleRight * startpointR;

    offset -= Add;
  }

  *StartPointLeft = endpointL;
  *StartPointRight = endpointR;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddBytesStereoB( ADDARGS )
{
  BYTE    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL = 0, endpointR = 0; // Make compiler happy
  LONG     lastpointL, lastpointR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpointL = *StartPointLeft;
      startpointR = *StartPointRight;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 + 2 ] << 8;
      startpointR = src[ offseti * 2 + 1 + 2 ] << 8;
    }

    endpointL = src[ offseti * 2 + 0 ] << 8;
    endpointR = src[ offseti * 2 + 1 ] << 8;

    startpointL += (((endpointL - startpointL) * offsetf ) >> 15);
    startpointR += (((endpointR - startpointR) * offsetf ) >> 15);

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

    *dst++ += ScaleLeft * startpointL;
    *dst++ += ScaleRight * startpointR;

    offset -= Add;
  }

  *StartPointLeft = endpointL;
  *StartPointRight = endpointR;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddWordMonoB( ADDARGS )
{
  WORD    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint = 0; // Make compiler happy
  LONG     lastpoint;

  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpoint = *StartPointLeft;
    }
    else
    {
      startpoint = src[ offseti + 1 ];
    }

    endpoint = src[ offseti ];

    startpoint += (((endpoint - startpoint) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += ScaleLeft * startpoint;

    offset -= Add;
  }

  *StartPointLeft = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddWordStereoB( ADDARGS )
{
  WORD    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint = 0; // Make compiler happy
  LONG     lastpoint;

  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code
  
  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpoint = *StartPointLeft;
    }
    else
    {
      startpoint = src[ offseti + 1 ];
    }

    endpoint = src[ offseti ];

    startpoint += (((endpoint - startpoint) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += ScaleLeft * startpoint;
    *dst++ += ScaleRight * startpoint;

    offset -= Add;
  }

  *StartPointLeft = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddWordsMonoB( ADDARGS )
{
  WORD    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL = 0, endpointR = 0; // Make compiler happy
  LONG     lastpointL, lastpointR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpointL = *StartPointLeft;
      startpointR = *StartPointRight;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 + 2 ];
      startpointR = src[ offseti * 2 + 1 + 2 ];
    }

    endpointL = src[ offseti * 2 + 0 ];
    endpointR = src[ offseti * 2 + 1 ];

    startpointL += (((endpointL - startpointL) * offsetf ) >> 15);
    startpointR += (((endpointR - startpointR) * offsetf ) >> 15);

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

    *dst++ += ScaleLeft * startpointL + ScaleRight * startpointR;

    offset -= Add;
  }

  *StartPointLeft = endpointL;
  *StartPointRight = endpointR;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddWordsStereoB( ADDARGS )
{
  WORD    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL = 0, endpointR = 0; // Make compiler happy
  LONG     lastpointL, lastpointR;

  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpointL = *StartPointLeft;
      startpointR = *StartPointRight;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 + 2 ];
      startpointR = src[ offseti * 2 + 1 + 2 ];
    }

    endpointL = src[ offseti * 2 + 0 ];
    endpointR = src[ offseti * 2 + 1 ];

    startpointL += (((endpointL - startpointL) * offsetf ) >> 15);
    startpointR += (((endpointR - startpointR) * offsetf ) >> 15);

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

    *dst++ += ScaleLeft * startpointL;
    *dst++ += ScaleRight * startpointR;

    offset -= Add;
  }

  *StartPointLeft = endpointL;
  *StartPointRight = endpointR;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


#undef offseti
#undef offsetf

/*****************************************************************************/
