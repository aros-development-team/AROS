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

STRPTR AllocateNameFromLock(BPTR lock);

int main(int argc, char **argv)
{
    if (argc == 0)
    {
        struct WBStartup *startup = (struct WBStartup *) argv;
        
        if (startup->sm_NumArgs == 2)
        {
            BPTR   parent = startup->sm_ArgList[1].wa_Lock;
            STRPTR name   = startup->sm_ArgList[1].wa_Name;
            
            if
            (
                MUI_Request
                (
                    NULL, NULL, 0,
                    "Delete?", "Delete|Cancel",
                    "Are you sure you want to delete \"%s\"?", (IPTR) name
                ) == 1
            )
            {
                BPTR cd   = CurrentDir(parent);
                BPTR lock = Lock(name, ACCESS_WRITE);
                
                if (lock != NULL)
                {
                    STRPTR buffer = AllocateNameFromLock(lock);
                    
                    UnLock(lock);
                    
                    if (buffer != NULL)
                    {
                        if (DeleteFile(name))
                        {
                            UpdateWorkbenchObject(buffer, WBPROJECT, TAG_DONE);
                        }
                        else
                        {
                            MUI_Request
                            (
                                NULL, NULL, 0,
                                "Error", "OK",
                                "Could not delete file."
                            );
                        }
                    
                        FreeVec(buffer);
                    }
                    else
                    {
                        MUI_Request
                        (
                            NULL, NULL, 0,
                            "Error", "OK",
                            "Could not allocate memory."
                        );
                    }
                }
                else
                {
                    MUI_Request
                    (
                        NULL, NULL, 0,
                        "Error", "OK",
                        "Could not lock file for deletion."
                    );
                }
                
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
                    WBOPENA_ArgLock, (IPTR) startup->sm_ArgList[i].wa_Lock,
                    WBOPENA_ArgName, (IPTR) startup->sm_ArgList[i].wa_Name,
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
        PutStr("Error: Must be started from Wanderer.\n");
        return 40;
    }

    return 0;
}

STRPTR AllocateNameFromLock(BPTR lock)
{
    ULONG  length = 512;
    STRPTR buffer = NULL;
    BOOL   done   = FALSE;
    
    while (!done)
    {
        if (buffer != NULL) FreeVec(buffer);
        
        buffer = AllocVec(length, MEMF_ANY);
        if (buffer != NULL)
        {
            if (NameFromLock(lock, buffer, length))
            {
                done = TRUE;
                break;
            }
            else
            {
                if (IoErr() == ERROR_LINE_TOO_LONG)
                {
                    length += 512;
                    continue;
                }
                else
                {
                    break;
                }                
            }
        }
        else
        {
            SetIoErr(ERROR_NO_FREE_STORE);
            break;
        }
    }
    
    if (done)
    {
        return buffer;
    }
    else
    {
        if (buffer != NULL) FreeVec(buffer);
        return NULL;
    }
}
