/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Rename CLI command
    Lang: english
*/

/*****************************************************************************

    NAME

	Rename

    SYNOPSIS

	Rename [{FROM}] <name> [TO|AS] <name> [QUIET]

	FROM/A/M,TO=AS/A,QUIET/S

    LOCATION

	Workbench/c

    FUNCTION

	Renames a directory or file. Rename can also act like the UNIX mv
	command, which moves a file or files to another location on disk.

    INPUTS

	FROM  - The name(s) of the file(s) to rename or move. There may
		be many files specified, this is used when moving files
		into a new directory.

	TO|AS - The name which we wish to call the file.

	QUIET - Suppress any output from the command.

    RESULT

	Standard DOS error codes.

    NOTES

	100% compatible with the standard AmigaOS Rename command.

	The standard Rename command allows the use of pattern matching,
	so this has been implemented in AROS' Rename command.

    EXAMPLE

	Rename letter1.doc letter2.doc letters

	    Moves letter1.doc and letter2.doc to the directory letters.

	Rename ram:a ram:b quiet
	Rename from ram:a to ram:b quiet
	Rename from=ram:a to=ram:b quiet

	    All versions, renames file "a" to "b" and does not output any
	    diagnostic information.

    BUGS

    SEE ALSO

	AmigaDOS Manual 3rd Edition - Pages 131-132 (Rename command)
	AmigaDOS Manual 3rd Edition - Page  277     (Rename DOS command)

    INTERNALS

	Rename() can only move a file to another directory, if and only if
	the to path has the from filename appended to the end.

	e.g.
	    Rename("ram:a", "ram:clipboards/a");
	not
	    Rename("ram:a", "ram:clipboards/");

    HISTORY

	10.04.97    laguest    Corrected 'FromFiles ++' to '*(FromFiles ++)'

	08.04.97    laguest    Corrected the pointer arithmetic error,
			       from (*FromFiles)++ to *(Fromfiles++)

	28.03.97    srittau    Fixes as discussed on mailing-list

	19.03.97    laguest    Initial inclusion to the CVS repository

******************************************************************************/

#include <proto/dos.h>
#include <proto/exec.h>

#include <dos/dos.h>
#include <dos/dosasl.h>
#include <dos/rdargs.h>
#include <exec/memory.h>
#include <exec/types.h>

#define ARG_TEMPLATE	"FROM/A/M,TO=AS/A,QUIET/S"
#define ARG_FROM	0
#define ARG_TO		1
#define ARG_QUIET	2
#define TOTAL_ARGS	3

/* Used to define whether the boolean switches are set or not.
 */
#define NOT_SET 	0

#define MAX_PATH_LEN	512

/* Defines whether MathFirst(), etc has matched a file.
 */
#define MATCHED_FILE	0

static const char version[] = "$VER: Rename 41.1 (22.3.1997)\n";

void DumpStats(char *, char *);

int main(int argc, char *argv[])
{
    struct RDArgs     * rda;
    struct AnchorPath * apath;
    IPTR	      * args[TOTAL_ARGS] = { NULL, NULL, NULL };
    STRPTR	      * FromFiles;
    char	      * FileName;
    char		PathName[MAX_PATH_LEN];
    LONG		MatchResult;
    BOOL		Success;
    int 		Return_Value;

    Return_Value = RETURN_OK;

    apath = (struct AnchorPath *)AllocVec(
	    sizeof(struct AnchorPath) + MAX_PATH_LEN
	    , MEMF_ANY | MEMF_CLEAR
    );

    if (apath)
    {
	/* Make sure DOS knows the buffer size.
	 *
	 * ap_Strlen in AmigaOS.
	 */
	apath->ap_StrLen = MAX_PATH_LEN;

	/* The cast in ReadArgs() stops the compiler from complaining.
	 */
	rda = ReadArgs(ARG_TEMPLATE, (LONG *)args, NULL);
	if (rda)
	{
	    FromFiles = (STRPTR *)args[ARG_FROM];

	    while
	    (
		*FromFiles != NULL
	    &&
		Return_Value == RETURN_OK
	    )
	    {
		MatchResult = MatchFirst((STRPTR)*FromFiles
					 , apath
		);

		do
		{
		    if (MatchResult == MATCHED_FILE)
		    {
			FileName = FilePart((UBYTE *)args[ARG_TO]);

			/* If we do not have a filename at the end of the path.
			 */
			if (*FileName == NULL)
			{

			    /* Obtain a full path with filename.
			     */
			    if
			    (
				AddPart(&PathName[0]
					, (UBYTE *)args[ARG_TO]
					, MAX_PATH_LEN
				)
			    )
			    {
				if
				(
				    AddPart(&PathName[sizeof(args[ARG_TO])]
					    , FilePart(&apath->ap_Buf[0])
					    , MAX_PATH_LEN - sizeof(args[ARG_TO]))
				)
				{

				    if (args[ARG_QUIET] == NOT_SET)
					DumpStats(apath->ap_Buf
						  , &PathName[0]
					);

				    Success = Rename(apath->ap_Buf
						     , &PathName[0]
				    );

				    if (Success == FALSE)
				    {
					Return_Value = RETURN_FAIL;
					PrintFault(IoErr(), "Rename");
				    }
				}
				else
				{
				    Return_Value = RETURN_FAIL;
				    PrintFault(IoErr(), "Rename");
				}
			    }
			    else
			    {
				Return_Value = RETURN_FAIL;
				PrintFault(IoErr(), "Rename");
			    }
			}
			else
			{
			    if (args[ARG_QUIET] == NOT_SET)
				DumpStats(*FromFiles
					  , (UBYTE *)args[ARG_TO]
				);

			    /* Perform a normal rename.
			     */
			    Success = Rename((char *)*FromFiles
					     , (UBYTE *)args[ARG_TO]
			    );

			    if (Success == FALSE)
			    {
				Return_Value = RETURN_FAIL;
				PrintFault(IoErr(), "Rename");
			    }
			}
		    }
		    else
		    {
			Return_Value = RETURN_FAIL;

			PutStr("Can't rename ");
			if (*FileName == NULL) {
			    PutStr(apath->ap_Buf);
			    PutStr(" as ");
			    PutStr(PathName);
			} else {
			    PutStr(*FromFiles);
			    PutStr(" as ");
			    PutStr((STRPTR)args[ARG_TO]);
			}
			PutStr(" because ");
			PrintFault(IoErr(), NULL);
		    }
		}
		while
		(
		    (MatchResult = MatchNext(apath)) ==  MATCHED_FILE
		&&
		    Return_Value == RETURN_OK
		);

		if (Return_Value != RETURN_OK)
		    break;

		MatchEnd(apath);

		FromFiles ++;
	    }
	}

	FreeArgs(rda);
    } else
	Return_Value = RETURN_FAIL;

    FreeVec(apath);

    return(Return_Value);

} /* main */

void DumpStats(char *from, char *to)
{
    IPTR args[2];

    args[0] = (IPTR)from;
    args[1] = (IPTR)to;

    VPrintf("Renaming %s as %s\n", args);

} /* DumpStats */
