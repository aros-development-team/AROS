/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Delete CLI Command
    Lang: english
*/

/*****************************************************************************

    NAME
	Delete

    SYNOPSIS
	Delete <files> [ALL] [FORCE] [QUIET]

	FILE/M/A,ALL/S,QUIET/S,FORCE/S

    LOCATION
	Workbench/c

    FUNCTION
	Deletes files and directories

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include <proto/dos.h>
#include <proto/exec.h>

#include <dos/dos.h>
#include <dos/dosasl.h>
#include <exec/memory.h>

#define ARG_TEMPLATE    "FILE/M/A,ALL/S,QUIET/S,FORCE/S"
#define ARG_FILE	0
#define ARG_ALL		1
#define ARG_QUIET	2
#define ARG_FORCE	3
#define TOTAL_ARGS      4

/* To define whether a command line switch was set or not.
 */
#define NOT_SET         0

/* Maximum file path length.
 */
#define MAX_PATH_LEN    512

static const char version[] = "$VER: delete 41.2 (6.1.2000)\n";
static /*const*/ char cmdname[] = "Delete";


int Do_Delete(struct AnchorPath *a, STRPTR *files, BOOL all, BOOL quiet, BOOL force);


int main(int argc, char *argv[])
{
	struct RDArgs		* rda;
	struct AnchorPath	* apath;
	LONG			args[TOTAL_ARGS] = {};
	int			Return_Value;

	Return_Value = RETURN_OK;

	apath = AllocVec(sizeof(struct AnchorPath) + MAX_PATH_LEN, MEMF_ANY | MEMF_CLEAR);
	if (apath)
	{
		/* This is ap_Strlen under AmigaOS. */
		apath->ap_Strlen = MAX_PATH_LEN;

		if ((rda = ReadArgs(ARG_TEMPLATE, (LONG *)args, NULL)))
		{
			Return_Value = Do_Delete(apath,
				(STRPTR *)args[ARG_FILE],
				(BOOL)args[ARG_ALL],
				(BOOL)args[ARG_QUIET],
				(BOOL)args[ARG_FORCE]);

			FreeArgs(rda);
		}
		else
		{
			PrintFault(IoErr(), cmdname);
			Return_Value = RETURN_FAIL;
		}

	}
	else
		Return_Value = RETURN_FAIL;

	FreeVec(apath);

	return Return_Value;

} /* main */

/* Defines whether MathFirst(), etc has matched a file.
 */
#define MATCHED_FILE 0

int Do_Delete(struct AnchorPath *a, STRPTR *files, BOOL all, BOOL quiet, BOOL force)
{
	LONG Result;
	int  Return_Value;

	Return_Value = RETURN_OK;

	while (*files != NULL)
	{
		Result = MatchFirst(*files, a);

		do
		{
			if (Result == MATCHED_FILE)
			{
				if (!DeleteFile(a->ap_Buf))
					Return_Value = RETURN_FAIL;
			}
			else
			{
				PrintFault(IoErr(), cmdname);
				Return_Value = RETURN_FAIL;
			}
		} while (((Result = MatchNext(a)) == MATCHED_FILE) && Return_Value == RETURN_OK);

		MatchEnd(a);

		files++;
	}

	return Return_Value;

} /* Do_Delete */
