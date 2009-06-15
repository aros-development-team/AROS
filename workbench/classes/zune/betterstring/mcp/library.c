/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2009 by BetterString.mcc Open Source Team

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

#include <proto/exec.h>
#include <proto/intuition.h>

/******************************************************************************/
/*                                                                            */
/* MCC/MCP name and version                                                   */
/*                                                                            */
/* ATTENTION:  The FIRST LETTER of NAME MUST be UPPERCASE                     */
/*                                                                            */
/******************************************************************************/

#include "private.h"
#include "version.h"

#define VERSION       LIB_VERSION
#define REVISION      LIB_REVISION

#define CLASS         MUIC_BetterString_mcp
#define SUPERCLASSP   MUIC_Mccprefs

#define INSTDATAP     InstData_MCP

#define USERLIBID     CLASS " " LIB_REV_STRING " [" SYSTEMSHORT "/" CPU "] (" LIB_DATE ") " LIB_COPYRIGHT
#define MASTERVERSION 19

#define CLASSINIT
#define CLASSEXPUNGE
#define MIN_STACKSIZE 8192

#include "locale.h"

#if defined(__amigaos4__) || defined(__MORPHOS__)
struct Library *LocaleBase = NULL;
#else
struct LocaleBase *LocaleBase = NULL;
#endif

#if defined(__amigaos4__)
struct LocaleIFace *ILocale = NULL;
#endif

/******************************************************************************/
/* define the functions used by the startup code ahead of including mccinit.c */
/******************************************************************************/
static BOOL ClassInit(UNUSED struct Library *base);
static VOID ClassExpunge(UNUSED struct Library *base);

/******************************************************************************/
/* include the lib startup code for the mcc/mcp  (and muimaster inlines)      */
/******************************************************************************/
#define USE_IMAGE_COLORS
#define USE_IMAGE_BODY
#define PREFSIMAGEOBJECT \
  BodychunkObject,\
    MUIA_FixWidth,              IMAGE_WIDTH,\
    MUIA_FixHeight,             IMAGE_HEIGHT,\
    MUIA_Bitmap_Width,          IMAGE_WIDTH ,\
    MUIA_Bitmap_Height,         IMAGE_HEIGHT,\
    MUIA_Bodychunk_Depth,       IMAGE_DEPTH,\
    MUIA_Bodychunk_Body,        (UBYTE *)image_body,\
    MUIA_Bodychunk_Compression, IMAGE_COMPRESSION,\
    MUIA_Bodychunk_Masking,     IMAGE_MASKING,\
    MUIA_Bitmap_SourceColors,   (ULONG *)image_colors,\
    MUIA_Bitmap_Transparent,    0,\
  End
#include "icon.bh"
#include "mccinit.c"

/******************************************************************************/
/* define all implementations of our user functions                           */
/******************************************************************************/
static BOOL ClassInit(UNUSED struct Library *base)
{
  if((LocaleBase = (APTR)OpenLibrary("locale.library", 38)) &&
     GETINTERFACE(ILocale, struct LocaleIFace *, LocaleBase))
  {
    // open the TextEditor.mcp catalog
    OpenCat();

    return TRUE;
  }

  return FALSE ;
}


static VOID ClassExpunge(UNUSED struct Library *base)
{
  CloseCat();

  if(LocaleBase)
  {
    DROPINTERFACE(ILocale);
    CloseLibrary((APTR)LocaleBase);
    LocaleBase  = NULL;
  }
}

