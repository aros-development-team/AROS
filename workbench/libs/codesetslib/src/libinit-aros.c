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

#include "lib.h"
#include "version.h"

#include <aros/symbolsets.h>
#include <exec/resident.h>
#include <proto/exec.h>

#include "debug.h"

/****************************************************************************/

// define the base to avoid recursion when opening the library
struct LibraryHeader * CodesetsBase;

/****************************************************************************/

BOOL LibInit(struct LibraryHeader *base)
{
  BOOL success = FALSE;

  InitSemaphore(&base->libSem);
  InitSemaphore(&base->poolSem);

  base->sysBase = (APTR)SysBase;
  base->pool = NULL;
  base->flags = 0;
  base->systemCodeset = NULL;

  // protect access to initBase()
  ObtainSemaphore(&base->libSem);

  success = initBase(base);

  // unprotect initBase()
  ReleaseSemaphore(&base->libSem);

  return success;
}

/****************************************************************************/

void LibExpunge(struct LibraryHeader *base)
{
  // free all our private data and stuff.
  ObtainSemaphore(&base->libSem);

  freeBase(base);
  
  // unprotect
  ReleaseSemaphore(&base->libSem);
}

/****************************************************************************/

ADD2INITLIB(LibInit, 0);
ADD2EXPUNGELIB(LibExpunge, 0);

/***********************************************************************/
