/*
    Copyright © 2006-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: IconX WB script starter
    Lang: English
*/

/******************************************************************************


    NAME

        IconX

    SYNOPSIS

        FILE/A

    LOCATION

        C:

    FUNCTION

        Starts DOS scripts from Workbench. In order to use it you need an icon for
        your script. Set 'IconX' as default tool.

    INPUTS

        FILE - The script filename to execute.
        
        Tooltypes for script icon:
            WINDOW            -- Specification of the shell window
                                 default: con:0/50//80/IconX/Auto
            STACK=n           -- default: 40960
            USERSHELL=YES|NO  -- default: YES
            WAIT=n            -- Wait n seconds before closing window (default 2)
            DELAY=n           -- Wait n/50 seconds before closing window

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

        Execute
        
    INTERNALS

    HISTORY

******************************************************************************/

//#define DEBUG 1

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/icon.h>
#include <proto/alib.h>

#include <workbench/startup.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* some default values */
#define DEFWINDOW "con:0/50//80/IconX/Auto"
#define DEFSTACK  (40960)
#define DEFWAIT   (2 * 50) // two seconds
#define DEFUSHELL (TRUE)

#define BASECOMMAND "EXECUTE "

const TEXT version[] = "\0$VER: IconX 41.3 (23.1.2012)";
int __forceerrorrequester = 1;
static TEXT errbuffer[255];


void displayMsg(LONG code)
{
    if (code)
    {
        Fault(code, "IconX", errbuffer, sizeof(errbuffer));
        struct EasyStruct es = {sizeof(struct EasyStruct), 0,
            "Error", errbuffer, "OK"};
        EasyRequest(0, &es, 0);
    }
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



STRPTR BuildCommandLine(struct WBStartup *startup)
{
    const struct WBStartup *wbsstate = startup;
    STRPTR                  buffer   = NULL;
    ULONG                   length   = 2 /* NULL + '\n' */ + strlen(BASECOMMAND);
    int i;

    /*-- Calculate length of resulting string ------------------------------*/
    for (i = 1 ; i < wbsstate->sm_NumArgs ; i++)
    {
        BPTR lock = Lock((STRPTR) wbsstate->sm_ArgList[i].wa_Name, ACCESS_READ);
        if (lock != BNULL)
        {
            BPTR cd   = CurrentDir(lock);
            STRPTR path = AllocateNameFromLock(lock);
            if (path != NULL)
            {
                length += 3 /* space + 2 '"' */ + strlen(path);
                FreeVec(path);
            }
            UnLock(lock);
            CurrentDir(cd);
        }
    }

    /*-- Allocate space for command line string ----------------------------*/
    buffer = AllocVec(length, MEMF_ANY);
    D(bug("[IconX] buffer length: %d\n", length));

    if (buffer != NULL)
    {
        /*-- Build command line --------------------------------------------*/
        strcpy(buffer, BASECOMMAND);

        for (i = 1 ; i < wbsstate->sm_NumArgs ; i++)
        {
            BPTR lock = Lock((STRPTR) wbsstate->sm_ArgList[i].wa_Name, ACCESS_READ);
            if (lock != BNULL)
            {
                BPTR cd   = CurrentDir(lock);
                STRPTR path = AllocateNameFromLock(lock);
                if (path != NULL)
                {
                    strcat(buffer, " \"");
                    strcat(buffer, path);
                    strcat(buffer, "\"");
                    FreeVec(path);
                }
                UnLock(lock);
                CurrentDir(cd);
            }
        }
        strcat(buffer, "\n");
    }
    else
    {
        SetIoErr(ERROR_NO_FREE_STORE);
    }

    return buffer;
}


int main(int argc, char **argv)
{
    LONG               rc            =       RETURN_FAIL;
    STRPTR             filename,
                       commandLine   =       NULL,
                       ixWindow      =       DEFWINDOW;
    LONG               ixWait        =       0,
                       ixStack       =       DEFSTACK;
    BOOL               ixUShell      =       DEFUSHELL;
    BPTR               oldlock       = (BPTR)-1,
                       dirlock       = (BPTR)-1,
                       window        =       BNULL;
    struct DiskObject *dobj          =       NULL;

    D(bug("IconX argc %d\n", argc));

    if (argc != 0)
    {
        displayMsg(ERROR_REQUIRED_ARG_MISSING);
        goto exit;
    }

    struct WBStartup *startup = (struct WBStartup *) argv;
    if (startup->sm_NumArgs < 2)
    {
        displayMsg(ERROR_REQUIRED_ARG_MISSING);
        goto exit;
    }
    
    D(bug("[IconX] startup->sm_NumArgs: %d\n", startup->sm_NumArgs));

    dirlock  = startup->sm_ArgList[1].wa_Lock;
    filename = startup->sm_ArgList[1].wa_Name;

    oldlock = CurrentDir(dirlock);

    /* query diskobject for tooltypes */
    dobj = GetDiskObject(filename);
    if (dobj == NULL)
    {
        struct EasyStruct es = {sizeof(struct EasyStruct), 0,
            "Error", "IconX\nGetDiskObject failed for:\n%s", "OK"};
        EasyRequest(0, &es, 0, filename);
        goto exit;
    }

    if (dobj->do_Type == WBPROJECT)
    {
        const STRPTR *toolarray = (const STRPTR *)dobj->do_ToolTypes;
        STRPTR s;
        if ((s = FindToolType(toolarray, "WINDOW")))
        {
            ixWindow = s;
        }
        if ((s = FindToolType(toolarray, "STACK")))
        {
            ixStack = atol(s);
        }
        if ((s = FindToolType(toolarray, "USERSHELL")))
        {
            if (MatchToolValue(s, "NO"))
            {
                ixUShell = FALSE;
            }
        }
        if ((s = FindToolType(toolarray, "WAIT")))
        {
            ixWait += atol(s) * 50;
        }
        if ((s = FindToolType(toolarray, "DELAY")))
        {
            ixWait += atol(s);
        }
    }
    else
    {
        displayMsg(ERROR_OBJECT_WRONG_TYPE);
        goto exit;
    }
    
    if (ixWait <= 0)
        ixWait = DEFWAIT;

    if (ixStack <= 4096)
        ixStack = DEFSTACK;
    
    D(bug("wait %d stack %d usershell %d window %s\n", ixWait, ixStack, ixUShell, ixWindow));

    D(bug("Building command line\n"));
    commandLine = BuildCommandLine(startup);
    if (commandLine == NULL)
    {
        displayMsg(IoErr());
        goto exit;
    }
    D(bug("[IconX] commandLine: '%s'\n", commandLine));

    window  = Open(ixWindow, MODE_OLDFILE);
    if (window == BNULL)
    {
        /* try to open default window */
        window = Open(DEFWINDOW, MODE_OLDFILE);
    }

    if (window)
    {
        D(bug("[IconX] window ok\n"));
        struct TagItem tags[] =
        {
            { SYS_Asynch,      FALSE        },
            { SYS_Background,  TRUE         },
            { SYS_Input,       (IPTR)window },
            { SYS_Output,      (IPTR)NULL   },
            { SYS_Error,       (IPTR)NULL   },
            { SYS_UserShell,   ixUShell     },
            { NP_StackSize,    ixStack      },
            { TAG_DONE,        0            }
        };

        rc = SystemTagList(commandLine, tags);
        if (rc == -1)
        {
            displayMsg(IoErr());
            rc = RETURN_FAIL;
        }
    }
    else
    {
        displayMsg(IoErr());
        goto exit;
    }

    Delay(ixWait);
    rc = RETURN_OK;

exit:
    Close(window);
    FreeDiskObject(dobj);

    if (oldlock != (BPTR)-1)
        CurrentDir(oldlock);

    return rc;
}

