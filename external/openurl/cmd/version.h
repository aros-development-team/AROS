/***************************************************************************

 openurl.library - universal URL display and browser launcher library
 Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
 Copyright (C) 2005-2009 by openurl.library Open Source Team

 This library is free software; it has been placed in the public domain
 and you can freely redistribute it and/or modify it. Please note, however,
 that some components may be under the LGPL or GPL license.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 openurl.library project: http://sourceforge.net/projects/openurllib/

 $Id$

***************************************************************************/

#ifndef _VERSION_H_
#define _VERSION_H_

#include "macros.h"

// for setting all necessary version information
#define LIB_VERSION    7
#define LIB_REVISION   12
#define LIB_DATE       "20.12.2012"
#define LIB_COPYRIGHT  "Copyright (c) 2005-2012 openurl.library Open Source Team"

// set the LIB_REV_STRING
#define LIB_REV_STRING STR(LIB_VERSION) "." STR(LIB_REVISION)

// identify the system we are compiling for
#if defined(__amigaos4__)
  #define SYSTEM      "AmigaOS4"
  #define SYSTEMSHORT "OS4"
#elif defined(__MORPHOS__)
  #define SYSTEM      "MorphOS"
  #define SYSTEMSHORT "MOS"
#elif defined(__AROS__)
  #define SYSTEM      "AROS"
  #define SYSTEMSHORT SYSTEM
#elif defined(__AMIGA__)
  #define SYSTEM      "AmigaOS3"
  #define SYSTEMSHORT "OS3"
#else
  #warning "Unsupported System - check SYSTEM define"
  #define SYSTEM      "???"
  #define SYSTEMSHORT "???"
#endif

// identify the CPU model
#if defined(__PPC__) || defined(__powerpc__)
  #define CPU "PPC"
#elif defined(_M68060) || defined(__M68060) || defined(__mc68060)
  #define CPU "m68060"
#elif defined(_M68040) || defined(__M68040) || defined(__mc68040)
  #define CPU "m68040"
#elif defined(_M68030) || defined(__M68030) || defined(__mc68030)
  #define CPU "m68030"
#elif defined(_M68020) || defined(__M68020) || defined(__mc68020)
  #define CPU "m68k"
#elif defined(_M68000) || defined(__M68000) || defined(__mc68000)
  #define CPU "m68000"
#elif defined(__i386__)
  #define CPU "x86"
#elif defined(__x86_64__)
  #define CPU "x86_64"
#elif defined(__arm__)
  #define CPU "ARM"
#else
  #warning "Unsupported CPU model - check CPU define"
  #define CPU "???"
#endif

#endif // _VERSION_H_
