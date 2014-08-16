/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2014 TextEditor.mcc Open Source Team

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

#define DEBUG_USE_MALLOC_REDEFINE 1
#include "Debug.h"
#include "version.h"

/******************************************************************************/
/* include the minimal startup code to be able to start the class from a      */
/* shell without crashing the system                                          */
/******************************************************************************/
#include "shellstart.c"

#define VERSION       LIB_VERSION
#define REVISION      LIB_REVISION

#define CLASS         MUIC_TextEditor
#define SUPERCLASS    MUIC_Area

#define INSTDATA      InstData

#define USERLIBID     CLASS " " LIB_REV_STRING " [" SYSTEMSHORT "/" CPU "] (" LIB_DATE ") " LIB_COPYRIGHT
#define MASTERVERSION 19

#define CLASSINIT
#define CLASSEXPUNGE

#define USEDCLASSESP used_mcps
static const char *used_mcps[] = { "TextEditor.mcp", NULL };

#define MIN_STACKSIZE 8192

struct Library *DiskfontBase = NULL;
struct Library *KeymapBase = NULL;
struct Library *LayersBase = NULL;
struct Library *LocaleBase = NULL;
struct Library *RexxSysBase = NULL;
struct Library *WorkbenchBase = NULL;

#if defined(__amigaos4__)
struct DiskfontIFace *IDiskfont = NULL;
struct KeymapIFace *IKeymap = NULL;
struct LayersIFace *ILayers = NULL;
struct LocaleIFace *ILocale = NULL;
struct RexxSysIFace *IRexxSys = NULL;
struct Interface *IWorkbench = NULL;
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
  ENTER();

  if((LocaleBase = OpenLibrary("locale.library", 38)) &&
     GETINTERFACE(ILocale, struct LocaleIFace *, LocaleBase))
  {
    if((LayersBase = OpenLibrary("layers.library", 36)) &&
       GETINTERFACE(ILayers, struct LayersIFace *, LayersBase))
    {
      if((KeymapBase = OpenLibrary("keymap.library", 36)) &&
         GETINTERFACE(IKeymap, struct KeymapIFace *, KeymapBase))
      {
        if((RexxSysBase = OpenLibrary("rexxsyslib.library", 36)) &&
           GETINTERFACE(IRexxSys, struct RexxSysIFace *, RexxSysBase))
        {
          if((DiskfontBase = OpenLibrary("diskfont.library", 36)) &&
             GETINTERFACE(IDiskfont, struct DiskfontIFace *, DiskfontBase))
          {
            if(StartClipboardServer() == TRUE)
            {
              /* workbench.library is optional */
              if ((WorkbenchBase = OpenLibrary("workbench.library", 44)))
              {
                if (!(GETINTERFACE(IWorkbench, struct Interface *, WorkbenchBase)))
                {
                  CloseLibrary(WorkbenchBase);
                  WorkbenchBase = NULL;
                }
              }

              RETURN(TRUE);
              return(TRUE);
            }

            DROPINTERFACE(IDiskfont);
            CloseLibrary(DiskfontBase);
            DiskfontBase = NULL;
          }

          DROPINTERFACE(IRexxSys);
          CloseLibrary(RexxSysBase);
          RexxSysBase  = NULL;
        }

        DROPINTERFACE(IKeymap);
        CloseLibrary(KeymapBase);
        KeymapBase  = NULL;
      }

      DROPINTERFACE(ILayers);
      CloseLibrary(LayersBase);
      LayersBase  = NULL;
    }

    DROPINTERFACE(ILocale);
    CloseLibrary(LocaleBase);
    LocaleBase  = NULL;
  }

  RETURN(FALSE);
  return(FALSE);
}


static VOID ClassExpunge(UNUSED struct Library *base)
{
  ENTER();

  ShutdownClipboardServer();

  if(WorkbenchBase)
  {
    DROPINTERFACE(IWorkbench);
    CloseLibrary(WorkbenchBase);
    WorkbenchBase = NULL;
  }

  if(DiskfontBase)
  {
    DROPINTERFACE(IDiskfont);
    CloseLibrary(DiskfontBase);
    DiskfontBase = NULL;
  }

  if(RexxSysBase)
  {
    DROPINTERFACE(IRexxSys);
    CloseLibrary(RexxSysBase);
    RexxSysBase = NULL;
  }

  if(KeymapBase)
  {
    DROPINTERFACE(IKeymap);
    CloseLibrary(KeymapBase);
    KeymapBase = NULL;
  }

  if(LayersBase)
  {
    DROPINTERFACE(ILayers);
    CloseLibrary(LayersBase);
    LayersBase = NULL;
  }

  if(LocaleBase)
  {
    DROPINTERFACE(ILocale);
    CloseLibrary(LocaleBase);
    LocaleBase = NULL;
  }

  LEAVE();
}

