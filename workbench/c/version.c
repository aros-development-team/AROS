/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Version CLI command
    Lang: english
*/
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <dos/datetime.h>
#include <clib/dos_protos.h>
#include <clib/utility_protos.h>
#include <utility/tagitem.h>
#include <utility/utility.h>
#include <clib/aros_protos.h>
#include <stdio.h>

static const char version[] = "$VER: Version 1.1 (4.10.96)\n";

UBYTE Buffer[256];
UBYTE Name[64];
ULONG Version;
ULONG Revision;
ULONG Day;
ULONG Month;
ULONG Year;

int main (int argc, char ** argv)
{
    IPTR args[1] = { 0L };
#define ARG_File	((UBYTE *)args[0])

    struct RDArgs *rda;
    LONG error=0;
    IPTR pargs[6];

    rda = ReadArgs ("File", (IPTR *)&args, NULL);

    if (rda != NULL)
    {
	BPTR file;

	if (ARG_File)
	{
	    file = Open (ARG_File, MODE_OLDFILE);

	    if (file)
	    {
		LONG c;
		LONG len;

		while ((c = FGetC (file)) != EOF)
		{
		    if (c == '$')
		    {

    #define NEXT_CHAR(expect)                               \
			if ((c = FGetC (file)) == EOF)      \
			    break;			    \
							    \
			if (c != expect)                    \
			    continue

			NEXT_CHAR('V');
			NEXT_CHAR('E');
			NEXT_CHAR('R');
			NEXT_CHAR(':');

			while ((c = FGetC (file)) != EOF)
			    if (c != ' ' && c != '\t')
				break;

			UnGetC (file, c);
			Flush (file);
			Read (file, Buffer, sizeof (Buffer));

			for (len=0; len<sizeof(Name)-1; len ++)
			{
			    if (Buffer[len] == ' ' || Buffer[len] == '\t')
				break;

			    Name[len] = Buffer[len];
			}

			Name[len] = 0;

			while (Buffer[len] != ' ' && Buffer[len] != '\t')
			    len ++;

			c = StrToLong (&Buffer[len], &Version);
			len += c+1;
			c = StrToLong (&Buffer[len], &Revision);
			len += c+1;

			while (Buffer[len] != '(' && len < sizeof(Buffer))
			    len ++;

			if (Buffer[len] == '(')
			    len ++;

			c = StrToLong (&Buffer[len], &Day);
			len += c+1;
			c = StrToLong (&Buffer[len], &Month);
			len += c+1;
			c = StrToLong (&Buffer[len], &Year);
			len += c+1;

			if (Year < 85)
			    Year += 2000;
			else if (Year < 100)
			    Year += 1900;

			pargs[0] = (IPTR) Name;
			pargs[1] = Version;
			pargs[2] = Revision;
			pargs[3] = Day;
			pargs[4] = Month;
			pargs[5] = Year;

			VPrintf ("%s %ld.%ld (%ld.%ld.%ld)\n", pargs);
			break;
		    }
		}

		Close (file);
	    }
	    else
		error = RETURN_FAIL;
	}
	else
	{
	    pargs[0] = SysBase->LibNode.lib_Version;
	    pargs[1] = SysBase->LibNode.lib_Revision;

	    VPrintf ("Kickstart %ld.%ld\n", pargs);
	}

	FreeArgs (rda);
    }
    else
	error = RETURN_FAIL;

    if (error)
	PrintFault (IoErr (), "Dir");

    return error;
} /* main */

