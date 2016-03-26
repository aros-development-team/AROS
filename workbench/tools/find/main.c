/*
    Copyright © 2016, The AROS Development Team. All rights reserved.
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

#include "findgroup_class.h"
#include "locale.h"

#if 0
#define ARGTEMPLATE "ARCHIVE,DESTINATION,PUBSCREEN/K"


enum
{
    ARG_ARCHIVE,
    ARG_DESTINATION,
    ARG_PUBSCREEN,
    ARG_COUNT
};

#endif

static void cleanup_exit(CONST_STRPTR str);

static Object *app, *win;
static struct DiskObject *dobj, *dobj2 = NULL;
static struct RDArgs *rda;
#if 0
static IPTR args[ARG_COUNT];
static BPTR olddir = (BPTR)-1;
#endif

int main(int argc, char **argv)
{
    STRPTR archive = NULL, temp = NULL;
    STRPTR destination = NULL;
    STRPTR pubscreen = NULL;

    STRPTR *toolarray, *toolarray2;
#if 0
    dobj = GetDiskObject("PROGDIR:Find");
    if (dobj)
    {
        toolarray = dobj->do_ToolTypes;
        archive = FindToolType(toolarray, "ARCHIVE");
        destination = FindToolType(toolarray, "DESTINATION");
    }

    if (argc) // started from CLI
    {
        rda = ReadArgs(ARGTEMPLATE, args, NULL);
        if (!rda)
        {
            cleanup_exit(_(MSG_ERR_READARGS));
        }
        archive = (STRPTR)args[ARG_ARCHIVE];
        destination = (STRPTR)args[ARG_DESTINATION];
        pubscreen = (STRPTR)args[ARG_PUBSCREEN];
    }
    else // started from Wanderer
    {
        struct WBStartup *wbmsg = (struct WBStartup *)argv;
        struct WBArg *wbarg = wbmsg->sm_ArgList;

        // started as default tool from an archive's icon
        if (wbmsg->sm_NumArgs > 1 && wbarg[1].wa_Lock)
        {
            olddir = CurrentDir(wbarg[1].wa_Lock);
            archive = wbarg[1].wa_Name;
            dobj2 = GetDiskObject(archive);
            if (dobj2)
            {
                toolarray2 = dobj2->do_ToolTypes;
                destination = FindToolType(toolarray2, "DESTINATION");
                temp = FindToolType(toolarray2, "ARCHIVE");
                if (temp)
                    archive = temp;
            }
        }
    }
#endif
    app = ApplicationObject,
        MUIA_Application_Author, (IPTR)"The AROS Development Team",
        MUIA_Application_Base, (IPTR)"FIND",
        MUIA_Application_Title, (IPTR)"Find",
        MUIA_Application_Version, (IPTR)"$VER: Find 1.0 (26.03.2016)",
        MUIA_Application_Copyright, __(MSG_AppCopyright),
        MUIA_Application_Description, __(MSG_AppDescription),
        MUIA_Application_DiskObject, (IPTR)dobj,
        SubWindow, (IPTR)(win = WindowObject,
            MUIA_Window_Title, __(MSG_WI_TITLE),
            MUIA_Window_ID, MAKE_ID('F', 'I', 'N', 'D'),
            WindowContents, (IPTR)(FindGroupObject,
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
#if 0
    if (dobj) FreeDiskObject(dobj);
    if (dobj2) FreeDiskObject(dobj2);
    if (rda) FreeArgs(rda);
    if (olddir != (BPTR)-1) CurrentDir(olddir);
#endif
}
