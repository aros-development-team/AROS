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
** LONG	    *StartPoints
** APTR      Unused
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
AddByte71( ADDARGS )
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

    dst[ 0 ] += ScaleLeft * startpoint;
    dst[ 1 ] += ScaleRight * startpoint;
    dst += 8;

    offset += Add;
  }

  *StartPointLeft = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}

LONG
AddBytes71( ADDARGS )
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

    dst[ 0 ] += ScaleLeft * startpointL;
    dst[ 1 ] += ScaleRight * startpointR;
    dst += 8;

    offset += Add;
  }

  *StartPointLeft = endpointL;
  *StartPointRight = endpointR;

  *Dst    = dst;
  *Offset = offset;

  return i;
}
LONG
AddWord71( ADDARGS )
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

    dst[ 0 ] += ScaleLeft * startpoint;
    dst[ 1 ] += ScaleRight * startpoint;
    dst += 8;

    offset += Add;
  }

  *StartPointLeft = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddWords71( ADDARGS )
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

    dst[ 0 ] += ScaleLeft * startpointL;
    dst[ 1 ] += ScaleRight * startpointR;
    dst += 8;

    offset += Add;
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

#define offseti ( (long) ( offset >> 32 ) )

#define offsetf ( offset & 0xffffffffULL )

LONG
AddLong71( ADDARGS )
{
  LONG    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint = 0; // Make compiler happy
  LONG     lastpoint;
  LONG     scale_mono = ( ScaleLeft + ScaleRight ) / 2;

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

    dst[ 0 ] += (LONG) ( ( (LONGLONG) ScaleLeft * startpoint ) >> 16 );
    dst[ 1 ] += (LONG) ( ( (LONGLONG) ScaleRight * startpoint ) >> 16 );
    dst += 8;

    offset += Add;
  }

  *StartPointLeft = endpoint >> 16;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddLongs71( ADDARGS )
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

    dst[ 0 ] += (LONG) ( ( (LONGLONG) ScaleLeft * startpointL ) >> 16 );
    dst[ 1 ] += (LONG) ( ( (LONGLONG) ScaleRight * startpointR ) >> 16 );
    dst += 8;

    offset += Add;
  }

  *StartPointLeft = endpointL >> 16;
  *StartPointRight = endpointR >> 16;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
Add7171( ADDARGS71 )
{
  LONG    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR;
  LONG     startpointRL, startpointRR;
  LONG     startpointSL, startpointSR;
  LONG     startpointC, startpointLFE;
  LONG     endpointL  = 0, endpointR   = 0; // Make compiler happy
  LONG     endpointRL = 0, endpointRR  = 0; // Make compiler happy
  LONG     endpointSL = 0, endpointSR  = 0; // Make compiler happy
  LONG     endpointC  = 0, endpointLFE = 0; // Make compiler happy
  LONG     lastpointL,  lastpointR;
  LONG     lastpointRL, lastpointRR;
  LONG     lastpointSL, lastpointSR;
  LONG     lastpointC,  lastpointLFE;
  LONG     scale_mono = ( ScaleLeft + ScaleRight ) / 2;

  lastpointL  = lastpointR   = 0;        // 0 doesn't affect the StopAtZero code
  lastpointRL = lastpointRR  = 0;        // 0 doesn't affect the StopAtZero code
  lastpointSL = lastpointSR  = 0;        // 0 doesn't affect the StopAtZero code
  lastpointC  = lastpointLFE = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpointL   = StartPoints[ CD_L   ] << 16;
      startpointR   = StartPoints[ CD_R   ] << 16;
      startpointRL  = StartPoints[ CD_RL  ] << 16;
      startpointRR  = StartPoints[ CD_RR  ] << 16;
      startpointSL  = StartPoints[ CD_SL  ] << 16;
      startpointSR  = StartPoints[ CD_SR  ] << 16;
      startpointC   = StartPoints[ CD_C   ] << 16;
      startpointLFE = StartPoints[ CD_LFE ] << 16;
    }
    else
    {
      startpointL   = src[ offseti * 8 + 0 - 8 ];
      startpointR   = src[ offseti * 8 + 1 - 8 ];
      startpointRL  = src[ offseti * 8 + 2 - 8 ];
      startpointRR  = src[ offseti * 8 + 3 - 8 ];
      startpointSL  = src[ offseti * 8 + 4 - 8 ];
      startpointSR  = src[ offseti * 8 + 5 - 8 ];
      startpointC   = src[ offseti * 8 + 6 - 8 ];
      startpointLFE = src[ offseti * 8 + 7 - 8 ];
    }

    endpointL   = src[ offseti * 8 + 0 ];
    endpointR   = src[ offseti * 8 + 1 ];
    endpointRL  = src[ offseti * 8 + 2 ];
    endpointRR  = src[ offseti * 8 + 3 ];
    endpointSL  = src[ offseti * 8 + 4 ];
    endpointSR  = src[ offseti * 8 + 5 ];
    endpointC   = src[ offseti * 8 + 6 ];
    endpointLFE = src[ offseti * 8 + 7 ];

    startpointL   += (LONG) (((LONGLONG) (endpointL   - startpointL)   * offsetf ) >> 32);
    startpointR   += (LONG) (((LONGLONG) (endpointR   - startpointR)   * offsetf ) >> 32);
    startpointRL  += (LONG) (((LONGLONG) (endpointRL  - startpointRL)  * offsetf ) >> 32);
    startpointRR  += (LONG) (((LONGLONG) (endpointRR  - startpointRR)  * offsetf ) >> 32);
    startpointSL  += (LONG) (((LONGLONG) (endpointSL  - startpointSL)  * offsetf ) >> 32);
    startpointSR  += (LONG) (((LONGLONG) (endpointSR  - startpointSR)  * offsetf ) >> 32);
    startpointC   += (LONG) (((LONGLONG) (endpointC   - startpointC)   * offsetf ) >> 32);
    startpointLFE += (LONG) (((LONGLONG) (endpointLFE - startpointLFE) * offsetf ) >> 32);

    if( StopAtZero &&
        ( ( lastpointL < 0 && startpointL >= 0 ) ||
          ( lastpointR < 0 && startpointR >= 0 ) ||
          ( lastpointL > 0 && startpointL <= 0 ) ||
          ( lastpointR > 0 && startpointR <= 0 ) ) )
    {
      break;
    }

    lastpointL   = startpointL;
    lastpointR   = startpointR;
    lastpointRL  = startpointRL;
    lastpointRR  = startpointRR;
    lastpointSL  = startpointSL;
    lastpointSR  = startpointSR;
    lastpointC   = startpointC;
    lastpointLFE = startpointLFE;

    *dst++ += (LONG) ( ( (LONGLONG) ScaleLeft  * startpointL   ) >> 16 );
    *dst++ += (LONG) ( ( (LONGLONG) ScaleRight * startpointR   ) >> 16 );
    *dst++ += (LONG) ( ( (LONGLONG) ScaleLeft  * startpointRL  ) >> 16 );
    *dst++ += (LONG) ( ( (LONGLONG) ScaleRight * startpointRR  ) >> 16 );
    *dst++ += (LONG) ( ( (LONGLONG) ScaleLeft  * startpointSL  ) >> 16 );
    *dst++ += (LONG) ( ( (LONGLONG) ScaleRight * startpointSR  ) >> 16 );
    *dst++ += (LONG) ( ( (LONGLONG) scale_mono * startpointC   ) >> 16 );
    *dst++ += (LONG) ( ( (LONGLONG) scale_mono * startpointLFE ) >> 16 );

    offset += Add;
  }

  StartPoints[ CD_L   ] = endpointL >> 16;
  StartPoints[ CD_R   ] = endpointR >> 16;
  StartPoints[ CD_RL  ] = endpointRL >> 16;
  StartPoints[ CD_RR  ] = endpointRR >> 16;
  StartPoints[ CD_SL  ] = endpointSL >> 16;
  StartPoints[ CD_SR  ] = endpointSR >> 16;
  StartPoints[ CD_C   ] = endpointC >> 16;
  StartPoints[ CD_LFE ] = endpointLFE >> 16;

  *Dst    = dst;
  *Offset = offset;

  return i;
}

#undef offseti
#undef offsetf


/*****************************************************************************/

/* Backward mixing code */

#define offseti ( (long) ( offset >> 32 ) )

#define offsetf ( (long) ( 32768 - ( (unsigned long) ( offset & 0xffffffffULL ) >> 17 ) ) )

LONG
AddByte71B( ADDARGS )
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

    dst[ 0 ] += ScaleLeft * startpoint;
    dst[ 1 ] += ScaleRight * startpoint;
    dst += 8;

    offset -= Add;
  }

  *StartPointLeft = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}

LONG
AddBytes71B( ADDARGS )
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

    dst[ 0 ] += ScaleLeft * startpointL;
    dst[ 1 ] += ScaleRight * startpointR;
    dst += 8;

    offset -= Add;
  }

  *StartPointLeft = endpointL;
  *StartPointRight = endpointR;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddWord71B( ADDARGS )
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

    dst[ 0 ] += ScaleLeft * startpoint;
    dst[ 1 ] += ScaleRight * startpoint;
    dst += 8;

    offset -= Add;
  }

  *StartPointLeft = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddWords71B( ADDARGS )
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

    dst[ 0 ] += ScaleLeft * startpointL;
    dst[ 1 ] += ScaleRight * startpointR;
    dst += 8;

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

#define offseti ( (long) ( offset >> 32 ) )

#define offsetf ( (long) ( 32768 - ( (unsigned long) ( offset & 0xffffffffULL ) >> 17 ) ) )

LONG
AddLong71B( ADDARGS )
{
  LONG    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint = 0; // Make compiler happy
  LONG     lastpoint;
  LONG     scale_mono = ( ScaleLeft + ScaleRight ) / 2;

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

    dst[ 0 ] += (LONG) ( ( (LONGLONG) ScaleLeft * startpoint ) >> 16 );
    dst[ 1 ] += (LONG) ( ( (LONGLONG) ScaleRight * startpoint ) >> 16 );
    dst += 8;

    offset -= Add;
  }

  *StartPointLeft = endpoint >> 16;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddLongs71B( ADDARGS )
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

    dst[ 0 ] += (LONG) ( ( (LONGLONG) ScaleLeft * startpointL ) >> 16 );
    dst[ 1 ] += (LONG) ( ( (LONGLONG) ScaleRight * startpointR ) >> 16 );
    dst += 8;

    offset -= Add;
  }

  *StartPointLeft = endpointL >> 16;
  *StartPointRight = endpointR >> 16;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
Add7171B( ADDARGS71 )
{
  LONG    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR;
  LONG     startpointRL, startpointRR;
  LONG     startpointSL, startpointSR;
  LONG     startpointC, startpointLFE;
  LONG     endpointL  = 0, endpointR   = 0; // Make compiler happy
  LONG     endpointRL = 0, endpointRR  = 0; // Make compiler happy
  LONG     endpointSL = 0, endpointSR  = 0; // Make compiler happy
  LONG     endpointC  = 0, endpointLFE = 0; // Make compiler happy
  LONG     lastpointL,  lastpointR;
  LONG     lastpointRL, lastpointRR;
  LONG     lastpointSL, lastpointSR;
  LONG     lastpointC,  lastpointLFE;
  LONG     scale_mono = ( ScaleLeft + ScaleRight ) / 2;

  lastpointL  = lastpointR   = 0;        // 0 doesn't affect the StopAtZero code
  lastpointRL = lastpointRR  = 0;        // 0 doesn't affect the StopAtZero code
  lastpointSL = lastpointSR  = 0;        // 0 doesn't affect the StopAtZero code
  lastpointC  = lastpointLFE = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == FirstOffsetI ) {
      startpointL   = StartPoints[ CD_L   ] << 16;
      startpointR   = StartPoints[ CD_R   ] << 16;
      startpointRL  = StartPoints[ CD_RL  ] << 16;
      startpointRR  = StartPoints[ CD_RR  ] << 16;
      startpointSL  = StartPoints[ CD_SL  ] << 16;
      startpointSR  = StartPoints[ CD_SR  ] << 16;
      startpointC   = StartPoints[ CD_C   ] << 16;
      startpointLFE = StartPoints[ CD_LFE ] << 16;
    }
    else
    {
      startpointL   = src[ offseti * 8 + 0 + 8 ];
      startpointR   = src[ offseti * 8 + 1 + 8 ];
      startpointRL  = src[ offseti * 8 + 2 + 8 ];
      startpointRR  = src[ offseti * 8 + 3 + 8 ];
      startpointSL  = src[ offseti * 8 + 4 + 8 ];
      startpointSR  = src[ offseti * 8 + 5 + 8 ];
      startpointC   = src[ offseti * 8 + 6 + 8 ];
      startpointLFE = src[ offseti * 8 + 7 + 8 ];
    }

    endpointL   = src[ offseti * 8 + 0 ];
    endpointR   = src[ offseti * 8 + 1 ];
    endpointRL  = src[ offseti * 8 + 2 ];
    endpointRR  = src[ offseti * 8 + 3 ];
    endpointSL  = src[ offseti * 8 + 4 ];
    endpointSR  = src[ offseti * 8 + 5 ];
    endpointC   = src[ offseti * 8 + 6 ];
    endpointLFE = src[ offseti * 8 + 7 ];

    startpointL   += (LONG) (((LONGLONG) (endpointL   - startpointL)   * offsetf ) >> 32);
    startpointR   += (LONG) (((LONGLONG) (endpointR   - startpointR)   * offsetf ) >> 32);
    startpointRL  += (LONG) (((LONGLONG) (endpointRL  - startpointRL)  * offsetf ) >> 32);
    startpointRR  += (LONG) (((LONGLONG) (endpointRR  - startpointRR)  * offsetf ) >> 32);
    startpointSL  += (LONG) (((LONGLONG) (endpointSL  - startpointSL)  * offsetf ) >> 32);
    startpointSR  += (LONG) (((LONGLONG) (endpointSR  - startpointSR)  * offsetf ) >> 32);
    startpointC   += (LONG) (((LONGLONG) (endpointC   - startpointC)   * offsetf ) >> 32);
    startpointLFE += (LONG) (((LONGLONG) (endpointLFE - startpointLFE) * offsetf ) >> 32);

    if( StopAtZero &&
        ( ( lastpointL < 0 && startpointL >= 0 ) ||
          ( lastpointR < 0 && startpointR >= 0 ) ||
          ( lastpointL > 0 && startpointL <= 0 ) ||
          ( lastpointR > 0 && startpointR <= 0 ) ) )
    {
      break;
    }

    lastpointL   = startpointL;
    lastpointR   = startpointR;
    lastpointRL  = startpointRL;
    lastpointRR  = startpointRR;
    lastpointSL  = startpointSL;
    lastpointSR  = startpointSR;
    lastpointC   = startpointC;
    lastpointLFE = startpointLFE;

    *dst++ += (LONG) ( ( (LONGLONG) ScaleLeft  * startpointL   ) >> 16 );
    *dst++ += (LONG) ( ( (LONGLONG) ScaleRight * startpointR   ) >> 16 );
    *dst++ += (LONG) ( ( (LONGLONG) ScaleLeft  * startpointRL  ) >> 16 );
    *dst++ += (LONG) ( ( (LONGLONG) ScaleRight * startpointRR  ) >> 16 );
    *dst++ += (LONG) ( ( (LONGLONG) ScaleLeft  * startpointSL  ) >> 16 );
    *dst++ += (LONG) ( ( (LONGLONG) ScaleRight * startpointSR  ) >> 16 );
    *dst++ += (LONG) ( ( (LONGLONG) scale_mono * startpointC   ) >> 16 );
    *dst++ += (LONG) ( ( (LONGLONG) scale_mono * startpointLFE ) >> 16 );

    offset -= Add;
  }

  StartPoints[ CD_L   ] = endpointL >> 16;
  StartPoints[ CD_R   ] = endpointR >> 16;
  StartPoints[ CD_RL  ] = endpointRL >> 16;
  StartPoints[ CD_RR  ] = endpointRR >> 16;
  StartPoints[ CD_SL  ] = endpointSL >> 16;
  StartPoints[ CD_SR  ] = endpointSR >> 16;
  StartPoints[ CD_C   ] = endpointC >> 16;
  StartPoints[ CD_LFE ] = endpointLFE >> 16;

  *Dst    = dst;
  *Offset = offset;

  return i;
}

#undef offseti
#undef offsetf
