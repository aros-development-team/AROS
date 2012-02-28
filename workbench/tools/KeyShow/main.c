/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/alib.h>

#include <libraries/mui.h>

#include "keyboardgroup_class.h"
#include "locale.h"

static void cleanup_exit(CONST_STRPTR str);

static Object *app, *win;

int main(void)
{
    Locale_Initialize();

    app = ApplicationObject,
        MUIA_Application_Title, (IPTR)"KeyShow",
        MUIA_Application_Version, (IPTR)"$VER: KeyShow 1.2 (28.02.2012)",
        MUIA_Application_Copyright, (IPTR)_(MSG_AppCopyright),
        MUIA_Application_Author, (IPTR)"The AROS Development Team",
        MUIA_Application_Description, (IPTR)_(MSG_AppDescription),
        MUIA_Application_Base, (IPTR)"KEYSHOW",

        SubWindow, (IPTR)(win = WindowObject,
            MUIA_Window_Title, (IPTR)_(MSG_WI_TITLE),
            MUIA_Window_ID, MAKE_ID('K','S','W','N'),
            WindowContents, (IPTR)KeyboardGroupObject,
            End,
        End),
    End;

    if (app == NULL)
    {
        cleanup_exit("Can't create application");
    }

    DoMethod(win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
        app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    SET(win, MUIA_Window_Open, TRUE);
    DoMethod(app, MUIM_Application_Execute);
    cleanup_exit(NULL);
    return 0;
}

static void cleanup_exit(CONST_STRPTR str)
{
    if (str)
    {
        struct EasyStruct es =
        {
            sizeof(struct EasyStruct), 0,
            _(MSG_ERR), str, _(MSG_OK)
        };
        EasyRequestArgs(NULL, &es, NULL, NULL);
    }
    MUI_DisposeObject(app);
    Locale_Deinitialize();
}
