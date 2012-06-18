/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/alib.h>
#include <proto/icon.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <workbench/startup.h>
#include <zune/systemprefswindow.h>

#include "locale.h"
#include "bibeditor.h"
#include "args.h"
#include "prefs.h"

// #define DEBUG 1
#include <aros/debug.h>

#define VERSION "$VER: BIB 2.0 (14.05.2012) AROS Dev Team"
/*********************************************************************************************/

int main(int argc, char **argv)
{
    Object *application;
    Object *window;
    struct DiskObject *disko;
    static struct WBStartup *argmsg;
    static struct WBArg *wb_arg;
    static STRPTR cxname;

    D(bug("[bib prefs] InitLocale\n"));
    Locale_Initialize();

    D(bug("[bib prefs] started\n"));

    if (argc)
    {
        cxname = argv[0];
    } else {
        argmsg = (struct WBStartup *)argv;
        wb_arg = argmsg->sm_ArgList;
        cxname = wb_arg->wa_Name;
    }
    disko = GetDiskObject(cxname);
    
    /* init */
    if (ReadArguments(argc, argv))
    {
        D(bug("[bib prefs] initialized\n"));
        if (ARG(USE) || ARG(SAVE))
        {
            Prefs_HandleArgs((STRPTR)ARG(FROM), ARG(USE), ARG(SAVE));
        }
        else
        {
            application = (Object *)ApplicationObject,
                MUIA_Application_Title, __(MSG_TITLE),
                MUIA_Application_Version, (IPTR) VERSION,
                MUIA_Application_Description, __(MSG_TITLE),
                MUIA_Application_Base, (IPTR) "BIBPREF",
                MUIA_Application_DiskObject, (IPTR)disko,
                SubWindow, (IPTR)(window = (Object *)SystemPrefsWindowObject,
                    MUIA_Window_ID, MAKE_ID('B','I','P','8'),
                    WindowContents, (IPTR) BibEditorObject,
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

    if (disko != NULL)
        FreeDiskObject(disko);
    Locale_Deinitialize();
    return 0;
}

/*********************************************************************************************/


