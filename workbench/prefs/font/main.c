/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>
#include <zune/systemprefswindow.h>

#include "locale.h"
#include "args.h"
#include "fpeditor.h"

int main(void)
{
    Object *application,  *window;

    Locale_Initialize();
    
    if (ReadArguments())
    {
        /* FIXME: handle arguments... */
        
        // FROM - import prefs from this file at start
        // USE  - 'use' the loaded prefs immediately, don't open window.
        // SAVE - 'save' the lodaed prefs immediately, don't open window.
        
        FreeArguments();
    }
    
    application = ApplicationObject,
        SubWindow, (IPTR) window = SystemPrefsWindowObject,
            WindowContents, (IPTR) FPEditorObject,
            End,
        End,
    End;

    if (application != NULL)
    {
        SET(window, MUIA_Window_Open, TRUE);
        DoMethod(application, MUIM_Application_Execute);
        SET(window, MUIA_Window_Open, FALSE);
        
        MUI_DisposeObject(application);
    }

    Locale_Deinitialize();
    
    return 0;
}
