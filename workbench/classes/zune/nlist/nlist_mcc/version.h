/***************************************************************************

 NList.mcc - New List MUI Custom Class
 Registered MUI class, Serial Number: 1d51 0x9d510030 to 0x9d5100A0
                                           0x9d5100C0 to 0x9d5100FF

 Copyright (C) 1996-2001 by Gilles Masson
 Copyright (C) 2001-2013 by NList Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 NList classes Support Site:  http://www.sf.net/projects/nlist-classes

 $Id$

***************************************************************************/

#ifndef _VERSION_H_
#define _VERSION_H_

// transforms a define into a string
#define STR(x)  STR2(x)
#define STR2(x) #x

// for setting all necessary version information
#define LIB_VERSION    20
#define LIB_REVISION   135
#define LIB_DATE       "25.04.2013"
#define LIB_COPYRIGHT  "Copyright (C) 2001-2013 NList Open Source Team"

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
#else
  #warning "Unsupported CPU model - check CPU define"
  #define CPU "???"
#endif

#endif // _VERSION_H_
