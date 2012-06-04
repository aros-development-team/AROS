/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

/*********************************************************************************************/

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <zune/systemprefswindow.h>

#include <prefs/pointer.h>

#include "locale.h"
#include "pteditor.h"
#include "args.h"
#include "prefs.h"

// #define DEBUG 1
#include <aros/debug.h>

#define VERSION "$VER: Pointer 1.4 (04.06.2012) AROS Dev Team"
/*********************************************************************************************/

int main(int argc, char **argv)
{
    Object *application;
    Object *window;

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
                MUIA_Application_Title, __(MSG_WINTITLE),
                MUIA_Application_Version, (IPTR) VERSION,
                MUIA_Application_Description, __(MSG_WINTITLE),
                MUIA_Application_Base, (IPTR) "POINTERPREF",
                SubWindow, (IPTR)(window = (Object *)SystemPrefsWindowObject,
                    MUIA_Window_ID, ID_NPTR,
                    WindowContents, (IPTR) PTEditorObject,
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
