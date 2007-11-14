/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#ifdef DEBUG
#undef DEBUG
#endif
# define  DEBUG 1
# include <aros/debug.h>

#include "dos_newcliproc.h"
#include "dos_intern.h"
#include <utility/tagitem.h>
#include <dos/dostags.h>
#include <proto/utility.h>
#include <dos/dosextens.h>
#include <aros/asmcall.h>
#include <exec/ports.h>

static BPTR DupFH(BPTR fh, LONG mode, struct DosLibrary * DOSBase);

/*****************************************************************************

    NAME */

#include <proto/dos.h>

	AROS_LH2(LONG, SystemTagList,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR    , command, D1),
	AROS_LHA(struct TagItem *, tags,    D2),

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
	NP_Error
	NP_Output
	NP_CloseInput
	NP_CloseOutput
	NP_CloseError
	NP_HomeDir
	NP_Cli
        NP_Arguments
	NP_Synchrounous
	NP_UserData


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

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BPTR   cis = Input(), cos = Output(), ces = Error(), script = NULL;
    BPTR   shellseg = NULL;
    STRPTR cShell   = NULL;
    BOOL script_opened = FALSE;
    BOOL cis_opened    = FALSE;
    BOOL cos_opened    = FALSE;
    BOOL ces_opened    = FALSE;
    BOOL isBoot        = TRUE;
    BOOL isBackground  = TRUE;
    BOOL isAsynch      = FALSE;
    LONG rc            = -1;
    LONG *cliNumPtr    = NULL;

    struct TagItem *newtags, *tags2 = tags, *tag;

    while ((tag = NextTagItem(&tags2)))
    {
        switch (tag->ti_Tag)
	{
	    case SYS_ScriptInput:
	        script = (BPTR)tag->ti_Data;
		break;

	    case SYS_Input:
	        cis = (BPTR)tag->ti_Data;
		break;

	    case SYS_Output:
	        cos = (BPTR)tag->ti_Data;
		break;

	    case SYS_Error:
	        ces = (BPTR)tag->ti_Data;
		break;

	    case SYS_CustomShell:
	        cShell = (STRPTR)tag->ti_Data;
		break;

            case SYS_UserShell:
	        isBoot = !tag->ti_Data;
	        break;

	    case SYS_Background:
                isBackground = tag->ti_Data;
		break;

	    case SYS_Asynch:
		isAsynch = tag->ti_Data;
		break;

	    case SYS_CliNumPtr:
	        cliNumPtr = (LONG *)tag->ti_Data;
		break;
	}
    }

    /* Set up the streams */
    if (!cis)
    {
        cis = Open("NIL:", FMF_READ);
	if (!cis) goto end;

	cis_opened = TRUE;
    }
    else
    if (cis == (BPTR)SYS_DupStream)
    {
        cis = DupFH(Input(), FMF_READ, DOSBase);
	if (!cis) goto end;

	cis_opened = TRUE;
    }

    if (!cos)
    {
        if (IsInteractive(cis))
	    cos = DupFH(cis, FMF_WRITE, DOSBase);
	else
	    cos = Open("*", FMF_WRITE);
;
        if (!cos) goto end;

	cos_opened = TRUE;
    }
    else
    if (cos == (BPTR)SYS_DupStream)
    {
        cos = DupFH(Output(), FMF_WRITE, DOSBase);
	if (!cos) goto end;

	cos_opened = TRUE;
    }

    if (!ces)
    {
        if (IsInteractive(cis))
	    ces = DupFH(cos, FMF_WRITE, DOSBase);
	else
	    ces = Open("*", FMF_WRITE);

        if (!ces) goto end;

	ces_opened = TRUE;
    }
    else
    if (ces == (BPTR)SYS_DupStream)
    {
        ces = DupFH(Output(), FMF_WRITE, DOSBase);
	if (!ces) goto end;

	ces_opened = TRUE;
    }

    /* Load the shell */
#warning implement UserShell and BootShell
    shellseg = LoadSeg("C:Shell");
    if (!shellseg)
        goto end;

    newtags = CloneTagItems(tags);
    if (newtags)
    {
	struct CliStartupMessage csm;
	struct Process *me       = (struct Process *)FindTask(NULL);
	struct Process *cliproc;

	struct TagItem proctags[] =
	{
	    { NP_Entry      , (IPTR) NewCliProc             }, /* 0  */
	    { NP_Priority   , me->pr_Task.tc_Node.ln_Pri    }, /* 1  */
	    
	    /*
		Disabled, because CreateNewProc() already handles NP_StackSize,
		i.e. it uses either AROS_STACKSIZE or the stack from the parent
		process.
	    */
	    // { NP_StackSize  , AROS_STACKSIZE                },
	    { TAG_IGNORE    , 0                             }, /* 2  */
	    { NP_Name       , isBoot ? (IPTR)"Boot Shell" :
	                      isBackground ?
			      (IPTR)"Background CLI" :
			      (IPTR)"New Shell"             }, /* 3  */
	    { NP_Input      , (IPTR)cis                     }, /* 4  */
	    { NP_Output     , (IPTR)cos                     }, /* 5  */
	    { NP_CloseInput , (isAsynch || cis_opened)      }, /* 6  */
	    { NP_CloseOutput, (isAsynch || cos_opened)      }, /* 7  */
	    { NP_Cli        , (IPTR)TRUE                    }, /* 8  */
	    { NP_WindowPtr  , isAsynch ? (IPTR)NULL :
	                      (IPTR)me->pr_WindowPtr        }, /* 9 */
	    { NP_Arguments  , (IPTR)command                 }, /* 10 */
	    { NP_Synchronous, FALSE                         }, /* 11 */
	    { NP_Error      , (IPTR)ces                     }, /* 12 */
	    { NP_CloseError , (isAsynch || ces_opened) &&
            /* 
                Since old AmigaOS programs don't know anything about Error()
                being handled by this function, don't close the Error stream
                if it's the same as the caller's one.
            */
			      ces != Error()                }, /* 13 */
	    { TAG_END       , 0                             }  /* 14 */
	};

	Tag filterList[] =
	{
	    NP_Seglist,
	    NP_FreeSeglist,
	    NP_Entry,
	    NP_Input,
	    NP_Output,
	    NP_CloseInput,
            NP_CloseOutput,
	    NP_CloseError,
	    NP_HomeDir,
	    NP_Cli,
	    NULL
	};

	FilterTagItems(newtags, filterList, TAGFILTER_NOT);

	proctags[sizeof(proctags)/(sizeof(proctags[0])) - 1].ti_Tag  = TAG_MORE;
	proctags[sizeof(proctags)/(sizeof(proctags[0])) - 1].ti_Data = (IPTR)newtags;

	cliproc = CreateNewProc(proctags);

	if (cliproc)
	{
	    csm.csm_Msg.mn_Node.ln_Type = NT_MESSAGE;
	    csm.csm_Msg.mn_Length       = sizeof(csm);
	    csm.csm_Msg.mn_ReplyPort    = &me->pr_MsgPort;

	    csm.csm_CurrentInput = script;
            csm.csm_ShellSeg     = shellseg;
            csm.csm_Background   = isBackground;
	    csm.csm_Asynch       = isAsynch;

	    PutMsg(&cliproc->pr_MsgPort, (struct Message *)&csm);
	    WaitPort(&me->pr_MsgPort);
	    GetMsg(&me->pr_MsgPort);

  	    if (cliNumPtr) *cliNumPtr = csm.csm_CliNumber;

	    rc = csm.csm_ReturnCode;

	    script_opened =
	    cis_opened    =
	    cos_opened    =
	    ces_opened    = FALSE;

	    shellseg = NULL;
	}
	FreeTagItems(newtags);
    }

end:
    UnLoadSeg(shellseg);
    if (script_opened) Close(script);
    if (cis_opened)    Close(cis);
    if (cos_opened)    Close(cos);
    if (ces_opened)    Close(ces);

    return rc;

    AROS_LIBFUNC_EXIT
} /* SystemTagList */

static BPTR DupFH(BPTR fh, LONG mode, struct DosLibrary * DOSBase)
{
    BPTR ret = NULL;

    if (fh)
    {
        BPTR olddir = CurrentDir(fh);
        ret    = Open("", mode);

        CurrentDir(olddir);
    }

    return ret;
}
