/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2013 by TextEditor.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 TextEditor class Support Site:  http://www.sf.net/projects/texteditor-mcc

 $Id$

***************************************************************************/

#include <libraries/mui.h>
#include <proto/muimaster.h>
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

/******************************************************************************/
/* include the minimal startup code to be able to start the class from a      */
/* shell without crashing the system                                          */
/******************************************************************************/
#include "shellstart.c"

#define VERSION       LIB_VERSION
#define REVISION      LIB_REVISION

#define CLASS         MUIC_TextEditor_mcp
#define SUPERCLASSP   MUIC_Mccprefs

#define INSTDATAP     InstData_MCP

#define USERLIBID     CLASS " " LIB_REV_STRING " [" SYSTEMSHORT "/" CPU "] (" LIB_DATE ") " LIB_COPYRIGHT
#define MASTERVERSION 19

#define CLASSINIT
#define CLASSEXPUNGE

#define USEDCLASSES used_mccs
static const char *used_mccs[] = { "TextEditor.mcc", "BetterString.mcc", NULL };

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

#if !defined(__MORPHOS__)
static BOOL nbitmapCanHandleRawData;
#endif

/******************************************************************************/
/* define the functions used by the startup code ahead of including mccinit.c */
/******************************************************************************/
static BOOL ClassInit(UNUSED struct Library *base);
static VOID ClassExpunge(UNUSED struct Library *base);

/******************************************************************************/
/* include the lib startup code for the mcc/mcp  (and muimaster inlines)      */
/******************************************************************************/
#define USE_ICON8_COLORS
#define USE_ICON8_BODY

#include "icon.h"

#if defined(__MORPHOS__)
#include <mui/Rawimage_mcc.h>
#else
#include <mui/NBitmap_mcc.h>
#endif

static Object *get_prefs_image(void)
{
  Object *obj;

  #if !defined(__MORPHOS__)
  if(nbitmapCanHandleRawData == TRUE)
  {
    obj = NBitmapObject,
      MUIA_FixWidth,       ICON32_WIDTH,
      MUIA_FixHeight,      ICON32_HEIGHT,
      MUIA_NBitmap_Type,   MUIV_NBitmap_Type_ARGB32,
      MUIA_NBitmap_Normal, icon32,
      MUIA_NBitmap_Width,  ICON32_WIDTH,
      MUIA_NBitmap_Height, ICON32_HEIGHT,
    End;
  }
  else
  {
    obj = NULL;
  }
  #else
  obj = RawimageObject,
    MUIA_Rawimage_Data, icon32,
  End;
  #endif

  // if the 32bit image data couldn't be loaded
  // we fall back to the 8bit icon
  if(obj == NULL)
  {
    obj = BodychunkObject,\
      MUIA_FixWidth,              ICON8_WIDTH,\
      MUIA_FixHeight,             ICON8_HEIGHT,\
      MUIA_Bitmap_Width,          ICON8_WIDTH ,\
      MUIA_Bitmap_Height,         ICON8_HEIGHT,\
      MUIA_Bodychunk_Depth,       ICON8_DEPTH,\
      MUIA_Bodychunk_Body,        (UBYTE *)icon8_body,\
      MUIA_Bodychunk_Compression, ICON8_COMPRESSION,\
      MUIA_Bodychunk_Masking,     ICON8_MASKING,\
      MUIA_Bitmap_SourceColors,   (ULONG *)icon8_colors,\
      MUIA_Bitmap_Transparent,    0,\
    End;
  }

  return obj;
}

#define PREFSIMAGEOBJECT get_prefs_image()

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

    #if !defined(__MORPHOS__)
    {
      struct Library *nbitmapMcc;

      nbitmapCanHandleRawData = FALSE;

      // we need at least NBitmap.mcc V15.8 to be able to let it handle raw image data
      if((nbitmapMcc = OpenLibrary("mui/NBitmap.mcc", 0)) != NULL)
      {
        SHOWVALUE(DBF_ALWAYS, nbitmapMcc->lib_Version);
        SHOWVALUE(DBF_ALWAYS, nbitmapMcc->lib_Revision);

        if(nbitmapMcc->lib_Version > 15 || (nbitmapMcc->lib_Version == 15 && nbitmapMcc->lib_Revision >= 8))
          nbitmapCanHandleRawData = TRUE;

        CloseLibrary(nbitmapMcc);
      }

      SHOWVALUE(DBF_ALWAYS, nbitmapCanHandleRawData);
    }
    #endif

    // Initialize the subclasses
    if(CreateSubClasses())
      return TRUE;

    DROPINTERFACE(ILocale);
    CloseLibrary((APTR)LocaleBase);
    LocaleBase  = NULL;
  }

  return FALSE;
}


static VOID ClassExpunge(UNUSED struct Library *base)
{
  DeleteSubClasses();

  CloseCat();

  if(LocaleBase)
  {
    DROPINTERFACE(ILocale);
    CloseLibrary((APTR)LocaleBase);
    LocaleBase  = NULL;
  }
}

