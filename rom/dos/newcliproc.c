/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/debug.h>

#include <dos/dos.h>
#include <dos/cliinit.h>
#include <dos/stdio.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "dos_intern.h"
#include "dos_newcliproc.h"
#include "fs_driver.h"

ULONG internal_CliInitAny(struct DosPacket *dp, APTR DOSBase)
{
    ULONG flags = 0;
    LONG Type;
    struct CommandLineInterface *oldcli, *cli;
    struct Process *me = (struct Process *)FindTask(NULL);
    BPTR cis, cos, cas, ces, olddir, newdir;
    BOOL inter_in = FALSE, inter_out = FALSE, inter_err = FALSE;
    struct ExtArg *ea = NULL;

    ASSERT_VALID_PROCESS(me);

    D(bug("CliInit%s packet @%p: Process %p\n", (dp->dp_Type > 1) ? "Custom" : (dp->dp_Res1 ? "Newcli" : "Run"), dp, me));
    D(bug("\tdp_Type: %p \n", (APTR)(IPTR)dp->dp_Type));
    D(bug("\tdp_Res1: %p (is NewCli/NewShell?)\n", (APTR)dp->dp_Res1));
    D(bug("\tdp_Res2: %p (is Invalid?)\n", (APTR)dp->dp_Res2));
    D(bug("\tdp_Arg1: %p (%s)\n", (APTR)dp->dp_Arg1, dp->dp_Res1 ? "CurrentDir" : "BPTR to old CLI"));
    D(bug("\tdp_Arg2: %p (StandardInput)\n", (APTR)dp->dp_Arg2));
    D(bug("\tdp_Arg3: %p (StandardOutput)\n", (APTR)dp->dp_Arg3));
    D(bug("\tdp_Arg4: %p (CurrentInput)\n", (APTR)dp->dp_Arg4));
    if (dp->dp_Res1 == DOSFALSE) {
        D(bug("\tdp_Arg5: %p (CurrentDir)\n", (APTR)dp->dp_Arg5));
        D(bug("\tdp_Arg6: %p (Flags)\n", (APTR)dp->dp_Arg6));
    }

    Type = dp->dp_Type;
#ifdef __mc68000
    /* If dp_Type > 1, assume it's the old BCPL style of
     * packet, where dp->dp_Type pointed to a BCPL callback to run
     */
    if (Type > 1) {
        extern void BCPL_thunk(void);
        LONG ret;
        APTR old_RetAddr = me->pr_ReturnAddr;
        D(bug("%s: Calling custom BCPL CliInit routine @%p\n",__func__, Type));
        ret = AROS_UFC8(LONG, BCPL_thunk,
                AROS_UFCA(BPTR,   MKBADDR(dp), D1),
                AROS_UFCA(ULONG,  0,           D2),
                AROS_UFCA(ULONG,  0,           D3),
                AROS_UFCA(ULONG,  0,           D4),
                AROS_UFCA(APTR, me->pr_Task.tc_SPLower, A1),
                AROS_UFCA(APTR, me->pr_GlobVec, A2),
                AROS_UFCA(APTR, &me->pr_ReturnAddr, A3),
                AROS_UFCA(LONG_FUNC, (IPTR)Type, A4));
        D(bug("%s: Called custom BCPL CliInit routine @%p => 0x%08x\n",__func__, Type, ret));
        if (ret > 0) {
            D(bug("%s: Calling custom BCPL reply routine @%p\n",__func__, ret));
            ret = AROS_UFC8(ULONG, BCPL_thunk,
                    AROS_UFCA(BPTR,   IoErr(), D1),
                    AROS_UFCA(ULONG,  0,       D2),
                    AROS_UFCA(ULONG,  0,       D3),
                    AROS_UFCA(ULONG,  0,       D4),
                    AROS_UFCA(APTR, me->pr_Task.tc_SPLower, A1),
                    AROS_UFCA(APTR, me->pr_GlobVec, A2),
                    AROS_UFCA(APTR, &me->pr_ReturnAddr, A3),
                    AROS_UFCA(LONG_FUNC, (IPTR)ret, A4));
            ret = 0;
        }
        me->pr_ReturnAddr = old_RetAddr;
        D(bug("%s: Called custom BCPL reply routine @%p => 0x%08x\n",__func__, Type, ret));
        return ret;
    }
#endif

    /* Create a new CLI if needed
     */
    cli = Cli();
    if (cli == NULL) {
        D(bug("%s: Creating a new pr_CLI\n", __func__));
        cli = AllocDosObject(DOS_CLI, NULL);
        if (cli == NULL) {
            if (dp) {
                ReplyPkt(dp, DOSFALSE, ERROR_NO_FREE_STORE);
                SetIoErr((SIPTR)me);
            } else {
                SetIoErr(ERROR_NO_FREE_STORE);
            }
            return 0;
        }
        me->pr_CLI = MKBADDR(cli);
        me->pr_Flags |= PRF_FREECLI;
        addprocesstoroot(me, DOSBase);
    }

    if (dp->dp_Res1) {
        /* C:NewCLI =  Res1 = 1, dp_Arg1 = CurrentDir, dp_Arg5 = unused    , dp_Arg6 = unused
         */

        oldcli = BNULL;

        /* dp_Arg1 a Lock() on the desired directory
         */
        newdir = (BPTR)dp->dp_Arg1;
    } else {
        /* C:Run    =  Res1 = 0,  dp_Arg1 = OldCLI,     dp_Arg5 = CurrentDir, dp_Arg6 = 0 */

        /* dp_Arg1 a BPTR to the old CommandLineInterface
         */
        oldcli = BADDR(dp->dp_Arg1);

        /* dp_Arg5 a Lock() on the desired directory
         */
        newdir = (BPTR)dp->dp_Arg5;
    }

    olddir = CurrentDir(newdir);
    if (olddir && (me->pr_Flags & PRF_FREECURRDIR)) {
        UnLock(olddir);
    }

    /* dp_Arg2 is the StandardInput  */
    cis = (BPTR)dp->dp_Arg2;

    /* dp_Arg3 is the COS override */
    cos = (BPTR)dp->dp_Arg3;

    /* dp_Arg7 is AROS extension information */
    ea = (struct ExtArg *)dp->dp_Arg7;
    ces = ea->ea_CES;

    /* dp_Arg4 contains the CurrentInput */
    cas = (BPTR)dp->dp_Arg4;

    D(bug("%s: Setting cli_StandardInput from dp_Arg2\n", __func__));
    if (dp->dp_Res1 == 0 && dp->dp_Arg6) {
        flags |= FNF_USERINPUT;
    }

    if (cis == BNULL)
        cis = cas;

    if (cis == BNULL) {
        SetIoErr((SIPTR)me);
        goto exit;
    }

    if (IsInteractive(cis)) {
        D(bug("%s: Setting ConsoleTask based on cli_StandardInput\n", __func__));
        SetConsoleTask(((struct FileHandle *)BADDR(cis))->fh_Type);
    }
    D(bug("%s: pr_ConsoleTask = %p\n", __func__, GetConsoleTask()));

    if (!cos) {
        D(bug("%s: No StandardOutput provided. Synthesize one.\n", __func__));
        if (GetConsoleTask()) {
            D(bug("%s: StandardOutput based on current console\n", __func__));
            cos = Open("*", MODE_NEWFILE);
        } else {
            D(bug("%s: StandardOutput is NIL:\n", __func__));
            cos = Open("NIL:", MODE_NEWFILE);
        }
        if (cos == BNULL) {
            SetIoErr((SIPTR)me);
            goto exit;
        }
        flags |= FNF_RUNOUTPUT;
    }

    if (!ces) {
        D(bug("%s: No StandardError provided. Synthesize one.\n", __func__));
        if (GetConsoleTask()) {
            D(bug("%s: StandardError based on current console\n", __func__));
            ces = Open("*", MODE_NEWFILE);
        } else {
            D(bug("%s: StandardError is NIL:\n", __func__));
            ces = Open("NIL:", MODE_NEWFILE);
        }
        if (ces == BNULL) {
            SetIoErr((SIPTR)me);
            goto exit;
        }
        me->pr_Flags |= PRF_CLOSECLIERROR;
    }

    if (ea->ea_Flags & EAF_CLOSECES)
        me->pr_Flags |= PRF_CLOSECLIERROR;

    cli->cli_CurrentInput   = cas ? cas : cis;
    cli->cli_StandardInput  = cis;

    cli->cli_StandardOutput =
    cli->cli_CurrentOutput  = cos;

    cli->cli_StandardError  = ces;
    /* Compatibility with Amiga Shells which are not aware of cli_StandardError */
    me->pr_CES = cli->cli_StandardError;

    if (IsInteractive(cli->cli_StandardInput)) {
        D(bug("%s: cli_StandardInput is interactive\n", __func__));
        fs_ChangeSignal(cli->cli_StandardInput, me, DOSBase);
        SetVBuf(cli->cli_StandardInput, NULL, BUF_LINE, -1);
        inter_in = TRUE;
    }
    if (IsInteractive(cli->cli_StandardOutput)) {
        D(bug("%s: cli_StandardOutput is interactive\n", __func__));
        fs_ChangeSignal(cli->cli_StandardOutput, me, DOSBase);
        SetVBuf(cli->cli_StandardOutput, NULL, BUF_LINE, -1);
        inter_out = TRUE;
    }
    if (IsInteractive(cli->cli_StandardError)) {
        D(bug("%s: cli_StandardError is interactive\n", __func__));
        fs_ChangeSignal(cli->cli_StandardError, me, DOSBase);
        SetVBuf(cli->cli_StandardError, NULL, BUF_LINE, -1);
        inter_err = TRUE;
    }

    if (oldcli) {
        D(bug("%s: Using old CLI %p\n", __func__, oldcli));
        SetPrompt(AROS_BSTR_ADDR(oldcli->cli_Prompt));
        SetCurrentDirName(AROS_BSTR_ADDR(oldcli->cli_SetName));
        cli->cli_DefaultStack = oldcli->cli_DefaultStack;
        cli->cli_CommandDir = internal_CopyPath(oldcli->cli_CommandDir, DOSBase);
    } else {
        D(bug("%s: Initializing CLI\n", __func__));
        SetPrompt("%N> ");
        SetCurrentDirName("SYS:");
        cli->cli_DefaultStack = AROS_STACKSIZE / CLI_DEFAULTSTACK_UNIT;
    }

    AROS_BSTR_setstrlen(cli->cli_CommandFile, 0);
    AROS_BSTR_setstrlen(cli->cli_CommandName, 0);
    cli->cli_FailLevel = 10;
    cli->cli_Module = BNULL;

    cli->cli_Background = (inter_out && inter_in && inter_err) ? DOSFALSE : DOSTRUE;

    D(bug("%s: cli_CurrentInput   = %p\n", __func__, cli->cli_CurrentInput));
    D(bug("%s: cli_StandardInput  = %p\n", __func__, cli->cli_StandardInput));
    D(bug("%s: cli_StandardOutput = %p\n", __func__, cli->cli_StandardOutput));

    D(bug("%s: cli_Interactive = %p\n", __func__, cli->cli_Interactive));
    D(bug("%s: cli_Background  = %p\n", __func__, cli->cli_Background));

    SetIoErr(0);

    D(bug("+ flags:%p\n", flags));
    if (dp->dp_Res1 == 0 || !(inter_in && inter_out && inter_err))
        flags |= FNF_VALIDFLAGS;

    D(bug("- flags:%p\n", flags));
    switch (Type) {
    case CLI_ASYSTEM: flags = FNF_ASYNCSYSTEM | FNF_RUNOUTPUT;
                      /* Fallthrough */
    case CLI_SYSTEM:  flags |= FNF_VALIDFLAGS | FNF_SYSTEM;
                      cli->cli_Background = TRUE;
                      break;
    case CLI_RUN:     cli->cli_Background = TRUE;
                      /* Fallthrough */
    case CLI_NEWCLI:  flags = 0;
                      break;
    case CLI_BOOT:    flags |= FNF_VALIDFLAGS | FNF_USERINPUT;
                      break;
    default:          break;
    }
    D(bug("= flags:%p (Type %d)\n", flags, Type));

exit:
    if (!(flags & FNF_VALIDFLAGS))
        PutMsg(dp->dp_Port, dp->dp_Link);
    FreeMem(ea, sizeof(struct ExtArg));

    D(bug("%s: Flags = 0x%08x\n", __func__, flags));

    return flags;
}
