/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <zune/systemprefswindow.h>

#include "locale.h"
#include "sereditor.h"
#include "args.h"
#include "prefs.h"

/* #define DEBUG 1 */
#include <aros/debug.h>

#define VERSION "$VER: Serial 2.1 (05.01.2010) AROS Dev Team"
/*********************************************************************************************/

int main(int argc, char **argv)
{
    Object *application;
    Object *window;

    D(bug("[serial prefs] InitLocale\n"));
    Locale_Initialize();

    D(bug("[serial prefs] started\n"));

    /* init */
    if (ReadArguments(argc, argv))
    {
        D(bug("[serial prefs] initialized\n"));
        if (ARG(USE) || ARG(SAVE))
        {
            Prefs_HandleArgs((STRPTR)ARG(FROM), ARG(USE), ARG(SAVE));
        }
        else
        {
            application = (Object *)ApplicationObject,
                MUIA_Application_Title, __(MSG_WINTITLE),
                MUIA_Application_Version, (IPTR) VERSION,
                MUIA_Application_Description, __(MSG_WINTITLE),
                MUIA_Application_Base, (IPTR) "SERIALPREF",
                SubWindow, (IPTR)(window = (Object *)SystemPrefsWindowObject,
                    MUIA_Window_ID, ID_SERL,
                    WindowContents, (IPTR) SerEditorObject,
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


