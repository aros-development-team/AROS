/***************************************
  $Header$

  Target machine header file tm.h.
  ******************/ /******************
  Written by Andrew M. Bishop

  This file consists of parts taken from GNU CC.

  GNU CC is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.
  ***************************************/


#ifndef TM_H
#define TM_H    /*+ To stop multiple inclusions. +*/

/* The configure script output */

#include "autoconfig.h"


/* Number of bits in an addressable storage unit */

#define BITS_PER_UNIT 8


/* This describes the machine the compiler is hosted on.  */

#define HOST_BITS_PER_INT (SIZEOF_INT*BITS_PER_UNIT)

#define HOST_BITS_PER_LONG (SIZEOF_LONG*BITS_PER_UNIT)


#define BITS_PER_WORD (SIZEOF_LONG*BITS_PER_UNIT)


/* Define results of standard character escape sequences.  */

#if defined(mvs)
#define TARGET_BELL	47
#define TARGET_BS	22
#define TARGET_TAB	5
#define TARGET_NEWLINE	21
#define TARGET_VT	11
#define TARGET_FF	12
#define TARGET_CR	13
#else
#define TARGET_BELL    007
#define TARGET_BS      010
#define TARGET_TAB     011
#define TARGET_NEWLINE 012
#define TARGET_VT      013
#define TARGET_FF      014
#define TARGET_CR      015
#endif


#endif /* TM_H */
