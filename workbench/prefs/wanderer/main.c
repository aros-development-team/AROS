/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    This file is part of the Wanderer Preferences program, which is distributed
    under the terms of version 2 of the GNU General Public License.
    
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>

#include <zune/systemprefswindow.h>
#include "locale.h"
#include "wpeditor.h"

int main(void)
{
    Object *application, *window;
    
    Locale_Initialize();
    
    application = ApplicationObject,
        SubWindow, (IPTR) window = SystemPrefsWindowObject,
            WindowContents, (IPTR) WPEditorObject,
            End,
        End,
    End;

    if (application)
    {
        SET(window, MUIA_Window_Open, TRUE);
        DoMethod(application, MUIM_Application_Execute);
        SET(window, MUIA_Window_Open, FALSE);
        
        MUI_DisposeObject(application);
    }

    Locale_Deinitialize();

    return 0;
}
