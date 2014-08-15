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

#ifndef _VERSION_H_
#define _VERSION_H_

// transforms a define into a string
#define STR(x)  STR2(x)
#define STR2(x) #x

// for setting all necessary version information
#define EXE_VERSION    2
#define EXE_REVISION   15
#define EXE_DATE       "04.04.2014"
#define EXE_COPYRIGHT  "Copyright (C) 2005-2014 FlexCat Open Source Team"

// set the EXE_REV_STRING
#define EXE_REV_STRING STR(EXE_VERSION) "." STR(EXE_REVISION)

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
#elif defined(__linux__)
  #define SYSTEM      "Linux"
  #define SYSTEMSHORT "linux"
#elif defined(_WIN32)
  #define SYSTEM      "Windows"
  #define SYSTEMSHORT "WIN"
#elif defined(__APPLE__) && defined(__MACH__)
  #define SYSTEM      "MacOSX"
  #define SYSTEMSHORT "OSX"
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
#else
  #warning "Unsupported CPU model - check CPU define"
  #define CPU "???"
#endif

#endif // _VERSION_H_
