/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <aros/debug.h>

#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/workbench.h>
#include <proto/dos.h>

#include <dos/dos.h>
#include <libraries/mui.h>
#include <workbench/startup.h>

#include <stdlib.h>

#include "executecommand.h"
#include "locale.h"
#include "support.h"

void cleanup(CONST_STRPTR message)
{
    ExecuteCommand_Deinitialize();
    
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
    BPTR    parent  = NULL;
    STRPTR  initial = NULL;
    
    if (!ExecuteCommand_Initialize()) cleanup(_(MSG_ERROR_CLASSES));
    
    if (argc == 0)
    {
        struct WBStartup *startup = (struct WBStartup *) argv;
        
        if (startup->sm_NumArgs > 1)
        {
            parent  = startup->sm_ArgList[1].wa_Lock;
            initial = startup->sm_ArgList[1].wa_Name;
        }
    }
            
    application = ExecuteCommandObject,
        MUIA_ExecuteCommand_Parent,  (IPTR) parent,
        MUIA_ExecuteCommand_Initial, (IPTR) initial,
    End;
    
    if (application != NULL)
    {
        DoMethod(application, MUIM_Application_Execute);
        MUI_DisposeObject(application);
    }
    
    cleanup(NULL);
    
    return 0; /* make compiler happy */
}
