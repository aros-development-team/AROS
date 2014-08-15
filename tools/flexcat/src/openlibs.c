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

#if defined(AMIGA)
#include "flexcat.h"
#include "openlibs.h"
#include "utils.h"
#define __NOLIBBASE__
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/locale.h>
#include <proto/codesets.h>

#if defined(__amigaos3__) || defined(__MORPHOS__) || defined(__AROS__)
#if defined(__AROS__)
struct UtilityBase *UtilityBase = NULL;
#else
struct Library *UtilityBase = NULL;
#endif
struct IntuitionBase *IntuitionBase = NULL;
#if defined(__MORPHOS__)
struct Library *LocaleBase = NULL;
#else
struct LocaleBase *LocaleBase = NULL;
#endif
#endif
struct Library *CodesetsBase = NULL;

#if defined(__amigaos4__)
struct CodesetsIFace *ICodesets = NULL;
#endif


BOOL OpenLibs(void)
{
  #if defined(__amigaos3__) || defined(__MORPHOS__) || defined(__AROS__)
  if(
     #if defined(__AROS__)
     (UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 37)) != NULL &&
     #else
     (UtilityBase = OpenLibrary("utility.library", 37)) != NULL &&
     #endif
     (IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37)) != NULL &&
     #if defined(__MORPHOS__)
     (LocaleBase = (struct Library *)OpenLibrary("locale.library", 37)) != NULL)
     #else
     (LocaleBase = (struct LocaleBase *)OpenLibrary("locale.library", 37)) != NULL)
     #endif
  #endif
  {
    if((CodesetsBase = OpenLibrary(CODESETSNAME, CODESETSVER)) &&
       GETINTERFACE(ICodesets, CodesetsBase))
    {
      return TRUE;
    }
  }

  return FALSE;
}

void CloseLibs(void)
{
  #if defined(__amigaos3__) || defined(__MORPHOS__) || defined(__AROS__)
  if(UtilityBase != NULL)
  {
    CloseLibrary((struct Library *)UtilityBase);
    UtilityBase = NULL;
  }
  if(IntuitionBase != NULL)
  {
    CloseLibrary((struct Library *)IntuitionBase);
    IntuitionBase = NULL;
  }
  if(LocaleBase != NULL)
  {
    CloseLibrary((struct Library *)LocaleBase);
    LocaleBase = NULL;
  }
  #endif

  if(CodesetsBase != NULL)
  {
    DROPINTERFACE(ICodesets);
    CloseLibrary(CodesetsBase);
    CodesetsBase = NULL;
  }
}

#endif // AMIGA
