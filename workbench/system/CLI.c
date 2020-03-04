/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CLI Command
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
        using the WINDOW tooltype.

    INPUTS

        The attributes are read as tooltypes from the Shell icon.

        WINDOW  --  Specification of the shell window. It must be in the form
                    con:[X]/[Y]/[WIDTH]/[HEIGHT]...

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

        STACK   --  Stack size in Bytes.

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
#include <workbench/startup.h>

//#define DEBUG 1
#include <aros/debug.h>

const TEXT ver[] = "$VER:CLI 1.2 (07.02.2017) \xA9 1995-2017 The AROS Dev Team";
static BPTR olddir = (BPTR)-1;

int main(int argc, char **argv)
{
    struct DiskObject *dobj = NULL;
    LONG rc = RETURN_OK;

    STRPTR winspec = NULL;
    STRPTR fromspec = NULL;
    ULONG stack = 0;

    BPTR win, from;
    BPTR iconlock = BNULL;
    STRPTR iconname = NULL;

    // CLI is a special case, because it is
    // a default tool of an icon with a different name,
    // usually SYS:System/Shell.info.

    // check the wbmessage for the icon name
    // from which we were called
    if (argc == 0)
    {
        struct WBStartup *wbmsg = (struct WBStartup *)argv;
        if (wbmsg->sm_NumArgs == 2)
        {
            iconlock = wbmsg->sm_ArgList[1].wa_Lock;
            iconname = wbmsg->sm_ArgList[1].wa_Name;
            olddir = CurrentDir(iconlock);
        }
    }

    // if we don't have a valid name we try the standard name
    if ((iconname == NULL) || (*iconname == '\0'))
    {
        iconname = "SYS:System/Shell";
    }

    // read the diskobject for the tooltypes
    if ((dobj = GetDiskObject(iconname)) != NULL)
    {
        STRPTR result;
        STRPTR *toolarray = dobj->do_ToolTypes;

        result = FindToolType(toolarray, "STACK");
        if (result)
            StrToLong(result, &stack);

        result = FindToolType(toolarray, "FROM");
        if (result)
            fromspec = result;

        result = FindToolType(toolarray, "WINDOW");
        if (result)
            winspec = result;
    }
    D(bug("[CLI] iconname %s diskobject %p\n", iconname, dobj));

    // sanity checks; set default values
    if (stack < AROS_STACKSIZE)
        stack = AROS_STACKSIZE;

    if (fromspec == NULL)
        fromspec = "S:Shell-Startup";

    if (winspec == NULL)
        winspec = "CON:0/50//130/AROS-Shell/CLOSE";

    D(bug("[CLI] stack %d from %s window %s\n", stack, fromspec, winspec));

    // open the streams for SystemTagList
    from  = Open(fromspec, MODE_OLDFILE);
    win   = Open(winspec, MODE_NEWFILE);

    // launch the Shell
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
            // SystemTagList closes the streams for us
            // when run successfully asynch
            win  = BNULL;
            from = BNULL;
        }
        else
            rc = RETURN_FAIL;
    }
    Close(win);
    Close(from);
    FreeDiskObject(dobj);

    if (olddir != (BPTR)-1)
    {
        CurrentDir(olddir);
        olddir = (BPTR)-1;
    }

    return rc;
}
