/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005 by TextEditor.mcc Open Source Team

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

#if defined(__amigaos4__)
struct IntuitionIFace *IIntuition = NULL;
struct MUIMasterIFace *IMUIMaster = NULL;
struct LocaleIFace *ILocale = NULL;
struct UtilityIFace *IUtility = NULL;
#endif

extern DISPATCHERPROTO(_DispatcherP);

int main(void)
{
  if((UtilityBase = OpenLibrary("utility.library", 38)) &&
    GETINTERFACE(IUtility, UtilityBase))
  {
    if((IntuitionBase = OpenLibrary("intuition.library", 38)) &&
      GETINTERFACE(IIntuition, IntuitionBase))
    {
      if((LocaleBase = OpenLibrary("locale.library", 38)) &&
        GETINTERFACE(ILocale, LocaleBase))
      {
        OpenCat();

        if((MUIMasterBase = OpenLibrary("muimaster.library", MUIMASTER_VMIN)) &&
           GETINTERFACE(IMUIMaster, MUIMasterBase))
        {
          Object *app = NULL;
          Object *window;
          struct MUI_CustomClass *mcc = NULL;

          if(CreateSubClasses())
            mcc = MUI_CreateCustomClass(NULL, "Group.mui", NULL, sizeof(struct InstData_MCP), ENTRY(_DispatcherP));
      
          if(mcc)
          {
            app = MUI_NewObject("Application.mui",
                MUIA_Application_Author,    "Allan Odgaard",
                MUIA_Application_Base,      "TextEditor-Prefs",
                MUIA_Application_Copyright, "®1997 Allan Odgaard",
                MUIA_Application_Description, "Preference for TextEditor.mcc",
                MUIA_Application_Title,     "TextEditor-Prefs",
                MUIA_Application_Version,   "$VER: TextEditor-Prefs V1.0 (18-Feb-97)",

                MUIA_Application_Window,
                  window = MUI_NewObject("Window.mui",
                  MUIA_Window_Title,    "TextEditor-Prefs",
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
            while((LONG)DoMethod(app, MUIM_Application_NewInput, &sigs) != MUIV_Application_ReturnID_Quit)
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

            DeleteSubClasses();
          }

          DROPINTERFACE(IMUIMaster);
          CloseLibrary(MUIMasterBase);
        }

        CloseCat();

        DROPINTERFACE(ILocale);
        CloseLibrary(LocaleBase);
      }

      DROPINTERFACE(IIntuition);
      CloseLibrary(IntuitionBase);
    }

    DROPINTERFACE(IUtility);
    CloseLibrary(UtilityBase);
  }

  return 0;
}
