/*
   Copyright © 1995-2013, The AROS Development Team. All rights reserved.
   $Id$
 */

/*********************************************************************************************/

#define MUIMASTER_YES_INLINE_STDARG

#include <aros/debug.h>

#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <libraries/mui.h>
#include <zune/systemprefswindow.h>

#include "locale.h"
#include "registertab.h"
#include "page_language.h"
#include "page_region.h"
#include "page_timezone.h"
#include "args.h"
#include "prefs.h"

#define VERSION "$VER: Locale 2.2 (24.05.2011) AROS Dev Team"

/*********************************************************************************************/

int main(int argc, char **argv)
{
    Object *application;
    Object *window;

    D(bug("[locale prefs] InitLocale\n"));
    Locale_Initialize();

    D(bug("[locale prefs] started\n"));

    if (Prefs_Initialize())
    {
        if (ReadArguments(argc, argv))
        {
            D(bug("[locale prefs] initialized\n"));
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
                    MUIA_Application_Base, (IPTR) "LOCALEPREF",
                    SubWindow, (IPTR)(window = (Object *)SystemPrefsWindowObject,
                        MUIA_Window_ID, ID_LCLE,
                        WindowContents, (IPTR) LocaleRegisterObject,
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

        Prefs_Deinitialize();
    }
    Locale_Deinitialize();
    return 0;
}
