/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005 by BetterString.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 BetterString class Support Site:  http://www.sf.net/projects/bstring-mcc/

 $Id: library.c,v 1.1 2005/04/21 20:52:04 damato Exp $

***************************************************************************/

#include <proto/exec.h>

/******************************************************************************/
/*                                                                            */
/* MCC/MCP name and version                                                   */
/*                                                                            */
/* ATTENTION:  The FIRST LETTER of NAME MUST be UPPERCASE                     */
/*                                                                            */
/******************************************************************************/

#include "private.h"
#include "rev.h"

#define VERSION       LIB_VERSION
#define REVISION      LIB_REVISION

#define CLASS         MUIC_BetterString
#define SUPERCLASS    MUIC_Area

#define INSTDATA      InstData

#define UserLibID     "$VER: BetterString.mcc " LIB_REV_STRING CPU " (" LIB_DATE ") " LIB_COPYRIGHT
#define MASTERVERSION 19

#define ClassInit
#define ClassExit

struct Library *DiskfontBase = NULL;
struct Library *KeymapBase = NULL;
struct Library *LocaleBase = NULL;

#if defined(__amigaos4__)
struct DiskfontIFace *IDiskfont = NULL;
struct KeymapIFace *IKeymap = NULL;
struct LocaleIFace *ILocale = NULL;
#endif

BOOL ClassInitFunc(UNUSED struct Library *base)
{
  if((LocaleBase = OpenLibrary("locale.library", 38)) &&
     GETINTERFACE(ILocale, LocaleBase))
  {
    if((KeymapBase = OpenLibrary("keymap.library", 38)) &&
       GETINTERFACE(IKeymap, KeymapBase))
    {
      if((DiskfontBase = OpenLibrary("diskfont.library", 38)) &&
         GETINTERFACE(IDiskfont, DiskfontBase))
      {
        return(TRUE);
      }

      DROPINTERFACE(IKeymap);
      CloseLibrary(KeymapBase);
      KeymapBase  = NULL;
    }

    DROPINTERFACE(ILocale);
    CloseLibrary(LocaleBase);
    LocaleBase  = NULL;
  }

  return(FALSE);
}


VOID ClassExitFunc(UNUSED struct Library *base)
{
  if(DiskfontBase)
  {
    DROPINTERFACE(IDiskfont);
    CloseLibrary(DiskfontBase);
    DiskfontBase = NULL;
  }

  if(KeymapBase)
  {
    DROPINTERFACE(IKeymap);
    CloseLibrary(KeymapBase);
    KeymapBase = NULL;
  }

  if(LocaleBase)
  {
    DROPINTERFACE(ILocale);
    CloseLibrary(LocaleBase);
    LocaleBase = NULL;
  }
}

/******************************************************************************/
/*                                                                            */
/* include the lib startup code for the mcc/mcp  (and muimaster inlines)      */
/*                                                                            */
/******************************************************************************/

#include "mccheader.c"
