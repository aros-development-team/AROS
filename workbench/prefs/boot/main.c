/*
    Copyright (C) 2009-2024, The AROS Development Team. All rights reserved.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <libraries/mui.h>
#include <zune/systemprefswindow.h>

#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

#include "locale.h"
#include "args.h"
#include "booteditor.h"

#define VERSION "$VER: Boot 1.4 (22.1.2024)"

int main(int argc, char **argv)
{
    Object *application,  *window;

    Locale_Initialize();

    ReadArguments(argc, argv);

    /* Show application unless SAVE parameter was used */
    if (!((BOOL)ARG(SAVE)))
    {
        struct Screen *pScreen = NULL;

        if (ARG(PUBSCREEN))
            pScreen = LockPubScreen((CONST_STRPTR)ARG(PUBSCREEN));

        application = (Object *)ApplicationObject,
            MUIA_Application_Title,  __(MSG_NAME),
            MUIA_Application_Version, (IPTR)VERSION,
            MUIA_Application_Description,  __(MSG_DESCRIPTION),
            MUIA_Application_SingleTask, TRUE,
            MUIA_Application_Base, (IPTR)"BOOTPREF",
            SubWindow, (IPTR)(window = (Object *)SystemPrefsWindowObject,
                MUIA_Window_Screen, (IPTR)pScreen,
                MUIA_Window_ID, MAKE_ID('B', 'O', 'O', 'T'),
                WindowContents, (IPTR)BootEditorObject,
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

        if (pScreen)
            UnlockPubScreen(NULL, pScreen);
    }

    FreeArguments();

    Locale_Deinitialize();

    return 0;
}
