/***************************************************************************

 NListviews.mcp - New Listview MUI Custom Class Preferences
 Registered MUI class, Serial Number: 1d51 (0x9d510001 to 0x9d51001F
                                            and 0x9d510101 to 0x9d51013F)

 Copyright (C) 1996-2001 by Gilles Masson
 Copyright (C) 2001-2005 by NList Open Source Team

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

/******************************************************************************/
/*                                                                            */
/* includes                                                                   */
/*                                                                            */
/******************************************************************************/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

/******************************************************************************/
/*                                                                            */
/* MCC/MCP name and version                                                   */
/*                                                                            */
/* ATTENTION:  The FIRST LETTER of NAME MUST be UPPERCASE                     */
/*                                                                            */
/******************************************************************************/

#include "private.h"
#include "version.h"

#define	VERSION             LIB_VERSION
#define	REVISION            LIB_REVISION

#define CLASS               MUIC_NListviews_mcp
#define SUPERCLASSP         MUIC_Mccprefs

#define	INSTDATAP           NListviews_MCP_Data

#define USERLIBID           CLASS " " LIB_REV_STRING " [" SYSTEMSHORT "/" CPU "] (" LIB_DATE ") " LIB_COPYRIGHT
#define MASTERVERSION       19

#define	CLASSINIT
#define	CLASSEXPUNGE

#define USEDCLASSES used_mccs
static const char *used_mccs[] = { "NList.mcc", "NListview.mcc", NULL };

#define USEDCLASSESP used_mcps
static const char *used_mcps[] = { "NListviews.mcp", NULL };

#define MIN_STACKSIZE       8192

#include "locale.h"

struct Library *CxBase = NULL;
struct Library *LocaleBase = NULL;
struct Device *ConsoleDevice = NULL;
//struct Catalog *catalog = NULL;

#if defined(__amigaos4__)
struct CommoditiesIFace *ICommodities = NULL;
struct LocaleIFace *ILocale = NULL;
struct ConsoleIFace *IConsole = NULL;
#endif

static struct IOStdReq ioreq;

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
  if((CxBase = OpenLibrary("commodities.library", 37L)) != NULL &&
     GETINTERFACE(ICommodities, struct CommoditiesIFace *, CxBase))
  {
    ioreq.io_Message.mn_Length = sizeof(ioreq);
    if(OpenDevice("console.device", -1L, (struct IORequest *)&ioreq, 0L) == 0)
	{
      ConsoleDevice = (struct Device *)ioreq.io_Device;
      if(GETINTERFACE(IConsole, struct ConsoleIFace *, ConsoleDevice))
      {
        if((LocaleBase = OpenLibrary( "locale.library", 38)) != NULL &&
           GETINTERFACE(ILocale, struct LocaleIFace *, LocaleBase))
        {
          // open the NListviews_mcp catalog
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

          return TRUE;
        }

        DROPINTERFACE(IConsole);
      }

      CloseDevice((struct IORequest *)&ioreq);
    }

    DROPINTERFACE(ICommodities);
    CloseLibrary(CxBase);
    CxBase = NULL;
  }

  return FALSE;
}


static VOID ClassExpunge(UNUSED struct Library *base)
{
  // close the catalog
  CloseCat();

  if(LocaleBase != NULL)
  {
    DROPINTERFACE(ILocale);
    CloseLibrary(LocaleBase);
    LocaleBase = NULL;
  }

  if(ConsoleDevice != NULL)
  {
    DROPINTERFACE(IConsole);
    CloseDevice((struct IORequest *)&ioreq);
  	ConsoleDevice = NULL;
  }

  if(CxBase != NULL)
  {
    DROPINTERFACE(ICommodities);
    CloseLibrary(CxBase);
    CxBase = NULL;
  }
}

