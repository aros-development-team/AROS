/*
    Copyright © 2004-2006, The AROS Development Team. All rights reserved.
    This file is part of the Wanderer Preferences program, which is distributed
    under the terms of version 2 of the GNU General Public License.
    
    $Id$
*/
#define DEBUG 1
#include <aros/debug.h>

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>
#include <dos/dos.h>

#include <zune/systemprefswindow.h>
#include "locale.h"
#include "wpeditor.h"

#define VERSIONSTR "$VER: Wanderer Prefs 1.1 (18.02.2006) ©1995-2006 The AROS Development Team"

int main(void)
{
    Object *application, *window;
    int rc = RETURN_OK;
    D(bug("[WANPREFS] Starting...\n"));
    Locale_Initialize();
    
    application = ApplicationObject,
        MUIA_Application_Title, (IPTR) "Wanderer Prefs",
        MUIA_Application_Version, (IPTR) VERSIONSTR,
        MUIA_Application_Description, __(MSG_DESCRIPTION),
        MUIA_Application_Copyright, (IPTR)"Copyright © 1995-2006, The AROS Development Team",
        MUIA_Application_Author, (IPTR)"The AROS Development Team",
        MUIA_Application_Base, (IPTR)"WANDERERPREFS",
        MUIA_Application_SingleTask, TRUE,
        SubWindow, (IPTR)(
            window = SystemPrefsWindowObject,
                MUIA_Window_ID, MAKE_ID('W','W','I','N'),
                MUIA_Window_Title, __(MSG_NAME),
                WindowContents, (IPTR)WPEditorObject,
                End,
            End),
    End;
    
    if (application)
    {
        SET(window, MUIA_Window_Open, TRUE);
        DoMethod(application, MUIM_Application_Execute);
        SET(window, MUIA_Window_Open, FALSE);
        MUI_DisposeObject(application);
    } else {
        rc = RETURN_FAIL;
        D(bug("[WANPREFS] Can't create application!\n"));
    }

    Locale_Deinitialize();
    D(bug("[WANPREFS] Quitting...\n"));
    return rc;
}
