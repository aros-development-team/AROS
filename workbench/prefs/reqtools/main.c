/*
   Copyright © 2013, The AROS Development Team. All rights reserved.
   $Id$
 */

/*********************************************************************************************/

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <zune/systemprefswindow.h>

#include "locale.h"
#include "reqtoolseditor.h"
#include "args.h"
#include "prefs.h"

 #define DEBUG 1 
#include <aros/debug.h>

#define VERSION "$VER: ReqTools 41.0 (11.09.2013) AROS Dev Team"

/*********************************************************************************************/

int main(int argc, char **argv)
{
    Object *application;
    Object *window;

    Locale_Initialize();

    /* init */
    if (ReadArguments(argc, argv))
    {
        if (ARG(USE) || ARG(SAVE))
        {
            Prefs_HandleArgs((STRPTR)ARG(FROM), ARG(USE), ARG(SAVE));
        }
        else
        {
            application = (Object *)ApplicationObject,
                MUIA_Application_Title, __(MSG_WINDOW_TITLE),
                MUIA_Application_Version, (IPTR) VERSION,
                MUIA_Application_Description, __(MSG_WINDOW_TITLE),
                MUIA_Application_Base, (IPTR) "REQTOOLSPREF",
                SubWindow, (IPTR)(window = (Object *)SystemPrefsWindowObject,
                    MUIA_Window_ID, MAKE_ID('R', 'Q', 'T', 'S'),
                    WindowContents, (IPTR) ReqToolsEditorObject,
                    End,
                End),
            End;

            if (application != NULL)
            {
                SET(window, MUIA_Window_Open, TRUE);
                DoMethod(application, MUIM_Application_Execute);

                MUI_DisposeObject(application);
            }
        }
        FreeArguments();
    }

    Locale_Deinitialize();
    return 0;
}
