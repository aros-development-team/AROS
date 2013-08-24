/***************************************************************************

 codesets.library - Amiga shared library for handling different codesets
 Copyright (C) 2001-2005 by Alfonso [alfie] Ranieri <alforan@tin.it>.
 Copyright (C) 2005-2007 by codesets.library Open Source Team

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

/***************************************************************************/

extern struct LibraryHeader* CodesetsBase;

/***************************************************************************/
