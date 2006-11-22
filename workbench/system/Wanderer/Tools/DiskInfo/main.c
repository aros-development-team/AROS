/*
    Copyright © 2005, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 1
#include <aros/debug.h>

#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/workbench.h>
#include <proto/dos.h>

#include <dos/dos.h>
#include <libraries/mui.h>
#include <workbench/startup.h>

#include <stdlib.h>

#include "diskinfo.h"
#include "locale.h"
#include "support.h"

void cleanup(CONST_STRPTR message)
{
    DiskInfo_Deinitialize();
    Locale_Deinitialize();

    if (message != NULL)
    {
        ShowError(NULL, NULL, message, TRUE);
        exit(RETURN_FAIL);
    }
    else
    {
        exit(RETURN_OK);
    }
}

int main(int argc, char **argv)
{
    Object *application;
    STRPTR  initial = NULL;
    Locale_Initialize();

    if (!DiskInfo_Initialize()) cleanup(_(MSG_ERROR_CLASSES));

    if (argc != 0)
    {
        /* start from wanderer only */
        DiskInfo_Deinitialize();
        Locale_Deinitialize();
        PrintFault(ERROR_OBJECT_WRONG_TYPE, argv[0]);
        return RETURN_FAIL;
    }

    struct WBStartup *startup = (struct WBStartup *) argv;
    if (startup->sm_NumArgs > 1)
    {
        initial = startup->sm_ArgList[1].wa_Name;
        D(bug("[DiskInfo] main, initial: %s\n", initial));
        application = DiskInfoObject,
            MUIA_DiskInfo_Initial, (IPTR) initial,
            MUIA_DiskInfo_Aspect, 0,
        End;

        if (application != NULL)
        {
            DoMethod(application, MUIM_Application_Execute);
            MUI_DisposeObject(application);
        }
    } else {
        DiskInfo_Deinitialize();
        Locale_Deinitialize();
        PrintFault(ERROR_REQUIRED_ARG_MISSING, argv[0]);
        return RETURN_FAIL;
    }
    cleanup(NULL);

    return RETURN_OK; /* make compiler happy */
}
