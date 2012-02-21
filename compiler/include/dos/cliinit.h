/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Document the CLI startup packet
    Lang: English
*/

#ifndef  DOS_CLIINIT_H
#define  DOS_CLIINIT_H

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>

/* Return codes from CliInitRun() and CliInitNewcli()
 */

#define FNF_VALIDFLAGS  (1 << 31)       /* Valid flags */
#define FNF_ASYNCSYSTEM (1 <<  3)       /* Async System() call */
#define FNF_SYSTEM      (1 <<  2)       /* If this a System() call */
#define FNF_USERINPUT   (1 <<  1)       /* User provided input stream */
#define FNF_RUNOUTPUT   (1 <<  0)       /* C:Run provided output stream */

/* Shell startup packets.
 *
 * In truth, only dp_Res1 and dp_Res2 are 'public' - the rest of
 * the arguments here are private, and are decoded by the
 * DOS/CliInitRun() and DOS/CliInitNewcli() routines.
 *
 * NewCli/NewShell: (Asynchonous)
 *
 *   dp_Type = 1
 *   dp_Res1 = 1
 *   dp_Res2 = 0
 *   dp_Arg1 = BPTR Lock of directory (shell must close)
 *   dp_Arg2 = BPTR to StandardInput
 *   dp_Arg3 = BPTR to StandardOuput
 *   dp_Arg4 = BPTR to CurrentInput
 *
 *   dp_Arg5..dp_Arg7 are unused, and have junk data
 *
 * Boot CLI:  (synchronous - this is AROS private - AOS uses a NULL Dos Packet pointer)
 *   dp_Type = -4       // AROS private
 *   dp_Res1 = 1
 *   dp_Res2 = 0
 *   dp_Arg1 = BPTR to Lock of directory (shell must close)
 *   dp_Arg2 = BPTR to StandardInput
 *   dp_Arg3 = BPTR to StandardOuput (can be BNULL if dp_Arg2 is Interactive)
 *   dp_Arg4 = BPTR to CurrentInput  (is BNULL if there is no startup-sequence)
 *
 *   dp_Arg5..dp_Arg7 are unused, and have junk data
 *
 * Run:
 *   NOTE: KS 1.3: bfunc is a BCPL CliInit*() function,
 *         KS 3.x  bfunc is -1
 *   dp_Type = bfunc
 *   dp_Res1 = 0
 *   dp_Res2 = 0
 *   dp_Arg1 = BPTR to the old CommandLineInterface
 *   dp_Arg2 = BPTR to StandardInput
 *   dp_Arg3 = BPTR to StandardOuput
 *   dp_Arg4 = BPTR to CurrentInput
 *   dp_Arg5 = BPTR to Lock of directory (shell must close)
 *   dp_Arg6 = 0 (use CurrentInput/Open("*", MODE_NEWFILE) as Stdin/Stdout,
 *                Close() on exit)
 *
 *   dp_Arg7 is unused, and has junk data
 *
 * System (SYS_Async == FALSE)
 *   dp_Type = -2
 *   dp_Res1 = 0
 *   dp_Res2 = 0
 *   dp_Arg1 = BPTR to the old CommandLineInterface
 *   dp_Arg2 = BPTR to StandardInput
 *   dp_Arg3 = BPTR to StandardOuput
 *   dp_Arg4 = BPTR to CurrentInput
 *   dp_Arg5 = BPTR to Lock of directory (shell must close)
 *   dp_Arg6 = 0 to use CurrentInput/Open("*", MODE_NEWFILE) as Stdin/Stdout
 *                Close() on exit
 *             1 to use StandardInput/StandardOutput,
 *                Do not Close() on exit
 *
 *   dp_Arg7 is unused, and has junk data
 *
 * System (SYS_Asynch == TRUE):
 *   dp_Type = -3
 *   dp_Res1 = 0
 *   dp_Res2 = 0
 *   dp_Arg1 = BPTR to the old CommandLineInterface
 *   dp_Arg2 = BPTR to StandardInput
 *   dp_Arg3 = BPTR to StandardOuput
 *   dp_Arg4 = BPTR to CurrentInput
 *   dp_Arg5 = BPTR to Lock of directory (shell must close)
 *   dp_Arg6 = 0 to use CurrentInput/Open("*", MODE_NEWFILE) as Stdin/Stdout
 *                Close() on exit
 *             1 to use StandardInput/StandardOutput,
 *                Do not Close() on exit
 */

#define CLI_NEWCLI    1
#define CLI_INVALID   0         /* Not a valid CLI type */
#define CLI_RUN      -1
#define CLI_SYSTEM   -2
#define CLI_ASYSTEM  -3
#define CLI_BOOT     -4         /* This is AROS specific. Not in AOS */

#include <proto/dos.h>
#include <proto/exec.h>

/* The following routine handles all the bureaucracy
 * of creating a Shell suitable for placing in L:,
 * including startup packet processing and replies,
 * and pr_CLI initialization and cleanup.
 *
 * Your 'main' function will have a valid Cli()
 * on entry. The following variables will
 * be available to your 'main' routine:
 *
 * ULONG AROS_CLI_Flags;  // As per the DOS/CliInitNewcli and
 *                        // DOS/CliInitRun() Autodocs
 * LONG  AROS_CLI_Type;   // One of the CLI_* types defined above
 *
 * Input() and Output() will be NULL
 *
 * Use the filehandles in Cli() for managing your
 * input, output, and error streams.
 *
 * See Amiga Mail II-65: "Writing a UserShell" for details.
 */
#define AROS_CLI(main) \
    AROS_ENTRY(SIPTR, main, \
            AROS_UFHA(struct DosPacket *, dp, A0), \
            AROS_UFHA(ULONG, unused, D0), \
            struct ExecBase *, SysBase) \
    { \
       AROS_USERFUNC_INIT \
       \
       extern SIPTR _shell_##main(ULONG flags, LONG type D(, struct DosPacket *dp)); \
       struct Process *me = (struct Process *)FindTask(NULL); \
       D(struct DosPacket olddp;)\
       ULONG flags; \
       LONG type; \
       APTR DOSBase; \
       struct CommandLineInterface *cli; \
       SIPTR ret; \
       BPTR dir; \
       BPTR *segArray; \
       \
       if (dp == NULL) { \
           WaitPort(&me->pr_MsgPort); \
           dp = (APTR)(GetMsg(&me->pr_MsgPort)->mn_Node.ln_Name); \
       } \
       DOSBase = OpenLibrary("dos.library", 36); \
       if (DOSBase == NULL || dp->dp_Res2 != 0) { \
           PutMsg(dp->dp_Port, dp->dp_Link); \
           return RETURN_FAIL; \
       } \
       D(CopyMem(dp, &olddp, sizeof(olddp));) \
       flags = dp->dp_Res1 ? CliInitNewcli(dp) : CliInitRun(dp); \
       if (flags & FNF_VALIDFLAGS) { \
           if ((flags & FNF_SYSTEM) && (flags & FNF_ASYNCSYSTEM)) { \
               PutMsg(dp->dp_Port, dp->dp_Link); \
           } \
       } else { \
           /* CliInit*() already returned the packet for me */ \
           if (IoErr() == (SIPTR)me) { \
               return RETURN_ERROR; \
           } \
       } \
       type = dp->dp_Type; \
       me->pr_HomeDir = BNULL; \
       segArray = BADDR(me->pr_SegList); \
       segArray[4] = segArray[3]; \
       segArray[3] = BNULL; \
       segArray[0] = (BPTR)3; \
       ret = _shell_##main(flags, type D(, &olddp)); \
       cli = Cli(); \
       if (flags & FNF_VALIDFLAGS) { \
           D(bug("AROS_CLI: System Exit\n")); \
           if (!(flags & FNF_USERINPUT)) { \
               D(bug("AROS_CLI: Close StandardInput\n")); \
               Close(cli->cli_StandardInput); \
               cli->cli_StandardInput = BNULL; \
           } \
           if ((flags & FNF_RUNOUTPUT)) { \
               D(bug("AROS_CLI: Close StandardOutput\n")); \
               Flush(cli->cli_StandardOutput); \
               Close(cli->cli_StandardOutput); \
               cli->cli_StandardOutput = BNULL; \
           } \
           if (!((flags & FNF_SYSTEM) && (flags & FNF_ASYNCSYSTEM))) { \
               dp->dp_Res1 = cli->cli_ReturnCode; \
               dp->dp_Res2 = cli->cli_Result2; \
               D(bug("AROS_CLI: Reply with %d, %d\n", dp->dp_Res1, dp->dp_Res2)); \
               PutMsg(dp->dp_Port, dp->dp_Link); \
           } \
       } else { \
           D(bug("AROS_CLI: Shell Exit\n")); \
           if (cli->cli_StandardInput != BNULL) { \
               Close(cli->cli_StandardInput); \
               cli->cli_StandardInput = BNULL; \
           } \
           if (cli->cli_StandardOutput != BNULL) { \
               /* A little bit of magic here. */ \
               /* If this was interactive, we don't want to Flush() */ \
               /* the StandardOutput stream. */ \
               if (IsInteractive(cli->cli_StandardOutput)) { \
                   struct FileHandle *fh = ((struct FileHandle *)BADDR(cli->cli_StandardOutput)); \
                  fh->fh_Flags &= ~0x80000000; \
               } \
               Close(cli->cli_StandardOutput); \
               cli->cli_StandardOutput = BNULL; \
           } \
       } \
       dir = CurrentDir(BNULL); \
       if (dir) UnLock(dir); \
       segArray[3] = segArray[4]; \
       segArray[4] = BNULL; \
       CloseLibrary(DOSBase); \
       \
       return ret; \
       \
       AROS_USERFUNC_EXIT \
    } \
    SIPTR _shell_##main(ULONG AROS_CLI_Flags, LONG AROS_CLI_Type D(, struct DosPacket *AROS_CLI_DosPacket))

#endif /* DOS_CLIINIT_H */
