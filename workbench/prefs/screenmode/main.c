/*
    Copyright © 2003-2006, The AROS Development Team. All rights reserved.
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

#define VERSION "ScreenMode Preferences 1.2 (18.02.2006)"
#define COPYRIGHT "Copyright © 1995-2006, The AROS Development Team"

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
	SubWindow, (IPTR)(win = SystemPrefsWindowObject,
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

