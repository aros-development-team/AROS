/***************************************************************************

 NBitmap.mcc - New Bitmap MUI Custom Class
 Copyright (C) 2006 by Daniel Allsopp
 Copyright (C) 2007 by NList Open Source Team

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

/* system includes */
#include <proto/exec.h>
#include <proto/intuition.h>

/* local includes */
#include "Debug.h"
#include "SetPatch.h"
#include "private.h"
#include "version.h"

/* mcc initialisation */
#define VERSION       LIB_VERSION
#define REVISION      LIB_REVISION

#define CLASS         MUIC_NBitmap
#define SUPERCLASS    MUIC_Area

#define INSTDATA      InstData

#define USERLIBID     CLASS " " LIB_REV_STRING " [" SYSTEMSHORT "/" CPU "] (" LIB_DATE ") " LIB_COPYRIGHT
#define MASTERVERSION 19

#define CLASSINIT
#define CLASSEXPUNGE
#define MIN_STACKSIZE 8192

// libraries
struct Library *DataTypesBase = NULL;
#if !defined(__amigaos4__)
struct Library *CyberGfxBase = NULL;
#endif

#if defined(__amigaos4__)
struct DataTypesIFace *IDataTypes = NULL;
#endif

/******************************************************************************/
/* define the functions used by the startup code ahead of including mccinit.c */
/******************************************************************************/
static BOOL ClassInit(UNUSED struct Library *base);
static VOID ClassExpunge(UNUSED struct Library *base);

/******************************************************************************/
/* include the lib startup code for the mcc/mcp  (and muimaster inlines)      */
/******************************************************************************/
#include "mccinit.c"

/******************************************************************************/
/* define all implementations of our user functions                           */
/******************************************************************************/
static BOOL ClassInit(UNUSED struct Library *base)
{
  BOOL res = FALSE;

  ENTER();

  // open library interfaces
  if((DataTypesBase = OpenLibrary("datatypes.library", 39L)) != NULL &&
     GETINTERFACE(IDataTypes, struct DataTypesIFace *, DataTypesBase))
  {
    #if !defined(__amigaos4__)
    if((CyberGfxBase = OpenLibrary("cybergraphics.library", 40)) != NULL &&
       GETINTERFACE(ICyberGfx, struct CyberGfxIFace *, CyberGfxBase))
    {
    }
    #endif

    GetSetPatchVersion();

    res = TRUE;
  }

  RETURN(res);
  return res;
}

static VOID ClassExpunge(UNUSED struct Library *base)
{
  ENTER();

  /* close libraries */

  #if !defined(__amigaos4__)
  if(CyberGfxBase != NULL)
  {
    DROPINTERFACE(ICyberGfx);
    CloseLibrary(CyberGfxBase);
    CyberGfxBase = NULL;
  }
  #endif

  if(DataTypesBase != NULL)
  {
    DROPINTERFACE(IDataTypes);
    CloseLibrary(DataTypesBase);
    DataTypesBase = NULL;
  }

  LEAVE();
}
