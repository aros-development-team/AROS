/*
    Copyright © 2006-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: IconX WB script starter
    Lang: English
*/

/******************************************************************************


    NAME

        IconX

    SYNOPSIS


    LOCATION

        Workbench:C

    FUNCTION

        Starts DOS scripts from Workbench. In order to use it you need an icon for
	your script. Set 'IconX' as default tool.

    INPUTS

        Tooltypes for script icon:
	    WINDOW	      -- Specification of the shell window
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

    INTERNALS

    HISTORY

******************************************************************************/

#define DEBUG 1

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/icon.h>

#include <workbench/startup.h>

#include <string.h>
#include <stdlib.h>

/* some default values */
#define DEFWINDOW "con:0/50//80/IconX/Auto"
#define DEFSTACK  (40960)
#define DEFWAIT   (2 * 50) // two seconds
#define DEFUSHELL (TRUE)

static const char version[] = "\0$VER: IconX 1.1 (24.03..2007) © by The AROS Development Team";
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


int main(int argc, char **argv)
{
    LONG rc = RETURN_FAIL;
    STRPTR filename;
    BPTR oldlock = (BPTR)-1;
    BPTR dirlock = (BPTR)-1;
    struct DiskObject *dobj = NULL;

    STRPTR ixWindow = DEFWINDOW;
    LONG ixWait = 0;
    LONG ixStack = DEFSTACK;
    BOOL ixUShell = DEFUSHELL;

    BPTR from = NULL;
    BPTR window = NULL;

    D(bug("IconX argc %d\n", argc));

    if (argc != 0)
    {
	displayMsg(ERROR_REQUIRED_ARG_MISSING);
	goto exit;
    }

    struct WBStartup *startup = (struct WBStartup *) argv;
    if (startup->sm_NumArgs != 2)
    {
	displayMsg(ERROR_REQUIRED_ARG_MISSING);
	goto exit;
    }

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

    from = Open(filename, MODE_OLDFILE);
    if (from == NULL)
    {
	displayMsg(IoErr());
	goto exit;
    }

    window  = Open(ixWindow, MODE_OLDFILE);
    if (window == NULL)
    {
	/* try to open default window */
	window = Open(DEFWINDOW, MODE_OLDFILE);
    }

    if (window)
    {
	struct TagItem tags[] =
	{
	    { SYS_Asynch,      FALSE        },
	    { SYS_Background,  TRUE         },
	    { SYS_Input,       (IPTR)window },
	    { SYS_Output,      (IPTR)NULL   },
	    { SYS_Error,       (IPTR)NULL   },
	    { SYS_ScriptInput, (IPTR)from   },
	    { SYS_UserShell,   ixUShell     },
	    { NP_StackSize,    ixStack      },
	    { TAG_DONE,        0            }
	};

	rc = SystemTagList("", tags);
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

