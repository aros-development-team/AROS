/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h
	9.1.2000    SDuvan  implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *, DOSBase)

    BPTR   cis = NULL, cos = NULL, ces = NULL, script = NULL;
    BPTR   shellseg = NULL;
    STRPTR cShell;
    BOOL script_opened = FALSE;
    BOOL cis_opened    = FALSE;
    BOOL cos_opened    = FALSE;
    BOOL ces_opened    = FALSE;
    BOOL isBoot        = FALSE;
    BOOL isBackground;
    BOOL isAsynch;
    LONG rc = -1;

    struct TagItem *newtags;

    /* Set up the streams */
    script = (BPTR)GetTagData(SYS_ScriptInput, (IPTR)NULL, tags);

    cis  = (BPTR)GetTagData(SYS_Input , (IPTR)Input(), tags);
    if (!cis)
    {
        cis = Open("NIL:", FMF_READ);
	if (!cis) goto end;

	cis_opened = TRUE;
    }

    cos  = (BPTR)GetTagData(SYS_Output , (IPTR)Output(), tags);
    if (!cos)
    {
        if (IsInteractive(cis))
	{
	    BPTR olddir = CurrentDir(cis);
	    cos = Open("", FMF_WRITE);
	    CurrentDir(olddir);
	}
	else
	    cos = Open("*", FMF_WRITE);
;
        if (!cos) goto end;

	cos_opened = TRUE;
    }

    ces  = (BPTR)GetTagData(SYS_Error , (IPTR)Error(), tags);
    if (!ces)
    {
        if (IsInteractive(cis))
	{
	    BPTR olddir = CurrentDir(cis);
	    ces = Open("", FMF_WRITE);
	    CurrentDir(olddir);
	}
	else
	    ces = Open("*", FMF_WRITE);

        if (!ces) goto end;

	ces_opened = TRUE;
    }

    /* Load the shell */
    cShell = (STRPTR)GetTagData(SYS_CustomShell, (IPTR)NULL, tags);
    if (cShell)
    {
	shellseg = LoadSeg(cShell);
    }
    else
#warning implement UserShell and BootShell
    if (!GetTagData(SYS_UserShell, FALSE, tags))
    {
	isBoot   = TRUE;
    }
    shellseg = LoadSeg("C:Shell");
    if (!shellseg)
    {
        goto end;
    }

    isBackground = GetTagData(SYS_Background, TRUE, tags);
    isAsynch     = GetTagData(SYS_Asynch,    FALSE, tags);

    newtags = CloneTagItems(tags);
    if (newtags)
    {
	struct CliStartupMessage csm;
	struct Process *me       = (struct Process *)FindTask(NULL);
	struct Process *cliproc;

	struct TagItem proctags[] =
	{
	    { NP_Entry      , (IPTR)NewCliProc              }, /* 0  */
	    { NP_Priority   , me->pr_Task.tc_Node.ln_Pri    }, /* 2  */
	    { NP_StackSize  , AROS_STACKSIZE                }, /* 3  */
	    { NP_Path       , (IPTR)NULL                    }, /* 4  */
	    { NP_Name       , isBoot ? (IPTR)"Boot Shell" :
	                      isBackground ?
			      (IPTR)"Background CLI" :
			      (IPTR)"New Shell"             }, /* 5  */
	    { NP_Input      , (IPTR)cis                     }, /* 6  */
	    { NP_Output     , (IPTR)cos                     }, /* 7  */
	    { NP_CloseInput , (isAsynch || cis_opened)      }, /* 8  */
	    { NP_CloseOutput, (isAsynch || cos_opened)      }, /* 9  */
	    { NP_Cli        , (IPTR)TRUE                    }, /* 11 */
	    { NP_WindowPtr  , isAsynch ? (IPTR)NULL :
	                      (IPTR)me->pr_WindowPtr        }, /* 12 */
	    { NP_Arguments  , (IPTR)command                 }, /* 13 */
	    { NP_Synchronous, FALSE                         }, /* 14 */
	    { NP_Error      , (IPTR)ces                     }, /* 16 */
	    { NP_CloseError , (isAsynch || ces_opened) &&
              /* Since old AmigaOS programs don't know anything about Error()
              being handled by this function, don't close the Error stream
              if it's the same as the caller's one*/
			      ces != Error()                }, /* 17 */
	    { TAG_END       , 0                             }
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

	proctags[sizeof(proctags)/(sizeof(proctags[0]))].ti_Tag  = TAG_MORE;
	proctags[sizeof(proctags)/(sizeof(proctags[0]))].ti_Data = (IPTR)newtags;

	cliproc = CreateNewProc(proctags);
	if (cliproc)
	{
	    csm.csm_Msg.mn_Node.ln_Type = NT_MESSAGE;
	    csm.csm_Msg.mn_Length       = sizeof(csm);
	    csm.csm_Msg.mn_ReplyPort    = &me->pr_MsgPort;

	    csm.csm_CurrentInput = script;
            csm.csm_ShellSeg     = shellseg;
            csm.csm_Background   = GetTagData(SYS_Background, TRUE, tags);
	    csm.csm_Asynch       = isAsynch;

	    PutMsg(&cliproc->pr_MsgPort, (struct Message *)&csm);
	    WaitPort(&me->pr_MsgPort);
	    GetMsg(&me->pr_MsgPort);

	    {
	        struct TagItem *tag = FindTagItem(SYS_CliNumPtr, tags);
		if (tag) *(LONG *)tag->ti_Data = csm.csm_CliNumber;
 	    }

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
