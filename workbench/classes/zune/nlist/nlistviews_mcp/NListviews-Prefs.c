/***************************************************************************

 NListviews.mcp - New Listview MUI Custom Class Preferences
 Registered MUI class, Serial Number: 1d51 (0x9d510001 to 0x9d51001F
                                            and 0x9d510101 to 0x9d51013F)

 Copyright (C) 1996-2001 by Gilles Masson
 Copyright (C) 2001-2014 NList Open Source Team

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

#include <stdio.h>

#include <exec/tasks.h>
#include <exec/memory.h>
#include <libraries/mui.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/locale.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>

#include "locale.h"
#include "private.h"

#include "SDI_hook.h"

#if defined(__amigaos4__)
struct Library *IntuitionBase = NULL;
struct Library *MUIMasterBase = NULL;
struct Library *LocaleBase = NULL;
struct Library *UtilityBase = NULL;
#elif defined(__MORPHOS__)
struct IntuitionBase *IntuitionBase = NULL;
struct Library *MUIMasterBase = NULL;
struct Library *LocaleBase = NULL;
struct Library *UtilityBase = NULL;
#else
struct IntuitionBase *IntuitionBase = NULL;
struct Library *MUIMasterBase = NULL;
struct LocaleBase *LocaleBase = NULL;
struct Library *UtilityBase = NULL;
#endif

struct Library *CxBase = NULL;
struct Device *ConsoleDevice = NULL;

#if defined(__amigaos4__)
struct CommoditiesIFace *ICommodities = NULL;
struct ConsoleIFace *IConsole = NULL;
struct IntuitionIFace *IIntuition = NULL;
struct MUIMasterIFace *IMUIMaster = NULL;
struct LocaleIFace *ILocale = NULL;
struct UtilityIFace *IUtility = NULL;
#endif

struct IOStdReq ioreq;

extern DISPATCHERPROTO(_DispatcherP);

int main(void)
{
  ioreq.io_Message.mn_Length = sizeof(ioreq);
  if((UtilityBase = OpenLibrary("utility.library", 38)) &&
    GETINTERFACE(IUtility, UtilityBase))
  if((IntuitionBase = (APTR)OpenLibrary("intuition.library", 38)) &&
    GETINTERFACE(IIntuition, IntuitionBase))
  if((CxBase = OpenLibrary("commodities.library", 37L)) &&
    GETINTERFACE(ICommodities, CxBase))
  if(!OpenDevice("console.device", -1L, (struct IORequest *)&ioreq, 0L))
  {
    ConsoleDevice = (struct Device *)ioreq.io_Device;
    if(ConsoleDevice != NULL &&
       GETINTERFACE(IConsole, ConsoleDevice))
    if((LocaleBase = (APTR)OpenLibrary("locale.library", 38)) &&
      GETINTERFACE(ILocale, LocaleBase))
    {
      OpenCat();

      #if defined(DEBUG)
      SetupDebug();
      #endif

      if((MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN)) &&
         GETINTERFACE(IMUIMaster, MUIMasterBase))
      {
        Object *app = NULL;
        Object *window = NULL;
        struct MUI_CustomClass *mcc = NULL;

        mcc = MUI_CreateCustomClass(NULL, (STRPTR)"Group.mui", NULL, sizeof(struct NListviews_MCP_Data), ENTRY(_DispatcherP));
        if(mcc)
        {
          app = MUI_NewObject("Application.mui",
              MUIA_Application_Author,      "NList Open Source Team",
              MUIA_Application_Base,        "NListviews-Prefs",
              MUIA_Application_Copyright,   "(c) 2001-2007 NList Open Source Team",
              MUIA_Application_Description, "Preference for NList classes",
              MUIA_Application_Title,       "NListviews-Prefs",
              MUIA_Application_Version,     "$VER: NListviews-Prefs V1.0 (15.09.2007)",

              MUIA_Application_Window,
                window = MUI_NewObject("Window.mui",
                MUIA_Window_Title,    "NListviews-Prefs",
                MUIA_Window_RootObject,
                  MUI_NewObject("Group.mui",
                  MUIA_Background, MUII_PageBack,
                  MUIA_Frame, MUIV_Frame_Text,
                  MUIA_InnerBottom, 11,
                  MUIA_InnerLeft,  6,
                  MUIA_InnerRight,   6,
                  MUIA_InnerTop,    11,

//                  Child, RectangleObject, End,
                  Child, NewObject(mcc->mcc_Class, NULL, TAG_DONE),

                  TAG_DONE ),
                TAG_DONE ),
              TAG_DONE );
        }

        if(app)
        {
          unsigned long sigs;

          DoMethod(window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
          set(window, MUIA_Window_Open, TRUE);

          while((LONG)DoMethod(app, MUIM_Application_NewInput, &sigs) != (LONG)MUIV_Application_ReturnID_Quit)
          {
            if(sigs)
            {
              sigs = Wait(sigs | SIGBREAKF_CTRL_C);
              if(sigs & SIGBREAKF_CTRL_C)
                break;
            }
          }

          MUI_DisposeObject(app);

          if(mcc)
            MUI_DeleteCustomClass(mcc);
        }

        DROPINTERFACE(IMUIMaster);
        CloseLibrary(MUIMasterBase);
      }

      CloseCat();

      DROPINTERFACE(ILocale);
      CloseLibrary((struct Library *)LocaleBase);
    }

    if(ConsoleDevice)
    {
      DROPINTERFACE(IConsole);
      CloseDevice((struct IORequest *)&ioreq);
      ConsoleDevice = NULL;
    }
  }

  if(CxBase)
  {
    DROPINTERFACE(ICommodities);
    CloseLibrary(CxBase);
    CxBase = NULL;
  }

  if(IntuitionBase)
  {
    DROPINTERFACE(IIntuition);
    CloseLibrary((struct Library *)IntuitionBase);
  }

  if(UtilityBase)
  {
    DROPINTERFACE(IUtility);
    CloseLibrary(UtilityBase);
  }

  return 0;
}
