/*
    Copyright © 2004-2006, The AROS Development Team. All rights reserved.
    This file is part of the Wanderer Preferences program, which is distributed
    under the terms of version 2 of the GNU General Public License.
    
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <aros/debug.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>

#include <zune/systemprefswindow.h>
#include "locale.h"
#include "wpeditor.h"

#define VERSIONSTR "$VER: Wanderer 1.1 (18.02.2006)"

int main(void)
{
    Object *application, *window;
    
    Locale_Initialize();
    
    application = ApplicationObject,
	MUIA_Application_Title, (IPTR)_(MSG_NAME),
	MUIA_Application_Version, (IPTR)VERSIONSTR,
	MUIA_Application_Copyright, (IPTR)"Copyright © 1995-2006, The AROS Development Team",
	MUIA_Application_Author, (IPTR)"The AROS Development Team",
	MUIA_Application_Base, (IPTR)"WANDERERPREF",
        MUIA_Application_SingleTask, TRUE,
        SubWindow, (IPTR) (window = SystemPrefsWindowObject,
            WindowContents, (IPTR) WPEditorObject,
            End,
        End),
    End;

    if (application)
    {
        SET(window, MUIA_Window_Open, TRUE);
        DoMethod(application, MUIM_Application_Execute);
        SET(window, MUIA_Window_Open, FALSE);
        
        MUI_DisposeObject(application);
    }
    else
    {
	D(bug("wandererprefs: Can't create application"));
    }

    Locale_Deinitialize();

    return 0;
}
