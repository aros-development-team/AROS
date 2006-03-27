/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: NewShell CLI Command
    Lang: English
*/

/******************************************************************************


    NAME

        NewShell

    SYNOPSIS

        WINDOW,FROM

    LOCATION

        Workbench:C

    FUNCTION

        Create a new shell in a new console window. This window will become
        the active one. The new shell inherits most attributes of the parent
        shell like the current directory, stack size, prompt and so on.
        However, it is completely independent of the parent shell.
	    The window belonging to the new shell may be specified by
        using the WINDOW keyword.

    INPUTS

        WINDOW  --  Specification of the shell window

	            X         --  number of pixels from the left edge of 
		                  the screen
		    Y         --  number of pixels from the top edge of 
		                  the screen
		    WIDTH     --  width of the shell window in pixels
		    HEIGHT    --  height of the shell window in pixels
		    TITLE     --  text to appear in the shell window's 
		                  title bar
		    AUTO      --  the window automatically appears when the
		                  program needs input or output
		    ALT       --  the window appears in the specified size
		                  and position when the zoom gadget is clicked
		    BACKDROP  --  the window is a backdrop window
		    CLOSE     --  include a close gadget
		    INACTIVE  --  the window is not made active when opened
		    NOBORDER  --  the window is borderless, only the size,
		                  depth and zoom gadgets are available
		    NOCLOSE   --  the window has no close gadget
		    NODEPTH   --  the window has no depth gadget
		    NODRAG    --  the window cannot be drag; implies NOCLOSE
		    NOSIZE    --  the window has no size gadget
		    SCREEN    --  name of a public screen to open the window on
		    SIMPLE    --  if the window is enlarged the text expands to
		                  fill the available space
		    SMART     --  if the window is enlarged the text will not
                                  expand
		    WAIT      --  the window can only be closed by selecting
                                  the close gadget or entering CTRL-\.


        FROM    --  File to execute before resorting to normal shell
	            operations. If nothing is specified S:Shell-Startup
		    is used.

    RESULT

    NOTES

    EXAMPLE

        NewShell "CON:10/10/640/480/My own shell/CLOSE"

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <dos/filesystem.h>
#include <aros/asmcall.h>
#include <string.h>

#include <aros/shcommands.h>

AROS_SH2(NewShell, 41.1,
AROS_SHA(STRPTR, ,WINDOW, ,"CON:10/10/640/480/AROS-Shell/CLOSE"),
AROS_SHA(STRPTR, ,FROM,   ,"S:Shell-Startup"))
{
    AROS_SHCOMMAND_INIT

    BPTR from = Open(SHArg(FROM),   FMF_READ);
    BPTR win  = Open(SHArg(WINDOW), FMF_READ);

    LONG rc = RETURN_FAIL;


    if (win)
    {
	struct TagItem tags[] =
        {
            { SYS_Asynch,      TRUE       },
	    { SYS_Background,  FALSE      },
	    { SYS_Input,       (IPTR)win  },
	    { SYS_Output,      (IPTR)NULL },
	    { SYS_Error,       (IPTR)NULL },
	    { SYS_ScriptInput, (IPTR)from },
	    { SYS_UserShell,   TRUE       },
	    { TAG_DONE,        0          }
        };

        rc = SystemTagList("", tags);
	if (rc != -1)
	{
	    win  = NULL;
	    from = NULL;
	}
	else
	    rc = RETURN_FAIL;
    }
    else
    {
        PrintFault(IoErr(), "NewShell");
    }

    Close(win);
    Close(from);

    return rc;

    AROS_SHCOMMAND_EXIT
}
