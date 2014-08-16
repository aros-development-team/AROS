/***************************************************************************

 codesets.library - Amiga shared library for handling different codesets
 Copyright (C) 2001-2005 by Alfonso [alfie] Ranieri <alforan@tin.it>.
 Copyright (C) 2005-2014 codesets.library Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 codesets.library project: http://sourceforge.net/projects/codesetslib/

 $Id$

***************************************************************************/

#ifndef BASE_H
#define BASE_H 1

#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif

#ifndef EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif

/***************************************************************************/

struct LibraryHeader
{
  struct Library          libBase;
  struct Library          *sysBase;
  BPTR                    segList;
  struct SignalSemaphore  libSem;
  APTR                    pool;
  struct SignalSemaphore  poolSem;
  ULONG                   flags;
  struct codesetList      codesets;       // list with all internal codesets.
  struct codeset          *systemCodeset; // ptr to the system's default codeset
  struct codeset          *utf8Codeset;   // ptr to the fake utf8 codeset
  struct codeset	      *utf16Codeset;  // ptr to the fake utf16 codeset
  struct codeset          *utf32Codeset;
};

#define __NOLIBBASE__
#include <proto/codesets.h>

/***************************************************************************/

extern struct LibraryHeader *CodesetsBase;

#if defined(__amigaos4__)
#define __BASE_OR_IFACE_TYPE	struct CodesetsIFace *
#define __BASE_OR_IFACE_VAR		ICodesets
#else
#define __BASE_OR_IFACE_TYPE	struct LibraryHeader *
#define __BASE_OR_IFACE_VAR		CodesetsBase
#endif
#define __BASE_OR_IFACE			__BASE_OR_IFACE_TYPE __BASE_OR_IFACE_VAR

/***************************************************************************/

#endif /* BASE_H */
