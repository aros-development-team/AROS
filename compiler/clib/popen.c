/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function fopen()
    Lang: english
*/

#include <utility/tagitem.h>
#include <proto/dos.h>
#include <fcntl.h>
#include <unistd.h>

#include "__stdio.h"
#include "__open.h"

#include <errno.h>


/*****************************************************************************

    NAME */
#include <stdio.h>

	FILE * popen (

/*  SYNOPSIS */
	const char * command,
	const char * mode)

/*  FUNCTION
	"opens" a process by creating a pipe, spawning a new process and invoking
	the shell.
	
    INPUTS
	command - Pointer to a null terminated string containing the command
	          to be executed by the shell.

	mode - Since a pipe is unidirectional, mode can be only one of

		r: Open for reading. After popen() returns, the stream can
		   be used to read from it, as if it were a normal file stream,
		   in order to get the command's output.

		w: Open for writing. After popen() returns, the stream can
		   be used to write to it, as if it were a normal file stream,
		   in order to provide the command with some input.

    RESULT
	A pointer to a FILE handle or NULL in case of an error. When NULL
	is returned, then errno is set to indicate the error.

    NOTES
        This function must not be used in a shared library or
        in a threaded application.

    EXAMPLE

    BUGS

    SEE ALSO
	fclose(), fread(), fwrite(), pipe(), pclose()

    INTERNALS

    HISTORY
	30.07.2002 falemagn created

******************************************************************************/
{
    int pipefds[2];

    if (!mode || (mode[0] != 'r' && mode[0] != 'w') || mode[1] != '\0')
    {
        GETUSER;

	errno = !mode ? EFAULT : EINVAL;
	return NULL;
    }

    if (pipe(pipefds) == 0)
    {
 	int fdtopass = (mode[0] == 'w');

        struct TagItem tags[] =
        {
            { 0           , NULL                                            },
            { SYS_Asynch  , TRUE                                            },
            { NP_StackSize, Cli()->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT },
            { TAG_DONE    , 0                                               }
        };

	tags[0].ti_Tag  = fdtopass ? SYS_Output : SYS_Input;
	tags[0].ti_Data = (IPTR)__getfdesc(pipefds[fdtopass])->fh;

	if (SystemTagList(command, tags) != -1)
	{
            return fdopen(pipefds[1 - fdtopass], NULL);
	}

	close(pipefds[0]);
	close(pipefds[1]);
    }

    return NULL;
} /* fopen */

