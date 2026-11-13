/***************************************
  $Header$

  System configuration header file config.h.
  ******************/ /******************
  Written by Andrew M. Bishop

  This file consists of parts taken from GNU CC.

  GNU CC is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.
  ***************************************/


#ifndef CONFIG_H
#define CONFIG_H    /*+ To stop multiple inclusions. +*/

/* The configure script output */

#include "autoconfig.h"


/* Target machine dependencies. */

#include "tm.h"


/* Need this for AIX apparently. */

#if defined(_AIX)
#pragma alloca
#endif


/* Use System V memory functions (if needed). */

#if !defined(HAVE_BCMP)

#define bcmp(a,b,c)  memcmp(a,b,c)
#define bcopy(a,b,c) memcpy(b,a,c)
#define bzero(a,b)   memset(a,0,b)

#define index  strchr
#define rindex strrchr

#endif


/* Exit codes. */

#ifndef FATAL_EXIT_CODE
#define FATAL_EXIT_CODE 33	/* gnu cc command understands this */
#endif

#ifndef SUCCESS_EXIT_CODE
#define SUCCESS_EXIT_CODE 0	/* 0 means success on Unix.  */
#endif

#endif /* CONFIG_H */
