/*
    Copyright © 2003-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <libraries/mui.h>
#include <zune/systemprefswindow.h>

#include "locale.h"
#include "args.h"
#include "fpeditor.h"
#include "prefs.h"

#define VERSION "$VER: Fonts 1.1 ("ADATE") ©AROS Dev Team"

int main(int argc, char **argv)
{
    Object *application, *window;

    Locale_Initialize();

    if (ReadArguments(argc, argv))
    {
        if (ARG(USE) || ARG(SAVE))
        {
            Prefs_HandleArgs((STRPTR)ARG(FROM), ARG(USE), ARG(SAVE));
        }
        else
        {
            application = (Object *)ApplicationObject,
                MUIA_Application_Title,  __(MSG_NAME),
                MUIA_Application_Version, (IPTR) VERSION,
                MUIA_Application_Description,  __(MSG_NAME),
                MUIA_Application_SingleTask, TRUE,
                MUIA_Application_Base, (IPTR) "FONTPREF",
                SubWindow, (IPTR) (window = (Object *)SystemPrefsWindowObject,
                    MUIA_Window_ID, MAKE_ID('F','W','I','N'),
                    WindowContents, (IPTR) FPEditorObject,
                    End,
                End),
            End;

            if (application != NULL)
            {
                SET(window, MUIA_Window_Open, TRUE);
                DoMethod(application, MUIM_Application_Execute);
                SET(window, MUIA_Window_Open, FALSE);

                MUI_DisposeObject(application);
            }
        }
        FreeArguments();
    }

    Locale_Deinitialize();

    return 0;
}
