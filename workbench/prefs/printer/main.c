/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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
#include "editor.h"
#include "args.h"
#include "prefs.h"

#include <aros/debug.h>

#define VERSION "$VER: Printer 0.1 (01.31.2011) AROS Dev Team"
/*********************************************************************************************/

int main(int argc, char **argv)
{
    Object *application;
    Object *window;

    D(bug("[printer prefs] InitLocale\n"));
    Locale_Initialize();

    D(bug("[printer prefs] started\n"));

    /* init */
    if (ReadArguments(argc, argv))
    {
        D(bug("[printer prefs] initialized\n"));
        if (ARG(USE) || ARG(SAVE))
        {
            Prefs_HandleArgs((STRPTR)ARG(FROM), ARG(USE), ARG(SAVE));
        }
        else
        {
            application = (Object *)ApplicationObject,
                MUIA_Application_Author, (IPTR)"Jason McMullan <jason.mcmullan@gmail.com>",
                MUIA_Application_Copyright, (IPTR)"2012, AROS Team",
                MUIA_Application_Title, __(MSG_WINTITLE),
                MUIA_Application_Version, (IPTR) VERSION,
                MUIA_Application_Description, __(MSG_WINTITLE),
                MUIA_Application_Base, (IPTR) "PRINTERPREF",
                SubWindow, (IPTR)(window =
                    SystemPrefsWindowObject,
                        MUIA_Window_ID, ID_PTXT,
                        WindowContents, (IPTR)
                                PrinterEditorObject,
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


