/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Break - send a signal to a process.
    Lang: english
 */

/*****************************************************************************

    NAME
        Break

    FORMAT
        Break <process> [ALL|C|D|E|F]

    SYNOPSIS
        PROCESS/N,PORT,NAME/K,ALL/S,C/S,D/S,E/S,F/S

    LOCATION
        C:

    FUNCTION
        BREAK sends one or more signals to a CLI process.
        The argument PROCESS specifies the numeric ID of the CLI process that
        you wish to send the signal to. The STATUS command will list all currently
        running CLI processes along with ther ID.
        You can also specify a public port name and send signal's to the
        port's task.

        You can send all signals at once via option ALL or any combination of the
        flags CTRL-C, CTRL-D, CTRL-E and CTRL-F by their respective options.
        When only the CLI process ID is specified the CTRL-C signal will be sent.

        The effect of using the BREAK command is the same as selecting
        the console window of a process and pressing the relevant key
        combination.

        The normal meaning of the keys is:
            CTRL-C	-	Halt a process
            CTRL-D	-	Halt a shell script
            CTRL-E	-	Close a process' window
            CTRL-F	-	Make active the process' window

        Not all programs respond to these signals, however most should
        respond to CTRL-C.

    EXAMPLE

        1.SYS:> BREAK 1

            Send the CTRL-C signal to the process numbered 1.

        1.SYS:> BREAK 4 E

            Send the CTRL-E signal to the process numbered 4.

        1.SYS:> BREAK NAME c:dhcpclient

            Send the CTRL-C signal to the process named "c:dhcpclient".

******************************************************************************/

#include <exec/types.h>
#include <exec/tasks.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/dos.h>

#define ARG_TEMPLATE    "PROCESS/N,PORT,NAME/K,C/S,D/S,E/S,F/S,ALL/S"
#define ARG_PROCESS     0
#define ARG_PORT        1
#define ARG_NAME        2
#define ARG_C           3
#define ARG_D           4
#define ARG_E           5
#define ARG_F           6
#define ARG_ALL         7
#define TOTAL_ARGS      8

const TEXT version[] = "$VER: Break 42.1 (8.12.2007)";

static const char exthelp[] =
        "Break: Send break signal(s) to a CLI process\n"
        "\tPROCESS/N    signal receiver's CLI process number\n"
        "\tPORT         Portname for the Process to set flags for\n"
		"\tNAME/K       Name of the process (as shown by tasklist)\n"
        "\tC/S          send CTRL-C signal\n"
        "\tD/S          send CTRL-D signal\n"
        "\tE/S          send CTRL-E signal\n"
        "\tF/S          send CTRL-F signal\n"
        "\tALL/S        send all signals\n";

int __nocommandline = 1;


int
main(void)
{
	struct RDArgs *rd, *rda = NULL;

	IPTR args[TOTAL_ARGS] = { };

	int error = 0;

	if ((rda = AllocDosObject(DOS_RDARGS, NULL)))
	{
		rda->RDA_ExtHelp = (STRPTR) exthelp;

		if ((rd = ReadArgs(ARG_TEMPLATE, (IPTR *) args, rda)))
		{
			struct Process *pr = NULL;
			Forbid();
			if (args[ARG_PROCESS])
				pr = FindCliProc(*(IPTR *) args[ARG_PROCESS]);
			else if (args[ARG_PORT])
			{
				struct MsgPort *MyPort;
				if ((MyPort = (struct MsgPort*) FindPort((STRPTR) args[ARG_PORT])) != NULL)
				{
					pr = (struct Process*) MyPort->mp_SigTask;
				}
			}
			else if (args[ARG_NAME])
			{
				pr = (struct Process *) FindTask((UBYTE *)args[ARG_NAME]);
			}
			if (pr != NULL)
			{
				ULONG
				mask = 0;

				/* Figure out the mask of flags to send. */
				if (args[ARG_ALL])
				{
					mask = SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D
					       | SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_F;
				}
				else
				{
					mask = (args[ARG_C] != 0 ? SIGBREAKF_CTRL_C : 0)
					       | (args[ARG_D] != 0 ? SIGBREAKF_CTRL_D : 0)
					       | (args[ARG_E] != 0 ? SIGBREAKF_CTRL_E : 0)
					       | (args[ARG_F]!= 0 ? SIGBREAKF_CTRL_F : 0);

					if (0 == mask)
					{
						mask = SIGBREAKF_CTRL_C; /* default */
					}
				}

				Signal((struct Task *) pr, mask);
				Permit();
			}
			else
			{
				/* There is no relevant error code, OBJECT_NOT_FOUND
				 * is a filesystem error, so we can't use that... */
				Permit();
				pr = (struct Process *) FindTask(NULL);

				BPTR errStream = (pr->pr_CES != BNULL)
				                 ? pr->pr_CES
						 : Output();

				if (args[ARG_PROCESS])
				{
					VFPrintf(errStream, "Break: Process %ld does not exist.\n", (APTR) args[ARG_PROCESS]);
				}
				else if (args[ARG_PORT])
				{
					FPrintf(errStream, "Break: Port \"%s\" does not exist.\n", (LONG) args[ARG_PORT]);
				}
				else if (args[ARG_NAME])
				{
					FPrintf(errStream, "Break: Task \"%s\" does not exist.\n", (LONG) args[ARG_NAME]);
				}
				else
				{
					FPuts(errStream, "Break: Either PROCESS, PORT or NAME is required.\n");
				}
				error = -1;
			}

			FreeArgs(rd);
		} /* ReadArgs() ok */
		else
		{
			error = IoErr();
		}

		FreeDosObject(DOS_RDARGS, rda);
	} /* Got rda */
	else
	{
		error = IoErr();
	}

	if (error != 0 && error != -1)
	{
		PrintFault(error, "Break");
		return RETURN_FAIL;
	}

	SetIoErr(0);

	return 0;
}
