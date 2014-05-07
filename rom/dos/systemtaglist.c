/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <aros/debug.h>
#include <utility/tagitem.h>
#include <dos/dostags.h>
#include <proto/utility.h>
#include <dos/dosextens.h>
#include <aros/asmcall.h>
#include <exec/ports.h>

#include "dos_newcliproc.h"
#include "dos_intern.h"
#include "fs_driver.h"

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
        that conflict with SystemTagList(). Currently, these are

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
        command - program and arguments as a string
        tags    - see <dos/dostags.h>. Note that both SystemTagList() tags and
                  tags for CreateNewProc() may be passed.

    RESULT
        The return code of the command executed or -1 if the command could
        not run because the shell couldn't be created. If the command is not
        found, the shell will return an error code, usually RETURN_ERROR.

    NOTES
        You must close the input and output filehandles yourself (if needed)
        after System() returns if they were specified via SYS_Input or
        SYS_Output (also, see below).

        You may NOT use the same filehandle for both SYS_Input and SYS_Output.
        If you want them to be the same CON: window, set SYS_Input to a
        filehandle on the CON: window and set SYS_Output to NULL. Then the
        shell will automatically set the output by opening CONSOLE: on that
        handler. Note that SYS_Error also follows this rule, so passing it
        set to NULL will automatically set the error by opening CONSOLE: on
        that handler.

        If you specified SYS_Asynch, both the input and the output filehandles
        will be closed when the command is finished (even if this was your
        Input() and Output()).

    EXAMPLE

    BUGS

    SEE ALSO
        Execute(), CreateNewProc(), Input(), Output(), <dos/dostags.h>

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Process *me = (struct Process *)FindTask(NULL);
    BPTR   cis = BNULL;
    BPTR   sis;
    BPTR   sos;
    BPTR   ses = BNULL;
    BPTR   shellseg = BNULL;
    STRPTR resShell    = BNULL;
    STRPTR shellName   = "Custom Shell";
    APTR   entry;
    BOOL sis_opened    = FALSE;
    BOOL cis_opened    = FALSE;
    BOOL sos_opened    = FALSE;
    BOOL ses_opened    = FALSE;
#ifdef __mc68000
    BOOL isCLI         = TRUE;  /* Default is BCPL compatible CLI */
#else
    BOOL isCLI         = FALSE; /* Default is C style Shell */
#endif
    BOOL isBoot        = TRUE;
    BOOL isCustom      = FALSE;
    BOOL isAsynch      = FALSE;
    BOOL isBackground  = TRUE;
    LONG cliType       = CLI_INVALID;
    ULONG commandlen;
    LONG rc            = -1;

    struct TagItem *tags2 = tags;
    struct TagItem *newtags, *tag;

    ASSERT_VALID_PROCESS(me);
    D(bug("SystemTagList('%s',%p)\n", command, tags));

    sis = Input();
    sos = Output();

    while ((tag = NextTagItem(&tags2)))
    {
        D(bug("Tag=%08x Data=%08x\n", tag->ti_Tag, tag->ti_Data));
        switch (tag->ti_Tag)
        {
            case SYS_ScriptInput:
                cis = (BPTR)tag->ti_Data;
                break;

            case SYS_Input:
                sis = (BPTR)tag->ti_Data;
                break;

            case SYS_Output:
                sos = (BPTR)tag->ti_Data;
                break;

            case SYS_Error:
                ses = (BPTR)tag->ti_Data;
                break;

            case SYS_CustomShell:
                resShell = (STRPTR)tag->ti_Data;
                isCustom = TRUE;
                isCLI   = FALSE;
                break;

            case SYS_UserShell:
                isBoot = !tag->ti_Data;
                isCustom = FALSE;
                break;

            case SYS_Asynch:
                isAsynch = tag->ti_Data ? TRUE : FALSE;
                break;

            case SYS_Background:
                isBackground = tag->ti_Data ? TRUE : FALSE;
                break;

            case SYS_CliType:
                cliType = (LONG)tag->ti_Data;
                break;
        }
    }

    /* Validate cliType */
    switch (cliType) {
    case CLI_SYSTEM:
        isAsynch = FALSE;
        isBackground = TRUE;
        break;
    case CLI_BOOT:
        isAsynch = TRUE;
        isBackground = FALSE;
        break;
    case CLI_NEWCLI:
        isAsynch = TRUE;
        isBackground = FALSE;
        break;
    case CLI_RUN:
    case CLI_ASYSTEM:
        isAsynch = TRUE;
        isBackground = TRUE;
        break;
    case CLI_INVALID:
        /* default */
        if (isBackground)
            cliType = isAsynch ? CLI_ASYSTEM : CLI_SYSTEM;
        else
            cliType = isAsynch ? CLI_NEWCLI : CLI_BOOT;
        break;
    default:
        /* invalid */
        goto end;
    }

    /* Set up the streams */
    if (sis == (BPTR)SYS_DupStream)
    {
        sis = OpenFromLock(DupLockFromFH(Input()));
        if (!sis) goto end;

        sis_opened = TRUE;
    }
    if (sis == BNULL) {
        sis = Open("NIL:", MODE_OLDFILE);
        if (!sis) goto end;

        sis_opened = TRUE;
    }
    D(bug("[SystemTagList] cli_StandardInput: %p\n", sis));

    if (cis == BNULL || cis == (BPTR)SYS_DupStream)
        cis = sis;
    D(bug("[SystemTagList] cli_CurrentInput: %p\n", cis));

    if (sos == (BPTR)SYS_DupStream)
    {
        sos = OpenFromLock(DupLockFromFH(Output()));
        if (!sos) goto end;

        sos_opened = TRUE;
    }
    D(bug("[SystemTagList] cli_StandardOutput: %p\n", sos));

    if (ses == (BPTR)SYS_DupStream)
    {
        ses = OpenFromLock(DupLockFromFH(me->pr_CES));
        if (!ses) goto end;

        ses_opened = TRUE;
    }
    D(bug("[SystemTagList] cli_StandardError: %p\n", ses));

    /* Inject the arguments, adding a trailing '\n'
     * if the user did not.
     */
    if (command == NULL)
        command = "";

    commandlen = strlen(command);
    if (commandlen) {
        STRPTR cmdcopy = NULL;
        BOOL ok;

        if (command[commandlen-1] != '\n') {
            cmdcopy = AllocVec(commandlen + 2, MEMF_ANY);
            CopyMem(command, cmdcopy, commandlen);
            cmdcopy[commandlen++] = '\n';
            cmdcopy[commandlen] = 0;
            command = cmdcopy;
        }

        ok = vbuf_inject(cis, command, commandlen, DOSBase);
        FreeVec(cmdcopy);

        if (!ok)
            goto end;
    }

    /* Load the shell */
    if (!isCustom) {
        /* Seglist of default shell is stored in RootNode when loaded */
        D(bug("[SystemTagList] Loading standard %s\n", isCLI? "CLI" : "shell"));
        if (isCLI)
            shellseg = findseg_cli(isBoot, DOSBase);
        else
            shellseg = findseg_shell(isBoot, DOSBase);
        /*
         * Set shell process name.
         */
        if (isCLI)
            shellName = "CLI";
        else
            shellName = "Shell";
    } else if (resShell != BNULL) {
        struct Segment *seg;

        /* Get the custom shell from the DOS resident list */
        D(bug("[SystemTagList] Loading custom shell\n"));
        Forbid();
        seg = FindSegment(resShell, NULL, TRUE);
        if (seg)
            shellseg = seg->seg_Seg;
        Permit();

        shellName = resShell;
    }

    if (!shellseg && isCLI) {
        /* We wanted the CLI shell, but it's not available.
         * Let's try to use the Shell instead.
         */
        D(bug("[SystemTagList] No CLI. Attempting to use shell instead\n"));
        shellseg = findseg_shell(isBoot, DOSBase);
        isCLI = FALSE;
    }

    if (!shellseg) {
        D(bug("[SystemTagList] No shell available\n"));
        goto end;
    }

#ifdef __mc68000
    if (isCLI)
    {
        D(bug("[SystemTagList] BCPL 'CLI' shell selected\n"));
        /* On the m68k AOS, the "CLI" shell was always a BCPL
         * program, that ran like a BCPL DOS packet handler.
         */
        extern void BCPL_RunHandler(void);
        entry = BCPL_RunHandler;
    }
    else 
#endif
    {
        D(bug("[SystemTagList] C 'shell' shell selected\n"));
        /* Use the standard C entry point */
        entry = NULL;
    }

    newtags = CloneTagItems(tags);
    if (newtags)
    {
        struct Process *cliproc;

        /* Note: SystemTagList + CliInitNewcli/CliInitRun + AROS_CLI macro manage
         * the creation and destruction of pr_CIS/pr_COS/pr_CES. CrateNewProc
         * logic is not used in such case.
         */
        struct TagItem proctags[] =
        {
            { NP_Priority   , me->pr_Task.tc_Node.ln_Pri    }, /* 0  */
            { NP_Name       , (IPTR)shellName               }, /* 1  */
            { NP_Input      , (IPTR)BNULL                   }, /* 2  */
            { NP_Output     , (IPTR)BNULL                   }, /* 3  */
            { NP_Error      , (IPTR)BNULL                   }, /* 4  */
            { NP_CloseInput , FALSE                         }, /* 5  */
            { NP_CloseOutput, FALSE                         }, /* 6  */
            { NP_CloseError , FALSE                         }, /* 7  */
            { NP_Cli        , ((cliType == CLI_NEWCLI) || (cliType == CLI_ASYSTEM))
                                      ?  TRUE : FALSE       }, /* 8  */
            { NP_WindowPtr  , isAsynch ? (IPTR)NULL :
                              (IPTR)me->pr_WindowPtr        }, /* 9  */
            { NP_Seglist    , (IPTR)shellseg                }, /* 10 */
            { NP_FreeSeglist, FALSE                         }, /* 11 */
            { NP_Synchronous, FALSE                         }, /* 12 */
            { NP_Entry      , (IPTR)entry                   }, /* 13 */
            { NP_CurrentDir , (IPTR)BNULL                   }, /* 14 */
            { NP_ConsoleTask, (IPTR)BNULL,                  }, /* 15 */
            { TAG_END       , 0                             }  /* 16 */
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
            NP_Arguments,
            NP_Synchronous,
            NP_UserData,
            0
        };

        struct DosPacket *dp;

        FilterTagItems(newtags, filterList, TAGFILTER_NOT);

        proctags[sizeof(proctags)/(sizeof(proctags[0])) - 1].ti_Tag  = TAG_MORE;
        proctags[sizeof(proctags)/(sizeof(proctags[0])) - 1].ti_Data = (IPTR)newtags;

        dp = AllocDosObject(DOS_STDPKT, NULL);
        if (dp) {
            cliproc = CreateNewProc(proctags);

            if (cliproc)
            {
                SIPTR oldSignal = 0;
                struct FileHandle *fh = NULL;
                struct ExtArg *ea = AllocMem(sizeof(struct ExtArg), MEMF_PUBLIC | MEMF_CLEAR);

                ea->ea_CES = ses;
                if (isAsynch || ses_opened)
                    ea->ea_Flags |= EAF_CLOSECES;

                D(bug("[SystemTagList] cliType = %d (background=%d, asynch=%d)\n", cliType, isBackground, isAsynch));
#ifdef __mc68000
                /* Trickery needed for BCPL user shells.
                 * This is needed for the trimmed AROS m68k
                 * ROMs that don't have shell.resource, and
                 * load the CLI from a BCPL L:Shell-Seg, like
                 * the one on Workbench 1.3
                 */
                if (isCLI) {
                    extern void BCPL_CliInit_NEWCLI(void);
                    extern void BCPL_CliInit_RUN(void);
                    extern void BCPL_CliInit_SYSTEM(void);
                    extern void BCPL_CliInit_ASYSTEM(void);
                    extern void BCPL_CliInit_SYSTEM(void);
                    extern void BCPL_CliInit_BOOT(void);
                    switch (cliType) {
                    case CLI_NEWCLI:  dp->dp_Type = (LONG)BCPL_CliInit_NEWCLI;  break;
                    case CLI_RUN:     dp->dp_Type = (LONG)BCPL_CliInit_RUN;     break;
                    case CLI_SYSTEM:  dp->dp_Type = (LONG)BCPL_CliInit_SYSTEM;  break;
                    case CLI_ASYSTEM: dp->dp_Type = (LONG)BCPL_CliInit_ASYSTEM; break;
                    case CLI_BOOT:    dp->dp_Type = (LONG)BCPL_CliInit_BOOT;    break;
                    default: goto end;
                    }
                } else /* Fallthough to the next line for user shells */
#endif
                dp->dp_Type = cliType;
                dp->dp_Res2 = 0;
                dp->dp_Arg2 = (IPTR)sis;        /* Input */
                dp->dp_Arg3 = (IPTR)sos;        /* Output */
                dp->dp_Arg7 = (IPTR)ea;         /* AROS extension information*/
                dp->dp_Arg4 = (IPTR)cis;        /* Arguments & Script*/

                /* The rest of the packet depends on the cliType. */
                if (cliType == CLI_NEWCLI) {
                    /* CliInitNewcli style */
                    dp->dp_Res1 = 1;
                    dp->dp_Arg1 = (IPTR)(me->pr_CurrentDir ? DupLock(me->pr_CurrentDir) : BNULL);
                } else {
                    /* CliInitRun style */
                    dp->dp_Res1 = 0;
                    dp->dp_Arg1 = (IPTR)(__is_process(me) ? MKBADDR(Cli()) : BNULL);
                    dp->dp_Arg5 = (IPTR)(me->pr_CurrentDir ? DupLock(me->pr_CurrentDir) : BNULL);
                    dp->dp_Arg6 = 1;
                }

                /* Migrate the CIS handle to the new process */
                if (!isAsynch && IsInteractive(sis)) {
                    fh = BADDR(sis);

                    if (dopacket(&oldSignal, fh->fh_Type, ACTION_CHANGE_SIGNAL, (SIPTR)fh->fh_Arg1, (SIPTR)&cliproc->pr_MsgPort, 0, 0, 0, 0, 0) == DOSFALSE)
                        oldSignal = 0;
                }

                SendPkt(dp, &cliproc->pr_MsgPort, &me->pr_MsgPort);
                if (WaitPkt() != dp)
                    Alert(AN_QPktFail);

                if (fh && oldSignal) {
                    DoPkt(fh->fh_Type, ACTION_CHANGE_SIGNAL, (SIPTR)fh->fh_Arg1, oldSignal, 0, 0, 0);
                }

                rc = dp->dp_Res1;
                SetIoErr(dp->dp_Res2);

                cis_opened = FALSE;
                if (isAsynch) {
                    sis_opened    =
                    sos_opened    =
                    ses_opened    = FALSE;
                }
            }

            FreeDosObject(DOS_STDPKT, dp);
        }
        FreeTagItems(newtags);
    }

end:
    if (sis_opened)    Close(sis);
    if (cis_opened)    Close(cis);
    if (sos_opened)    Close(sos);
    if (ses_opened)    Close(ses);

    return rc;

    AROS_LIBFUNC_EXIT
} /* SystemTagList */
