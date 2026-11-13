/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Firewall Preferences -- main entry point.
    Uses SystemPrefsWindowObject with a PrefsEditor subclass.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <zune/systemprefswindow.h>

#include "args.h"
#include "locale.h"
#include "fweditor.h"
#include "prefsdata.h"

#define VERSION "$VER: Firewall 1.0 (" __DATE__ ") AROS Dev Team"

int main(int argc, char **argv)
{
    Object *application, *window;

    Locale_Initialize();

    if (ReadArguments(argc, argv))
    {
        /* Load existing config */
        InitFirewallPrefs(PREFS_PATH_ENV);
        if (numFilterRules == 0 && numNatRules == 0)
            InitFirewallPrefs(PREFS_PATH_ENVARC);

        if (ARG(USE) || ARG(SAVE))
        {
            Prefs_HandleArgs(ARG(USE), ARG(SAVE));
        }
        else
        {
            struct Screen *pScreen = NULL;

            if (ARG(PUBSCREEN))
                pScreen = LockPubScreen((CONST_STRPTR)ARG(PUBSCREEN));

            application = (Object *)ApplicationObject,
                MUIA_Application_Title,       __(MSG_NAME),
                MUIA_Application_Version,     (IPTR)VERSION,
                MUIA_Application_Description, __(MSG_DESCRIPTION),
                MUIA_Application_SingleTask,  TRUE,
                MUIA_Application_Base,        (IPTR)"FIREWALLPREF",
                SubWindow, (IPTR)(window = (Object *)SystemPrefsWindowObject,
                    MUIA_Window_Screen, (IPTR)pScreen,
                    MUIA_Window_ID, MAKE_ID('F','W','P','R'),
                    WindowContents, (IPTR)FWEditorObject,
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
