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

#include "locale.h"

STRPTR AllocateNameFromLock(BPTR lock);

int main(int argc, char **argv)
{
	int result = RETURN_OK;

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
                    _(MSG_DELETE_REQUEST), _(MSG_DELETE_CHOICE),
                    _(MSG_REQUEST), (IPTR) name
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
                                _(MSG_ERROR), _(MSG_OK),
                                _(MSG_COULDNT_DELETE)
                            );
                        }
                    
                        FreeVec(buffer);
                    }
                    else
                    {
                        MUI_Request
                        (
                            NULL, NULL, 0,
                            _(MSG_ERROR), _(MSG_OK),
                            _(MSG_COULDNT_ALLOCATE)
                        );
                    }
                }
                else
                {
                    MUI_Request
                    (
                        NULL, NULL, 0,
                        _(MSG_ERROR), _(MSG_OK),
                        _(MSG_COULDNT_LOCK)
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
                    "WANDERER:Tools/Delete",
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
                _(MSG_INFO), _(MSG_OK), _(MSG_WRONG_ARGS), NULL
            );
            result = RETURN_ERROR;
        }
    }
    else
    {
        PutStr(_(MSG_WB_ONLY));
        result = RETURN_FAIL;
    }
    return result;
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
