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

#include "executecommand.h"

int main(int argc, char **argv)
{
    Object *application;
        
    if (!ExecuteCommand_Initialize()) return 20; // FIXME: better error handling
    
    if (argc == 0)
    {
        struct WBStartup *startup = (struct WBStartup *) argv;
        
        if (startup->sm_NumArgs > 1)
        {
            application = ExecuteCommandObject,
                MUIA_ExecuteCommand_Lock, (IPTR) startup->sm_ArgList[1].wa_Lock,
                MUIA_ExecuteCommand_Name, (IPTR) startup->sm_ArgList[1].wa_Name,
            EndBoopsi;
        }
        else
        {
            application = ExecuteCommandObject, EndBoopsi;
        }
        
        if (application != NULL)
        {
            DoMethod(application, MUIM_Application_Execute);
            MUI_DisposeObject(application);
        }
    }
    else
    {
        Printf("ERROR: Cannot be started from CLI."); 
    }
    
    return 0;
}
