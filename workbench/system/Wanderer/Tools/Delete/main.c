/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 1
#include <aros/debug.h>

#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/workbench.h>
#include <proto/dos.h>

#include <dos/dos.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>

#include <string.h>

int main(int argc, char **argv)
{
    if (argc == 0)
    {
        struct WBStartup *startup = (struct WBStartup *) argv;
        
        if (startup->sm_NumArgs == 2)
        {
            BPTR   lock = startup->sm_ArgList[1].wa_Lock;
            STRPTR name = startup->sm_ArgList[1].wa_Name;
            
            if
            (
                MUI_Request
                (
                    NULL, NULL, 0,
                    "Delete?", "Delete|Cancel",
                    "Are you sure you want to delete \"%s\"?", name
                ) == 1
            )
            {
                BPTR cd = CurrentDir(lock);
                TEXT buffer[512];
                
                DeleteFile(name); // FIXME: check error
                NameFromLock(lock, buffer, 512);
                strcat(buffer, name);
                D(bug("telling wb to update %s\n", buffer));
                UpdateWorkbenchObject(buffer, WBPROJECT, TAG_DONE);
                
                CurrentDir(cd);
            }
        }
        else if (startup->sm_NumArgs > 2)
        {
            ULONG i;
            
            for (i = 1; i < startup->sm_NumArgs; i++)
            {
                OpenWorkbenchObject
                (
                    "SYS:System/Delete",
                    WBOPENA_ArgLock, startup->sm_ArgList[i].wa_Lock,
                    WBOPENA_ArgName, startup->sm_ArgList[i].wa_Name,
                    TAG_DONE
                );
            }
        }
        else
        {
            MUI_RequestA
            (
                NULL, NULL, 0, 
                "Info", "OK", "Error: Wrong number of arguments.", NULL
            );
            return 20;
        }
    }
    else
    {
        PutStr("Info: Must be started from Wanderer.\n");
        return 40;
    }

    return 0;
}
