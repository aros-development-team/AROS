/*
    Copyright © 2010-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#define MUIMASTER_YES_INLINE_STDARG

#include <aros/debug.h>

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

#define VERSION "$VER: Palette 1.5 (07.01.2020) AROS Dev Team"
/*********************************************************************************************/

int main(int argc, char **argv)
{
    Object *application;
    Object *window;
    penarray_t pens = { NULL, -1 };
    APTR usepens = NULL;

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
            char *pubname = NULL;

            if (ARG(PUBSCREEN))
                pubname = (char *)ARG(PUBSCREEN);

            pScreen = LockPubScreen(pubname);
            if (pScreen)
            {
                if (GetBitMapAttr(pScreen->RastPort.BitMap, BMA_DEPTH) > 4)
                {
                    if (allocPens(pScreen->ViewPort.ColorMap, &pens))
                    {
                        usepens = &pens.pen[0];
                        appScreen = pScreen;
                    }
                }
                else
                {
                    UnlockPubScreen(NULL, pScreen);
                    pScreen = NULL;
                }
            }

            if (!appScreen)
            {
                appScreen = OpenScreenTags( NULL,
                    SA_Depth, 5,
                    SA_Width, 320,
                    SA_Height, 200,
                    SA_ShowTitle, TRUE,
                    SA_SharePens, TRUE,
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
                        (usepens) ? MUIA_PalEditor_Pens : TAG_IGNORE,
                        (IPTR)usepens,
                        MUIA_UserData, (IPTR)&pens,
                        MUIA_Window_Screen, (IPTR)appScreen,
                    End,
                End),
            End;

            if (application != NULL)
            {
                SET(window, MUIA_Window_Open, TRUE);
                DoMethod(application, MUIM_Application_Execute);

                MUI_DisposeObject(application);
            }
            releasePens(&pens);
            if (pScreen)
            {
                UnlockPubScreen(NULL, pScreen);
                if (pScreen == appScreen)
                    appScreen = NULL;
                pScreen = NULL;
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
