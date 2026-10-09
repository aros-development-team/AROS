/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Envoy Services preferences - the services this host offers
          through the Envoy Services Manager (ENV:Envoy/services.prefs), and
          whether the Envoy servers start at boot (ENV:Envoy/AutoRun).

    Services FROM,USE/S,SAVE/S,PUBSCREEN/K
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <zune/systemprefswindow.h>

#include "locale.h"
#include "svceditor.h"
#include "args.h"
#include "prefs.h"

const char version[] = "$VER: Services 50.0 (7.10.2026)";

int main(int argc, char **argv)
{
    Object *application, *window;
    int rc = RETURN_OK;

    Locale_Initialize();
    Prefs_Initialize();

    if (ReadArguments(argc, argv))
    {
        if (ARG(USE) || ARG(SAVE))
        {
            if (!Prefs_HandleArgs((STRPTR)ARG(FROM), ARG(USE), ARG(SAVE)))
                rc = RETURN_ERROR;
        }
        else
        {
            struct Screen *screen = NULL;

            if (ARG(PUBSCREEN))
                screen = LockPubScreen((CONST_STRPTR)ARG(PUBSCREEN));

            application = (Object *)ApplicationObject,
                MUIA_Application_Title, __(MSG_WINTITLE),
                MUIA_Application_Version, (IPTR)version,
                MUIA_Application_Description, __(MSG_DESCRIPTION),
                MUIA_Application_SingleTask, TRUE,
                MUIA_Application_Base, (IPTR)"ENVOYSERVICESPREF",
                SubWindow, (IPTR)(window = (Object *)SystemPrefsWindowObject,
                    MUIA_Window_Screen, (IPTR)screen,
                    MUIA_Window_ID, MAKE_ID('E', 'S', 'V', 'C'),
                    WindowContents, (IPTR)SvcEditorObject,
                    End,
                End),
            End;

            if (application)
            {
                SET(window, MUIA_Window_Open, TRUE);
                DoMethod(application, MUIM_Application_Execute);
                MUI_DisposeObject(application);
            }
            else
                rc = RETURN_FAIL;
            if (screen)
                UnlockPubScreen(NULL, screen);
        }
        FreeArguments();
    }
    else
        rc = RETURN_FAIL;

    Prefs_Deinitialize();
    Locale_Deinitialize();
    return rc;
}
