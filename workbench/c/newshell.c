/*
    (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc:
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
                                  the close gadget is selected or entering
				  CTRL-\.
		    

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
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

static const char version[] = "$VER: newshell 41.1 (14.3.1997)\n";

int main (int argc, char ** argv)
{
    IPTR args[2] = { (IPTR)"CON:10/10/640/480/AROS-Shell/CLOSE",
		     (IPTR)"S:Shell-Startup" };

    struct RDArgs  *rda;
    struct Process *process;
    
    BPTR    in, out, shell, lock;
    STRPTR  s1, s3, buf;
    LONG    error = RETURN_ERROR;
    
    rda = ReadArgs("WINDOW,FROM", args, NULL);

    if (rda != NULL)
    {
	int length;

	s1 = (STRPTR)args[1];
	length = strlen(s1);

	buf = (STRPTR)AllocVec(6 + 2*length, MEMF_ANY);

	if (buf != NULL)
	{
	    CopyMem("FROM \"", buf, 6);

	    s3 = buf + 6;

	    while (*s1 != 0)
	    {
		switch(*s1)
		{
		case '\n':
		    *s3++ = '*';
		    *s3++ = 'n';
		    break;

		case '\"':
		    *s3++ = '*';
		    break;

		case '*':
		    *s3++ = '*';
		    *s3++ = '*';
		    break;
		    
		default:
		    *s3++ = *s1;
		}

		s1++;
	    }

	    *s3++ = '\"';
	    *s3   = 0;

	    shell = LoadSeg("c:shell");
	    
	    if(shell != NULL)
	    {
		out = Open((STRPTR)args[0], MODE_READWRITE);

		if(out != NULL)
		{
		    /* Clone output filehandle */
		    
		    lock = DupLockFromFH(out);

		    if(lock != NULL)
		    {
			in = OpenFromLock(lock);

			if(in != NULL)
			{
			    struct TagItem tags[]=
			    {
				{ NP_Arguments  , 0           },
				{ NP_Input      , 0           },
				{ NP_Output     , 0           },
				{ NP_Error      , 0           },
				{ NP_Seglist    , 0           },
				{ NP_Cli        , 1           },
				{ NP_CopyVars   , (IPTR)TRUE  },
				{ TAG_END       , 0           }
			    };

			    tags[0].ti_Data = (IPTR)buf;
			    tags[1].ti_Data = (IPTR)in;
			    tags[2].ti_Data = (IPTR)out;
			    tags[4].ti_Data = (IPTR)shell;

			    process = CreateNewProc(tags);

			    if (process != NULL)
			    {
				out = in = shell = NULL;
				error = 0;
			    }

			    Close(in);
			}
			else
			{
			    UnLock(lock);
			}
		    }

		    Close(out);
		}

		UnLoadSeg(shell);
	    }

	    FreeVec(buf);
	}

	FreeArgs(rda);
    }
    else
    {
	error = RETURN_FAIL;
    }

    if(error != 0)
    {
	PrintFault(IoErr(), "NewShell");
    }

    return error;
}
