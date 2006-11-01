/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id:$

    Desc: IconX CLI Command
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
	    USERSHELL=YES|NO  -- default: no
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
#include <string.h>
#include <stdlib.h>

/* some default values */
#define DEFWINDOW "con:0/50//80/IconX/Auto"
#define DEFSTACK  (40960)
#define DEFWAIT   (2 * 50) // two seconds
#define DEFUSHELL (TRUE)

static const char version[] = "\0$VER: IconX 1.0 (1.11.2006) © by The AROS Development Team";
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
    STRPTR path = NULL;
    LONG pathlen;
    BPTR oldlock = (BPTR)-1;
    BPTR dirlock = (BPTR)-1;
    STRPTR pathPartEnd;
    struct DiskObject *dobj = NULL;

    STRPTR ixWindow = DEFWINDOW;
    LONG ixWait = 0;
    LONG ixStack = DEFSTACK;
    BOOL ixUShell = DEFUSHELL;

    BPTR from = NULL;
    BPTR window = NULL;

    D(bug("IconX argc %d\n", argc));

    if (argc != 2)
    {
	displayMsg(ERROR_REQUIRED_ARG_MISSING);
	goto exit;
    }

    D(bug("IconX argv[1] %s\n", argv[1]));

    /* get path part and cd into it */
    pathPartEnd = PathPart(argv[1]);
    pathlen = pathPartEnd - (STRPTR)argv[1];
    path = AllocVec( pathlen + 1, MEMF_ANY|MEMF_CLEAR);
    if (path == NULL)
    {
	displayMsg(ERROR_NO_FREE_STORE);
	goto exit;
    }

    strncpy(path, argv[1], pathlen);
    dirlock = Lock(path, SHARED_LOCK);
    if (dirlock == NULL)
    {
	displayMsg(ERROR_INVALID_LOCK);
	goto exit;
    }

    oldlock = CurrentDir(dirlock);

    filename = FilePart(argv[1]);

    D(bug("Path %s File %s\n", path, filename));

    /* queary diskobject for tooltypes */
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
    FreeVec(path);

    if (oldlock != (BPTR)-1)
	CurrentDir(oldlock);

    if (dirlock != (BPTR)-1)
	UnLock(dirlock);

    return rc;
}

