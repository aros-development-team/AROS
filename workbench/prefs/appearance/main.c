/*
    Copyright © 2013-2016, The AROS Development Team. All rights reserved.
    $Id$
*/

/*********************************************************************************************/

#define DEBUG 0
#include <aros/debug.h>

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <zune/systemprefswindow.h>

#include "appearanceeditor.h"
#include "args.h"
#include "locale.h"

#define VERSION "$VER: Appearance 1.1 (12.6.2017) AROS Dev Team"
/*********************************************************************************************/

int main(int argc, char **argv)
{
    Object *application;
    Object *window;

    D(bug("[AppearancePrefs] InitLocale\n"));
    Locale_Initialize();

    D(bug("[AppearancePrefs] started\n"));

    /* init */
    if (ReadArguments(argc, argv))
    {
        D(bug("[AppearancePrefs] initialized\n"));
        if (ARG(USE) || ARG(SAVE))
        {
//            Prefs_HandleArgs((STRPTR)ARG(FROM), ARG(USE), ARG(SAVE));
        }
        else
        {
            application = (Object *)ApplicationObject,
                MUIA_Application_Title, __(MSG_WINTITLE),
                MUIA_Application_Version, (IPTR) VERSION,
                MUIA_Application_Description, __(MSG_WINTITLE),
                MUIA_Application_SingleTask, TRUE,
                MUIA_Application_Base, (IPTR) "APPEARPREF",
                SubWindow, (IPTR)(window = (Object *)SystemPrefsWindowObject,
//                    MUIA_Window_ID, ID_SERL,
                    WindowContents, (IPTR) AppearanceEditorObject,
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

/*********************************************************************************************/


