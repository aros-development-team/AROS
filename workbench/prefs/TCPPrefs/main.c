/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    $Id: main.c 24810 2006-09-16 10:11:22Z olivieradam $
 */

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>
#include <zune/systemprefswindow.h>

#include "locale.h"
#include "args.h"
#include "fpeditor.h"
#include "AROSTCPPrefs.h"

#define VERSION "$VER: Fonts 0.1 (x) ©AROS Dev Team"

int main(void)
{
	Object *application,  *window;

	//Locale_Initialize();

	if (ReadArguments()) {
		/* FIXME: handle arguments... */

		// FROM - import prefs from this file at start
		// USE  - 'use' the loaded prefs immediately, don't open window.
		// SAVE - 'save' the lodaed prefs immediately, don't open window.

		FreeArguments();
	}

	ReadTCPPrefs();

	application = ApplicationObject,
	MUIA_Application_Title,  "AROS TCP Prefs",
	MUIA_Application_Version, (IPTR)VERSION,
	MUIA_Application_Description,  "TCPIP prefs for AROS",
	MUIA_Application_Base, (IPTR)"TCPIPREF",
	SubWindow, (IPTR)(window = SystemPrefsWindowObject,
			  MUIA_Window_ID, MAKE_ID('F', 'W', 'I', 'N'),
			  WindowContents, (IPTR)FPEditorObject,
			  End,
			  End),
	End;

	if (application != NULL) {
		SET(window, MUIA_Window_Open, TRUE);
		DoMethod(application, MUIM_Application_Execute);
		SET(window, MUIA_Window_Open, FALSE);

		MUI_DisposeObject(application);
	}

	//Locale_Deinitialize();

	return 0;
}
