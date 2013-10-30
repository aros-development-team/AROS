/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: NewShell CLI Command
    Lang: English
*/

/******************************************************************************


    NAME

        CLI

    SYNOPSIS

        WINDOW,FROM,STACK

    LOCATION

        SYS:System/

    FUNCTION

        Create a new shell in a new console window. This window will become
        the active one.

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

	STACK   --  Stack size

    RESULT

    NOTES
        As opposed to C:NewShell, this is a Workbench Tool.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include <proto/dos.h>
#include <proto/icon.h>

//#define DEBUG 1
#include <aros/debug.h>

int main(int argc, char **argv)
{
    struct DiskObject *dobj;
    LONG rc = RETURN_FAIL;

    dobj = GetDiskObject("PROGDIR:Shell");
    if (dobj)
    {
        ULONG stack;
        BPTR win, from;
        STRPTR result, winspec, fromspec;
        STRPTR *toolarray = dobj->do_ToolTypes;

        result = FindToolType(toolarray, "STACK");
        if (result)
            StrToLong(result, &stack);
        else
            stack = AROS_STACKSIZE;

        result = FindToolType(toolarray, "FROM");
        if (result)
            fromspec = result;
        else
            fromspec = "S:Shell-Startup";

        result = FindToolType(toolarray, "WINDOW");
        if (result)
            winspec = result;
        else
            winspec = "S:Shell-Startup";

        from  = Open(fromspec, MODE_OLDFILE);
        win   = Open(winspec, MODE_NEWFILE);

        if (stack < AROS_STACKSIZE)
            stack = AROS_STACKSIZE;

        D(bug("[CLI] stack %d from %s window %s\n", stack, fromspec, winspec));

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
                { NP_StackSize,    stack      },
                { TAG_DONE,        0          }
            };

            rc = SystemTagList("", tags);
            if (rc != -1)
            {
                win  = BNULL;
                from = BNULL;
            }
            else
                rc = RETURN_FAIL;
        }

        Close(win);
        Close(from);

        FreeDiskObject(dobj);
    }

    return rc;
}
