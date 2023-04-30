/*
   Copyright (C) 2022-2023, The AROS Development Team. All rights reserved.
 */

/*********************************************************************************************/

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 0
#include <aros/debug.h>

#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <zune/systemprefswindow.h>

#include "locale.h"
#include "asleditor.h"
#include "args.h"
#include "prefs.h"

#define VERSION "$VER: Asl 41.1 (30.04.2023) AROS Dev Team"

/*********************************************************************************************/

int main(int argc, char **argv)
{
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
            struct Screen *pScreen = NULL;
            Object *application;
            Object *window;

            if (ARG(PUBSCREEN))
                pScreen = LockPubScreen((CONST_STRPTR)ARG(PUBSCREEN));

            application = (Object *)ApplicationObject,
                MUIA_Application_Title, __(MSG_WINDOW_TITLE),
                MUIA_Application_Version, (IPTR) VERSION,
                MUIA_Application_Description, __(MSG_WINDOW_TITLE),
                MUIA_Application_SingleTask, TRUE,
                MUIA_Application_Base, (IPTR) "ASLPREF",
                SubWindow, (IPTR)(window = (Object *)SystemPrefsWindowObject,
                    MUIA_Window_Screen, (IPTR)pScreen,
                    MUIA_Window_ID, MAKE_ID('A', 'S', 'L', ' '),
                    WindowContents, (IPTR) AslEditorObject,
                    End,
                End),
            End;

            if (application != NULL)
            {
                SET(window, MUIA_Window_Open, TRUE);
                DoMethod(application, MUIM_Application_Execute);

                MUI_DisposeObject(application);
            }
            if (pScreen)
                UnlockPubScreen(NULL, pScreen);
        }
        FreeArguments();
    }

    Locale_Deinitialize();
    return 0;
}
