/*
    Copyright © 2016-2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/alib.h>

#include <libraries/mui.h>
#include <workbench/startup.h>

#include <stdlib.h>

#include "findgroup_class.h"
#include "locale.h"

#define ARGTEMPLATE "PATH,PATTERN,CONTENTS"

enum
{
    ARG_PATH,
    ARG_PATTERN,
    ARG_CONTENTS,
    ARG_COUNT
};

static void cleanup_exit(CONST_STRPTR str);

static Object *app, *win;
static struct DiskObject *dobj;
static struct RDArgs *rda;
static IPTR args[ARG_COUNT];

int main(int argc, char **argv)
{
    STRPTR path = NULL;
    STRPTR pattern = NULL;
    STRPTR contents = NULL;

    dobj = GetDiskObject("PROGDIR:Find");

    if (argc) // started from CLI
    {
        rda = ReadArgs(ARGTEMPLATE, args, NULL);
        if (!rda)
        {
            cleanup_exit(_(MSG_ERR_READARGS));
        }
        path = (STRPTR)args[ARG_PATH];
        pattern = (STRPTR)args[ARG_PATTERN];
        contents = (STRPTR)args[ARG_CONTENTS];
    }

    app = ApplicationObject,
        MUIA_Application_Author, (IPTR)"The AROS Development Team",
        MUIA_Application_Base, (IPTR)"FIND",
        MUIA_Application_Title, __(MSG_APP_TITLE),
        MUIA_Application_Version, (IPTR)"$VER: Find 1.2 (4.5.2018)",
        MUIA_Application_Copyright, __(MSG_APP_COPYRIGHT),
        MUIA_Application_Description, __(MSG_APP_DESCRIPTION),
        MUIA_Application_DiskObject, (IPTR)dobj,
        SubWindow, (IPTR)(win = WindowObject,
            MUIA_Window_Title, __(MSG_WI_TITLE),
            MUIA_Window_ID, MAKE_ID('F', 'I', 'N', 'D'),
            WindowContents, (IPTR)(FindGroupObject,
                MUIA_FindGroup_Path, (IPTR)path,
                MUIA_FindGroup_Pattern, (IPTR)pattern,
                MUIA_FindGroup_Contents, (IPTR)contents,
            End),
        End),
    End;

    if (!app)
        cleanup_exit(_(MSG_ERR_NO_APPLICATION));

    DoMethod
    (
        win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
        app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
    );

    SET(win, MUIA_Window_Open, TRUE);
    if (XGET(win, MUIA_Window_Open))
    {
        DoMethod(app, MUIM_Application_Execute);
    }
    else
    {
        MUI_Request(app, NULL, 0, _(MSG_APP_TITLE), _(MSG_OK), _(MSG_ERR_NO_WINDOW));
    }

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

    if (dobj) FreeDiskObject(dobj);
    if (rda) FreeArgs(rda);
    exit(0);
}
