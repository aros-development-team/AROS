/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2013 by BetterString.mcc Open Source Team

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

#include <libraries/asl.h>
#include <libraries/mui.h>
#include <libraries/iffparse.h>
#include <clib/alib_protos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include "private.h"

#if defined(__amigaos4__)
struct Library *DiskfontBase = NULL;
struct Library *GfxBase = NULL;
struct Library *IntuitionBase = NULL;
struct Library *MUIMasterBase = NULL;
struct Library *LayersBase = NULL;
struct Library *LocaleBase = NULL;
struct Library *UtilityBase = NULL;
struct Library *KeymapBase = NULL;
#elif defined(__MORPHOS__)
struct Library *DiskfontBase = NULL;
struct Library *GfxBase = NULL;
struct IntuitionBase *IntuitionBase = NULL;
struct Library *MUIMasterBase = NULL;
struct Library *LayersBase = NULL;
struct Library *LocaleBase = NULL;
struct Library *UtilityBase = NULL;
struct Library *KeymapBase = NULL;
#else
struct Library *DiskfontBase = NULL;
struct Library *GfxBase = NULL;
struct IntuitionBase *IntuitionBase = NULL;
struct Library *MUIMasterBase = NULL;
struct Library *LayersBase = NULL;
struct Library *LocaleBase = NULL;
#if defined(__AROS__)
struct UtilityBase *UtilityBase = NULL;
#else
struct Library *UtilityBase = NULL;
#endif
struct Library *KeymapBase = NULL;
#endif

#if defined(__amigaos4__)
struct DiskfontIFace *IDiskfont = NULL;
struct GraphicsIFace *IGraphics = NULL;
struct MUIMasterIFace *IMUIMaster = NULL;
struct IntuitionIFace *IIntuition = NULL;
struct LayersIFace *ILayers = NULL;
struct LocaleIFace *ILocale = NULL;
struct UtilityIFace *IUtility = NULL;
struct KeymapIFace *IKeymap = NULL;
#endif

DISPATCHERPROTO(_Dispatcher);

int main(void)
{
  if((DiskfontBase = OpenLibrary("diskfont.library", 38)) &&
    GETINTERFACE(IDiskfont, DiskfontBase))
  if((GfxBase = OpenLibrary("graphics.library", 38)) &&
    GETINTERFACE(IGraphics, GfxBase))
  if((IntuitionBase = (APTR)OpenLibrary("intuition.library", 38)) &&
    GETINTERFACE(IIntuition, IntuitionBase))
  if((KeymapBase = OpenLibrary("keymap.library", 37)) &&
    GETINTERFACE(IKeymap, KeymapBase))
  if((LocaleBase = OpenLibrary("locale.library", 38)) &&
    GETINTERFACE(ILocale, LocaleBase))
  if((LayersBase = OpenLibrary("layers.library", 38)) &&
    GETINTERFACE(ILayers, LayersBase))
  if((UtilityBase = (APTR)OpenLibrary("utility.library", 38)) &&
    GETINTERFACE(IUtility, UtilityBase))
  if((MUIMasterBase = OpenLibrary("muimaster.library", MUIMASTER_VMIN)) &&
    GETINTERFACE(IMUIMaster, MUIMasterBase))
  if(StartClipboardServer() == TRUE)
  if(CreateSharedPool() == TRUE)
  {
    struct MUI_CustomClass *mcc;
    Object *a1, *a2, *app, *window, *bstring, *bstring2, *bpos, *ssize, *button, *numbutton;
    const char *classes[] = {"BetterString.mcp", NULL};

    #if defined(DEBUG)
    SetupDebug();
    #endif

    mcc = MUI_CreateCustomClass(NULL, "Area.mui", NULL, sizeof(struct InstData), ENTRY(_Dispatcher));

    app =  ApplicationObject,
          MUIA_Application_Author,      "BetterString.mcc Open Source Team",
          MUIA_Application_Base,        "BetterString-Test",
          MUIA_Application_Copyright,    "(C) 2005-2013 by BetterString.mcc Open Source Team",
          MUIA_Application_Description,  "BetterString.mcc demonstration program",
          MUIA_Application_Title,        "BetterString-Test",
          MUIA_Application_Version,      "$VER: BetterString-Demo V1.0 (18.05.2007)",
          MUIA_Application_UsedClasses, classes,

          MUIA_Application_Window, window = WindowObject,
            MUIA_Window_Title,  "BetterString-Test",
            MUIA_Window_ID,      MAKE_ID('M','A','I','N'),
            MUIA_Window_RootObject, VGroup,

            Child, PopaslObject,
                MUIA_Popstring_String,  NewObject(mcc->mcc_Class, NULL, StringFrame, MUIA_BetterString_NoInput, TRUE, MUIA_CycleChain, TRUE, End,
                MUIA_Popstring_Button,  MUI_MakeObject(MUIO_PopButton, MUII_PopUp),
                MUIA_Popasl_Type,       ASL_FontRequest,
                End,

            Child, ColGroup(2), StringFrame,
                MUIA_Background, MUII_GroupBack,
                Child, TextObject,
                    MUIA_Weight, 0,
                    MUIA_Text_Contents, "\33rName:",
                    End,
                Child, a1 = (Object *)NewObject(mcc->mcc_Class, NULL,
                    MUIA_CycleChain, TRUE,
                    MUIA_Background, MUII_GroupBack,
                    MUIA_Frame, MUIV_Frame_None,
                    MUIA_String_AdvanceOnCR, TRUE,
                    MUIA_String_MaxLen, 10,
                    MUIA_ObjectID, MAKE_ID('N','A','M','E'),
                    End,
                Child, TextObject,
                    MUIA_Weight, 0,
                    MUIA_Text_Contents, "\33rStreet:",
                    End,
                Child, a2 = (Object *)NewObject(mcc->mcc_Class, NULL,
                    MUIA_CycleChain, TRUE,
                    MUIA_Background, MUII_GroupBack,
                    MUIA_Frame, MUIV_Frame_None,
                    MUIA_String_AdvanceOnCR, TRUE,
                    MUIA_ObjectID, MAKE_ID('S','T','R','T'),
                    End,
                Child, TextObject,
                    MUIA_Weight, 0,
                    MUIA_Text_Contents, "\33rZip, City:",
                    End,
                Child, NewObject(mcc->mcc_Class, NULL,
                    MUIA_CycleChain, TRUE,
                    MUIA_Background, MUII_GroupBack,
                    MUIA_Frame, MUIV_Frame_None,
                    MUIA_String_AdvanceOnCR, TRUE,
                    MUIA_ObjectID, MAKE_ID('C','I','T','Y'),
                    End,
                End,

              Child, TextObject,
                MUIA_Font, MUIV_Font_Tiny,
                MUIA_Text_Contents, "\33cBetterString.mcc",
                End,
              Child, NewObject(mcc->mcc_Class, NULL,
                StringFrame,
                MUIA_CycleChain, TRUE,
                MUIA_String_Secret, TRUE,
  //                  MUIA_String_MaxLen, 20,
                MUIA_String_AdvanceOnCR, TRUE,
                MUIA_BetterString_StayActive, TRUE,
  //                  MUIA_String_Accept, "0123456789",
                MUIA_String_Contents, "This is a wonderful example string!",
                End,
              Child, NewObject(mcc->mcc_Class, NULL,
                StringFrame,
                MUIA_CycleChain, TRUE,
                MUIA_BetterString_InactiveContents, "This is a wonderful example string!",
                End,
              Child, NewObject(mcc->mcc_Class, NULL,
                StringFrame,
                MUIA_CycleChain, TRUE,
                MUIA_String_Secret, TRUE,
                MUIA_BetterString_InactiveContents, "This is a wonderful example string!",
                End,
              Child, TextObject,
                MUIA_Font, MUIV_Font_Tiny,
                MUIA_Text_Contents, "\33cCentered",
                End,
              Child, bstring = (Object *)NewObject(mcc->mcc_Class, NULL,
                ButtonFrame,
                MUIA_Font, MUIV_Font_Big,
  //                  StringFrame,
                MUIA_String_AdvanceOnCR, TRUE,
                MUIA_String_Format, MUIV_String_Format_Center,
                MUIA_String_Contents, "This is a wonderful example string!",
                MUIA_CycleChain, TRUE,
                End,
              Child, TextObject,
                MUIA_Font, MUIV_Font_Tiny,
                MUIA_Text_Contents, "\33cRight-Aligned",
                End,
              Child, bstring2 = NewObject(mcc->mcc_Class, NULL,
                MUIA_CycleChain, TRUE,
  //                  StringFrame,
                MUIA_String_AdvanceOnCR, TRUE,
                MUIA_String_Contents, "This is a wonderful example string!",
                MUIA_String_Format, MUIV_String_Format_Right,
                End,
              Child, TextObject,
                MUIA_Font, MUIV_Font_Tiny,
                MUIA_Text_Contents, "\33cStringObject",
                End,
              Child, StringObject,
                StringFrame,
                MUIA_String_AdvanceOnCR, TRUE,
                MUIA_String_Contents, "This is a standard StringObject",
                MUIA_String_Format, MUIV_String_Format_Right,
                MUIA_String_MaxLen, 1024,
                MUIA_CycleChain, TRUE,
                End,
              Child, (Object *)NewObject(mcc->mcc_Class, NULL,
                StringFrame,
                MUIA_String_AdvanceOnCR, TRUE,
                MUIA_String_Contents, "Select on activate",
                MUIA_String_MaxLen, 1024,
                MUIA_CycleChain, TRUE,
                MUIA_BetterString_SelectOnActive, TRUE,
                End,
              Child, HGroup,
                Child, button = SimpleButton("Insert"),
                Child, bpos = SliderObject,
                  MUIA_Slider_Horiz, TRUE,
                  MUIA_Numeric_Max, 60,
                  End,
                Child, ssize = SliderObject,
                  MUIA_Slider_Horiz, TRUE,
                  MUIA_Numeric_Min, -30,
                  MUIA_Numeric_Max, 30,
                  MUIA_Numeric_Value, 0,
                  End,
                Child, numbutton = NumericbuttonObject,
                  MUIA_Numeric_Min, -30,
                  MUIA_Numeric_Max, 30,
                  MUIA_Numeric_Value, 0,
                  MUIA_Disabled, TRUE,
                  End,
                End,
              End,
            End,
          End;

    if(app)
    {
      ULONG sigs;

      DoMethod(app, MUIM_Application_Load, MUIV_Application_Load_ENV);

      DoMethod(a1, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, a2, 3, MUIM_Set, MUIA_String_Contents, MUIV_TriggerValue);
      DoMethod(a2, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, a1, 3, MUIM_Set, MUIA_String_Contents, MUIV_TriggerValue);

      DoMethod(ssize, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, numbutton, 3, MUIM_Set, MUIA_Numeric_Value, MUIV_TriggerValue);
      DoMethod(bpos, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, bstring, 3, MUIM_Set, MUIA_String_BufferPos, MUIV_TriggerValue);
      DoMethod(ssize, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, bstring, 3, MUIM_Set, MUIA_BetterString_SelectSize, MUIV_TriggerValue);
      DoMethod(button, MUIM_Notify, MUIA_Pressed, FALSE, bstring, 3, MUIM_BetterString_Insert, "*Test*", MUIV_BetterString_Insert_BufferPos);
      DoMethod(window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, MUIV_Notify_Application, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

      set(bstring, MUIA_BetterString_KeyDownFocus, bstring2);
      set(bstring2, MUIA_BetterString_KeyUpFocus, bstring);

      set(window, MUIA_Window_ActiveObject, bstring);
      set(window, MUIA_Window_DefaultObject, bstring);
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
        DoMethod(app, MUIM_Application_Save, MUIV_Application_Save_ENV);

        MUI_DisposeObject(app);
        if(mcc)
            MUI_DeleteCustomClass(mcc);
    }

    DROPINTERFACE(IMUIMaster);
    CloseLibrary(MUIMasterBase);
    MUIMasterBase = NULL;
  }

  DeleteSharedPool();
  ShutdownClipboardServer();

  if(UtilityBase)
  {
    DROPINTERFACE(IUtility);
    CloseLibrary((struct Library *)UtilityBase);
  }

  if(LayersBase)
  {
    DROPINTERFACE(ILayers);
    CloseLibrary(LayersBase);
  }

  if(LocaleBase)
  {
    DROPINTERFACE(ILocale);
    CloseLibrary(LocaleBase);
  }

  if(KeymapBase)
  {
    DROPINTERFACE(IKeymap);
    CloseLibrary(KeymapBase);
  }

  if(IntuitionBase)
  {
    DROPINTERFACE(IIntuition);
    CloseLibrary((struct Library *)IntuitionBase);
  }

  if(GfxBase)
  {
    DROPINTERFACE(IGraphics);
    CloseLibrary(GfxBase);
  }

  if(DiskfontBase)
  {
    DROPINTERFACE(IDiskfont);
    CloseLibrary(DiskfontBase);
  }

  return 0;
}
