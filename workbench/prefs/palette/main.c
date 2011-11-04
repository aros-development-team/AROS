/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/dos.h>

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

#define VERSION "$VER: Palette 1.1 (04.11.2011) AROS Dev Team"
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
                MUIA_Application_Title, __(MSG_WINTITLE),
                MUIA_Application_Version, (IPTR) VERSION,
                MUIA_Application_Description, __(MSG_WINTITLE),
                MUIA_Application_Base, (IPTR) "PALETTEPREF",
                SubWindow, (IPTR)(window = (Object *)SystemPrefsWindowObject,
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
        }
        FreeArguments();
    }

    Locale_Deinitialize();
    return 0;
}

/*********************************************************************************************/
