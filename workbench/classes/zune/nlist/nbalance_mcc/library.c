/***************************************************************************

 NBalance.mcc - New Balance MUI Custom Class
 Copyright (C) 2008 by NList Open Source Team

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
#include <proto/layers.h>

/* local includes */
#include "Debug.h"
#include "private.h"
#include "version.h"

/* mcc initialisation */
#define VERSION       LIB_VERSION
#define REVISION      LIB_REVISION

#define CLASS         MUIC_NBalance
#define SUPERCLASS    MUIC_Balance

#define INSTDATA      InstData

#define USERLIBID     CLASS " " LIB_REV_STRING " [" SYSTEMSHORT "/" CPU "] (" LIB_DATE ") " LIB_COPYRIGHT
#define MASTERVERSION 19

#define	CLASSINIT
#define	CLASSEXPUNGE
#define MIN_STACKSIZE 16384

struct Library *LayersBase = NULL;

#if defined(__amigaos4__)
struct LayersIFace *ILayers = NULL;
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
  if((LayersBase = OpenLibrary("layers.library", 39L)) &&
     GETINTERFACE(ILayers, struct LayersIFace *, LayersBase))
  {
    return(TRUE);
  }

  return(FALSE);
}


static VOID ClassExpunge(UNUSED struct Library *base)
{
  if(LayersBase != NULL)
  {
    DROPINTERFACE(ILayers);
	CloseLibrary(LayersBase);
    LayersBase = NULL;
  }
}
