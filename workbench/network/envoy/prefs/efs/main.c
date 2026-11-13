/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Filesystem Exports - the Envoy filesystem export preferences
          (ENV:Envoy/EFS.prefs), the graphical counterpart of EfsConfig.

    FilesystemExports FROM,USE/S,SAVE/S,PUBSCREEN/K

    As in the other AROS preferences programs, FROM is used with USE or
    SAVE; without them the window opens with the current settings.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <zune/systemprefswindow.h>
#include <zune/prefseditor.h>

#include "locale.h"
#include "args.h"
#include "efsprefs.h"
#include "efseditor.h"
#include <aros/debug.h>

#define VERSION "$VER: FilesystemExports 50.0 (7.10.2026) AROS Dev Team"
const char version[] = VERSION;

struct Library *AccountsBase;
struct Library *EnvoyBase;

int main(int argc, char **argv)
{
    Object *application, *window;

    Locale_Initialize();
    EfsPrefs_Init();
    AccountsBase = OpenLibrary("accounts.library", 0);     /* names for user and group IDs */
    EnvoyBase = OpenLibrary("envoy.library", 0);           /* the user/group requester */

    if (ReadArguments(argc, argv))
    {
        if (ARG(USE) || ARG(SAVE))
            EfsPrefs_HandleArgs((CONST_STRPTR)ARG(FROM), ARG(USE), ARG(SAVE));
        else
        {
            struct Screen *screen = NULL;

            if (ARG(PUBSCREEN))
                screen = LockPubScreen((CONST_STRPTR)ARG(PUBSCREEN));
            application = (Object *)ApplicationObject,
                MUIA_Application_Title, __(MSG_WINTITLE),
                MUIA_Application_Version, (IPTR)VERSION,
                MUIA_Application_Description, __(MSG_WINTITLE),
                MUIA_Application_SingleTask, TRUE,
                MUIA_Application_Base, (IPTR)"EFSEXPORTSPREF",
                SubWindow, (IPTR)(window = (Object *)SystemPrefsWindowObject,
                    MUIA_Window_Screen, (IPTR)screen,
                    MUIA_Window_ID, MAKE_ID('E', 'F', 'S', 'X'),
                    WindowContents, (IPTR)EfsEditorObject,
                    End,
                End),
            End;
            if (application)
            {
                SET(window, MUIA_Window_Open, TRUE);
                DoMethod(application, MUIM_Application_Execute);
                MUI_DisposeObject(application);
            }
            if (screen)
                UnlockPubScreen(NULL, screen);
        }
        FreeArguments();
    }

    EfsPrefs_Free();
    if (EnvoyBase)
        CloseLibrary(EnvoyBase);
    if (AccountsBase)
        CloseLibrary(AccountsBase);
    Locale_Deinitialize();
    return 0;
}
