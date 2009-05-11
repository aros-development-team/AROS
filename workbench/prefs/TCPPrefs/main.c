/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$
 */

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>
#include <zune/systemprefswindow.h>

#include "locale.h"
#include "args.h"
#include "netpeditor.h"
#include "prefsdata.h"

#define VERSION "$VER: Network 0.5 (11.05.2009) AROS Dev Team"

int main(void)
{
	Object *application,  *window;

	Locale_Initialize();

	if (ReadArguments()) {
		/* FIXME: handle arguments... */

		// FROM - import prefs from this file at start
		// USE  - 'use' the loaded prefs immediately, don't open window.
		// SAVE - 'save' the lodaed prefs immediately, don't open window.

		FreeArguments();
	}

    SetDefaultValues();
	ReadTCPPrefs();

	application = (Object *)ApplicationObject,
	MUIA_Application_Title,  __(MSG_NAME),
	MUIA_Application_Version, (IPTR)VERSION,
	MUIA_Application_Description,  __(MSG_DESCRIPTION),
	MUIA_Application_Base, (IPTR)"NETPREF",
	SubWindow, (IPTR)(window = (Object *)SystemPrefsWindowObject,
			  MUIA_Window_ID, MAKE_ID('N', 'E', 'T', 'P'),
			  WindowContents, (IPTR)NetPEditorObject,
			  End,
			  End),
	End;

	if (application != NULL) {
		SET(window, MUIA_Window_Open, TRUE);
		DoMethod(application, MUIM_Application_Execute);
		SET(window, MUIA_Window_Open, FALSE);

		MUI_DisposeObject(application);
	}

	Locale_Deinitialize();

	return 0;
}
