/*
    Copyright © 2010-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/alib.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>


#include <stdlib.h> /* for exit() */
#include <stdio.h>
#include <string.h>

#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>

#include <libraries/mui.h>
#include <zune/systemprefswindow.h>

#include <prefs/palette.h>

#include "locale.h"
#include "paleditor.h"
#include "args.h"
#include "prefs.h"

/* #define DEBUG 1 */
#include <aros/debug.h>

#define VERSION "$VER: Palette 1.4 (04.01.2020) AROS Dev Team"
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
            struct Screen *pScreen = NULL, *appScreen = NULL;

            if (ARG(PUBSCREEN))
            {
                pScreen = LockPubScreen((CONST_STRPTR)ARG(PUBSCREEN));
                if (pScreen && (GetBitMapAttr(pScreen->RastPort.BitMap, BMA_DEPTH) > 4))
                    appScreen = pScreen;
            }

            if (!appScreen)
            {
                appScreen = OpenScreenTags( NULL,
                    SA_Depth, 5,
                    SA_Width, 320,
                    SA_Height, 200,
                    SA_ShowTitle, TRUE,
                    SA_Title, "",
                    TAG_END);
            }                

            application = (Object *)ApplicationObject,
                MUIA_Application_Title, __(MSG_WINTITLE),
                MUIA_Application_Version, (IPTR) VERSION,
                MUIA_Application_Description, __(MSG_WINTITLE),
                MUIA_Application_SingleTask, TRUE,
                MUIA_Application_Base, (IPTR) "PALETTEPREF",
                SubWindow, (IPTR)(window = (Object *)SystemPrefsWindowObject,
                    MUIA_Window_Screen, (IPTR)appScreen,
                    MUIA_Window_ID, ID_PALT,
                    WindowContents, (IPTR) PalEditorObject,
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
            {
                UnlockPubScreen(NULL, pScreen);
                pScreen = appScreen = NULL;
            }
            if (appScreen)
                CloseScreen(appScreen);
        }
        FreeArguments();
    }

    Locale_Deinitialize();
    return 0;
}

/*********************************************************************************************/
