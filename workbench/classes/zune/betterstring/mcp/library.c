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

 $Id$

***************************************************************************/

#include <libraries/mui.h>
#include <proto/muimaster.h>
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

#define CLASS         MUIC_BetterString_mcp
#define SUPERCLASSP   MUIC_Mccprefs

#define INSTDATAP     InstData_MCP

#define UserLibID     "$VER: BetterString.mcp " LIB_REV_STRING CPU " (" LIB_DATE ") " LIB_COPYRIGHT
#define MASTERVERSION 19

#define ClassInit
#define ClassExit

#include "locale.h"

#if defined(__amigaos4__) || defined(__MORPHOS__)
struct Library *LocaleBase = NULL;
#else
struct LocaleBase *LocaleBase = NULL;
#endif

#if defined(__amigaos4__)
struct LocaleIFace *ILocale = NULL;
#endif

BOOL ClassInitFunc(UNUSED struct Library *base)
{
  if((LocaleBase = (APTR)OpenLibrary("locale.library", 38)) &&
     GETINTERFACE(ILocale, LocaleBase))
  {
    // open the TextEditor.mcp catalog
    OpenCat();

    return TRUE;
  }

  return FALSE ;
}


VOID ClassExitFunc(UNUSED struct Library *base)
{
  CloseCat();

  if(LocaleBase)
  {
    DROPINTERFACE(ILocale);
    CloseLibrary((APTR)LocaleBase);
    LocaleBase  = NULL;
  }
}


/******************************************************************************/
/*                                                                            */
/* include the lib startup code for the mcc/mcp  (and muimaster inlines)      */
/*                                                                            */
/******************************************************************************/

#include "icon.bh"

#include "mccheader.c"
