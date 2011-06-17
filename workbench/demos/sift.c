;/* sift.c - Execute me to compile me with SAS C 5.10
LC -b1 -cfistq -v -j73 sift.c
Blink FROM LIB:c.o,sift.o TO sift LIBRARY LIB:LC.lib,LIB:Amiga.lib
quit
*/

/*
Copyright (c) 1992 Commodore-Amiga, Inc.

This example is provided in electronic form by Commodore-Amiga, Inc. for
use with the "Amiga ROM Kernel Reference Manual: Libraries", 3rd Edition,
published by Addison-Wesley (ISBN 0-201-56774-1).

The "Amiga ROM Kernel Reference Manual: Libraries" contains additional
information on the correct usage of the techniques and operating system
functions presented in these examples.	The source and executable code
of these examples may only be distributed in free electronic form, via
bulletin board or as part of a fully non-commercial and freely
redistributable diskette.  Both the source and executable code (including
comments) must be included, without modification, in any copy.	This
example may not be published in printed form or distributed with any
commercial product.  However, the programming techniques and support
routines set forth in these examples may be used in the development
of original executable software products for Commodore Amiga computers.

All other rights reserved.

This example is provided "as-is" and is subject to change; no
warranties are made.  All use is at your own risk. No liability or
responsibility is assumed.
*/

/*
AROS NOTE

This example had to be changed, because it

a) uses obsolete includes, which are not included in the AROS distribution and
b) is not fully compatible with gcc.

All changes are indicated by AROS NOTE.
*/

/*
*
* sift.c:	Takes any IFF file and tells you what's in it.  Verifies syntax and all that cool stuff.
*
* Usage: sift -c		; For clipboard scanning
*    or  sift <file>		; For DOS file scanning
*
* Reads the specified stream and prints an IFFCheck-like listing of the contents of the IFF file, if any.
* Stream is a DOS file for <file> argument, or is the clipboard's primary clip for -c.
* This program must be run from a CLI.
*
*/

#include <exec/types.h>
#include <exec/memory.h>
/*
    AROS NOTE: In the Commodore version of sift the following include was
    <libraries/dos.h>. But because <libraries/dos.h> is considered obsolete
    and AROS doesn't include any obsolete includes or definitions, it was
    changed to <dos/dos.h>.
*/
#include <dos/dos.h>
#include <libraries/iffparse.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/iffparse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef LATTICE
int CXBRK(void) { return(0); }     /* Disable Lattice CTRL/C handling */
int chkabort(void) { return(0); }  /* really */
#endif

#define MINARGS 2

/*
    AROS NOTE: The version was changed from 37.1 to 37.2 and the current date
    was included. The date was missing in the Commodore version.
    The original string was: "\0$VER: sift 37.1"
*/
UBYTE vers[] = "\0$VER: sift 37.2 (6.3.97)";       /* 2.0 Version string for c:Version to find */
UBYTE usage[] = "Usage: sift IFFfilename (or -c for clipboard)";

void PrintTopChunk (struct IFFHandle *);  /* proto for our function */

/*
 * Text error messages for possible IFFERR_#? returns from various IFF routines.  To get the index into
 * this array, take your IFFERR code, negate it, and subtract one.
 *  idx = -error - 1;
 */
char	*errormsgs[] = {
	"End of file (not an error).", "End of context (not an error).", "No lexical scope.",
	"Insufficient memory.", "Stream read error.", "Stream write error.",
	"Stream seek error.", "File is corrupt.", "IFF syntax error.",
	"Not an IFF file.", "Required call-back hook missing.", "Return to client.  You should never see this."
};

struct Library *IFFParseBase;

int main(int argc, char **argv)
{
    struct IFFHandle	*iff = NULL;
    long		error;
    short		cbio = 0;

	/* if not enough args or '?', print usage */
	if(((argc)&&(argc<MINARGS))||(argv[argc-1][0]=='?'))
		{
		printf("%s\n", usage);
		goto bye;
		}

	/* Check to see if we are doing I/O to the Clipboard. */
	cbio = (argv[1][0] == '-'  &&  argv[1][1] == 'c');

	if (!(IFFParseBase = OpenLibrary ("iffparse.library", 0L)))
		{
		puts("Can't open iff parsing library.");
		goto bye;
		}

	/* Allocate IFF_File structure. */
	if (!(iff = AllocIFF ()))
		{
		puts ("AllocIFF() failed.");
		goto bye;
		}

	/*
	 * Internal support is provided for both AmigaDOS files, and the clipboard.device.  This bizarre
	 * 'if' statement performs the appropriate machinations for each case.
	 */
	if (cbio)
		{
		/*
		 * Set up IFF_File for Clipboard I/O.
		 */
		if (!(iff->iff_Stream =
				(IPTR) OpenClipboard (PRIMARY_CLIP)))
			{
			puts ("Clipboard open failed.");
			goto bye;
			}
		InitIFFasClip (iff);
		}
	else
		{
		/* Set up IFF_File for AmigaDOS I/O.  */
/*
    AROS NOTE: Added cast. This is necessary, because Open returns a BPTR,
    but iff_Stream is declared as IPTR, which is in fact an unsigned integer.
    IPTR is used in AROS instead of ULONG, if the corresponding field stores
    a value, which may be used as pointer.
*/
		if (!(iff->iff_Stream = (IPTR)Open (argv[1], MODE_OLDFILE)))
			{
			/*
			    AROS NOTE: Show what error it was */
			PrintFault (IoErr (), "File open failed");
			goto bye;
			}
		InitIFFasDOS (iff);
		}

	/* Start the IFF transaction. */
/*
    AROS NOTE: Added extra parentheses around "error = OpenIFF()". Gcc wants
    this for some reason.
*/
	if ((error = OpenIFF (iff, IFFF_READ)))
		{
		puts ("OpenIFF failed.");
		goto bye;
		}

	while (1)
		{
		/*
		 * The interesting bit.  IFFPARSE_RAWSTEP permits us to have precision monitoring of the
		 * parsing process, which is necessary if we wish to print the structure of an IFF file.
		 * ParseIFF() with _RAWSTEP will return the following things for the following reasons:
		 *
		 * Return code: 		Reason:
		 * 0				Entered new context.
		 * IFFERR_EOC			About to leave a context.
		 * IFFERR_EOF			Encountered end-of-file.
		 * <anything else>		A parsing error.
		 */
		error = ParseIFF (iff, IFFPARSE_RAWSTEP);

		/*
		 * Since we're only interested in when we enter a context, we "discard" end-of-context
		 * (_EOC) events.
		 */
		if (error == IFFERR_EOC)
			continue;
		else if (error)
			/*
			 * Leave the loop if there is any other error.
			 */
			break;


		/* If we get here, error was zero. Print out the current state of affairs. */
		PrintTopChunk (iff);
		}

	/*
	 * If error was IFFERR_EOF, then the parser encountered the end of
	 * the file without problems.  Otherwise, we print a diagnostic.
	 */
	if (error == IFFERR_EOF)
		puts ("File scan complete.");
	else
		printf ("File scan aborted, error %ld: %s\n",
			error, errormsgs[-error - 1]);

bye:
	if (iff) {
		/* Terminate the IFF transaction with the stream.  Free all associated structures. */
		CloseIFF (iff);

		/*
		 * Close the stream itself.
		 */
		if (iff->iff_Stream)
		{
			if (cbio)
				CloseClipboard ((struct ClipboardHandle *)
						iff->iff_Stream);
			else
/*
    AROS NOTE: Added cast. See above for reasons.
*/
				Close ((BPTR)iff->iff_Stream);
		}

		/* Free the IFF_File structure itself. */
		FreeIFF (iff);
		}
	if (IFFParseBase)       CloseLibrary (IFFParseBase);

	exit (RETURN_OK);
}

void
PrintTopChunk (iff)
struct IFFHandle *iff;
{
	struct ContextNode	*top;
	short			i;
	char			idbuf[5];

	/* Get a pointer to the context node describing the current context. */
	if (!(top = CurrentChunk (iff)))
		return;

	/*
	 * Print a series of dots equivalent to the current nesting depth of chunks processed so far.
	 * This will cause nested chunks to be printed out indented.
	 */
	for (i = iff->iff_Depth;  i--; )
		printf (". ");

	/* Print out the current chunk's ID and size. */

	/* Print the current chunk's type, with a newline. */
	puts (IDtoStr (top->cn_Type, idbuf));
}
