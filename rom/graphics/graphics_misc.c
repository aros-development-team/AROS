/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Code for miscellaneous operations needed bz graphics
    Lang: english
*/

#include <exec/types.h>

int CalcHashIndex(ULONG n)
{
  UBYTE Index = (n        & 0xff) +
               ((n >>  8) & 0xff) +
               ((n >> 16) & 0xff) +
               ((n >> 24) & 0xff);
  Index &=0x07;
  return Index; 
}
