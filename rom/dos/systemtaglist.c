/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

#include "dos_intern.h"
#include <utility/tagitem.h>
#include <dos/dostags.h>
#include <proto/utility.h>
#include <dos/dosextens.h>

/*****************************************************************************

    NAME */

#include <proto/dos.h>

	AROS_LH2(LONG, SystemTagList,

/*  SYNOPSIS */
	AROS_LHA(STRPTR          , command, D1),
	AROS_LHA(struct TagItem *, tags, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 101, Dos)

/*  FUNCTION

    Execute a command via a shell. As defaults, the process will use the
    current Input() and Output(), and the current directory as well as the
    path will be inherited from your process. If no path is specified, this
    path will be used to find the command.
        Normally, the boot shell is used but other shells may be specified
    via tags. The tags are passed through to CreateNewProc() except those
    who conflict with SystemTagList(). Currently, these are

        NP_Seglist
	NP_FreeSeglist
	NP_Entry
	NP_Input
	NP_Output
	NP_CloseInput
	NP_CloseOutput
	NP_HomeDir
	NP_Cli

    INPUTS

    command  --  program and arguments as a string
    tags     --  see <dos/dostags.h>. Note that both SystemTagList() tags and
                 tags for CreateNewProc() may be passed.

    RESULT

    The return code of the command executed or -1 or if the command could
    not run because the shell couldn't be created. If the command is not
    found, the shell will return an error code, usually RETURN_ERROR.
    
    NOTES

    You must close the input and output filehandles yourself (if needed)
    after System() returns if they were specified via SYS_Input or
    SYS_Output (also, see below).
        You may NOT use the same filehandle for both SYS_Input and SYS_Output.
    If you want them to be the same CON: window, set SYS_Input to a filehandle
    on the CON: window and set SYS_Output to NULL. Then the shell will
    automatically set the output by opening CONSOLE: on that handler.
        If you specified SYS_Asynch, both the input and the output filehandles
    will be closed when the command is finished (even if this was your Input()
    and Output().

    EXAMPLE

    BUGS

    SEE ALSO

    Execute(), CreateNewProc(), Input(), Output(), <dos/dostags.h>

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h
	9.1.2000    SDuvan  implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *, DOSBase)

    BPTR   input, output;
    BOOL   isAsynchronous;
    STRPTR shell, cShell;
    ULONG  type;		/* Type of execution */

    input  = (BPTR)GetTagData(SYS_Input , (IPTR)Input() , tags);
    output = (BPTR)GetTagData(SYS_Output, (IPTR)Output(), tags);

    isAsynchronous = GetTagData(SYS_Asynch, (IPTR)FALSE, tags);
    
#warning  "User shell vs. boot shell -- what's the difference?"

    if((BOOL)GetTagData(SYS_UserShell, FALSE, tags))
	shell = "C:shell";
    else
	shell = "C:shell";
    
    cShell = (STRPTR)GetTagData(SYS_CustomShell, (IPTR)NULL, tags);

    if(cShell != NULL)
	shell = cShell;

    type = (command == NULL) ? RUN_EXECUTE : isAsynchronous ? 
	RUN_SYSTEM : RUN_SYSTEM_ASYNCH;
    
    return ExecCommand(type, command, shell, input, output, tags, DOSBase);

    return RETURN_FAIL;
    AROS_LIBFUNC_EXIT
} /* SystemTagList */
