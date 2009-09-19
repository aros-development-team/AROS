/*
    Copyright � 2003-2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG
#define DEBUG 0

#include <aros/debug.h>
#include <dos/dos.h>

#include <proto/muimaster.h>
#include <proto/intuition.h>

#include <zune/systemprefswindow.h>

#include "locale.h"
#include "smeditor.h"

#define VERSION "ScreenMode Preferences 1.3 (19.9.2009)"
#define COPYRIGHT "Copyright � 1995-2009, The AROS Development Team"

static const char vers[] = VERSION;
static const char version[] = "$VER: " VERSION "\n";


int __nocommandline = 1;

int main()
{
    Object *app, *win;

    Locale_Initialize();

    app = ApplicationObject,
        MUIA_Application_Title, (IPTR) __(MSG_NAME),
        MUIA_Application_Version, (IPTR) vers,
        MUIA_Application_Copyright, (IPTR) COPYRIGHT,
        MUIA_Application_Author, (IPTR) "The AROS Development Team",
        MUIA_Application_Description, (IPTR) __(MSG_NAME),
        MUIA_Application_SingleTask, TRUE,
        MUIA_Application_Base, (IPTR) "SCREENMODEPREF",
	SubWindow, (IPTR)(win = SystemPrefsWindowObject,
	MUIA_Window_ID, MAKE_ID('S','W','I','N'),
            WindowContents, (IPTR) SMEditorObject,
            End,
	End),
    End;

    if (app)
    {	     
        set(win, MUIA_Window_Open, TRUE);
	
        DoMethod(app, MUIM_Application_Execute);
    
        MUI_DisposeObject(app);

        Locale_Deinitialize();

	return RETURN_ERROR;        
    }
    else
    {
        D(bug("screenmode preferences: couldn't create application"));
    }
    
    Locale_Deinitialize();

    return RETURN_OK;
}

