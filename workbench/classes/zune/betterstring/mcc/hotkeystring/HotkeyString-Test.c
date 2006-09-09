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

#include <exec/tasks.h>
#include <libraries/mui.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/iffparse.h>
#include <proto/muimaster.h>

#include "HotkeyString_mcc.h"
#include "private.h"

#if defined(__amigaos4__)
struct Library *IntuitionBase = NULL;
struct Library *MUIMasterBase = NULL;
struct Library *UtilityBase = NULL;
struct Library *KeymapBase = NULL;
#elif defined(__MORPHOS__)
struct IntuitionBase *IntuitionBase = NULL;
struct Library *MUIMasterBase = NULL;
struct Library *UtilityBase = NULL;
struct Library *KeymapBase = NULL;
#else
struct IntuitionBase *IntuitionBase = NULL;
struct Library *MUIMasterBase = NULL;
struct Library *UtilityBase = NULL;
struct Library *KeymapBase = NULL;
#endif

#if defined(__amigaos4__)
struct MUIMasterIFace *IMUIMaster = NULL;
struct IntuitionIFace *IIntuition = NULL;
struct UtilityIFace *IUtility = NULL;
struct KeymapIFace *IKeymap = NULL;
#endif

extern SAVEDS ASM ULONG _Dispatcher(REG(a0, struct IClass * cl), REG(a2, Object * obj), REG(a1, Msg msg));

#ifdef __MORPHOS__
DISPATCHERPROTO(_Dispatcher);
#endif

int main(void)
{
  if((IntuitionBase = (APTR)OpenLibrary("intuition.library", 38)) &&
    GETINTERFACE(IIntuition, IntuitionBase))
  if((KeymapBase = OpenLibrary("keymap.library", 38)) &&
    GETINTERFACE(IKeymap, KeymapBase))
  if((UtilityBase = OpenLibrary("utility.library", 38)) &&
    GETINTERFACE(IUtility, UtilityBase))
	if((MUIMasterBase = OpenLibrary("muimaster.library", MUIMASTER_VMIN)) &&
    GETINTERFACE(IMUIMaster, MUIMasterBase))
	{
		struct	MUI_CustomClass	*mcc;
		Object	*app, *window, *bstring, *button;
		STRPTR	classes[] = {"BetterString.mcc", NULL};

		mcc = MUI_CreateCustomClass(NULL, "BetterString.mcc", NULL, sizeof(struct InstData), ENTRY(_Dispatcher));
		app =	ApplicationObject,
					MUIA_Application_Author,		"Allan Odgaard",
					MUIA_Application_Base,			"HotkeyString-Demo",
					MUIA_Application_Copyright,	"®1997 Allan Odgaard",
					MUIA_Application_Description,	"HotkeyString.mcc demonstration program",
					MUIA_Application_Title,			"HotkeyString-Demo",
					MUIA_Application_Version,		"$VER: HotkeyString-Demo V1.0 (3-Sep-97)",
					MUIA_Application_UsedClasses, classes,

					MUIA_Application_Window, window = WindowObject,
						MUIA_Window_Title,		"HotkeyString-Demo",
						MUIA_Window_ID,		    MAKE_ID('M','A','I','N'),
						MUIA_Window_RootObject, VGroup,
							Child, TextObject,
								MUIA_Font, MUIV_Font_Tiny,
								MUIA_Text_Contents, "\33cHotkeyString.mcc",
								End,

							Child, HGroup,
								Child, bstring = ((Object *)NewObject(mcc->mcc_Class, NULL,
									StringFrame,
									MUIA_CycleChain, TRUE,
									MUIA_String_AdvanceOnCR, TRUE,
									MUIA_HotkeyString_Snoop, FALSE,
									End),
								Child, button = TextObject, ButtonFrame,
									MUIA_CycleChain, TRUE,
									MUIA_Background, MUII_ButtonBack,
									MUIA_Text_Contents, "Sample",
									MUIA_Text_SetMax, TRUE,
									MUIA_InputMode, MUIV_InputMode_Toggle,
									End,
								End,

							Child, TextObject,
								MUIA_Font, MUIV_Font_Tiny,
								MUIA_Text_Contents, "\33cConstant snoop",
								End,
							Child, NewObject(mcc->mcc_Class, NULL,
								StringFrame,
								MUIA_CycleChain, TRUE,
								End,

							End,
						End,
					End;

		if(app)
		{
				ULONG sigs;

			DoMethod(window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, MUIV_Notify_Application, 3, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
			DoMethod(button, MUIM_Notify, MUIA_Selected, TRUE, MUIV_Notify_Window, 3, MUIM_Set, MUIA_Window_ActiveObject, bstring);
			DoMethod(button, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, bstring, 3, MUIM_Set, MUIA_HotkeyString_Snoop, MUIV_TriggerValue);
			DoMethod(bstring, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, button, 3, MUIM_Set, MUIA_Selected, FALSE);

			set(window, MUIA_Window_ActiveObject, bstring);
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
		}

    DROPINTERFACE(IMUIMaster);
		CloseLibrary(MUIMasterBase);
	}

  if(UtilityBase)
  {
    DROPINTERFACE(IUtility);
    CloseLibrary(UtilityBase);
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

  return 0;
}
