/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Copy CLI command
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
/* #include <clib/aros_protos.h> */
#include <string.h>

static const char version[] = "$VER: Copy 1.0 (4.10.1996)\n";

UBYTE Buffer[4096];
APTR mem;
ULONG size;

int CopyFile (UBYTE * src, UBYTE * dest)
{
    BPTR file_in, file_out;
    IPTR pargs[5];
    int ret = TRUE;
    LONG read_size;

    file_in = file_out = (BPTR)NULL;

    file_in = Open (src, MODE_OLDFILE);

    if (!file_in)
    {
	pargs[0] = (IPTR)src;
	VPrintf ("Cannot open file %s for reading: ", pargs);
	PrintFault (IoErr (), "");
	goto error_exit;
    }

    file_out = Open (dest, MODE_NEWFILE);

    if (!file_out)
    {
	pargs[0] = (IPTR)dest;
	VPrintf ("Cannot open file %s for writing: ", pargs);
	PrintFault (IoErr (), "");
	goto error_exit;
    }

    pargs[0] = (IPTR)src;

    VPrintf ("    %s", pargs);
    Flush (Output ());

    while ((read_size = Read (file_in, mem, size)) > 0)
	Write (file_out, mem, size);

    VPrintf ("...\n", NULL);

    goto ok_exit;

error_exit:
    ret = FALSE;

ok_exit:
    if (file_in)
	Close (file_in);

    if (file_out)
	Close (file_out);

    return ret;
}

int main (int argc, char ** argv)
{
    IPTR args[3];
#define ARG_Source	((UBYTE **)args[0])
#define ARG_Dest	((UBYTE *)args[1])
    struct RDArgs *rda;
    LONG error=0;
    IPTR pargs[5];
    struct FileInfoBlock * fib;
    BPTR lock;

    fib = NULL;
    mem = NULL;
    lock = (BPTR)NULL;

    rda = ReadArgs ("Source/M/A,Dest/A", (IPTR *)&args, NULL);

    if (rda != NULL)
    {
	LONG t;
	BOOL DestIsDir;

	fib = AllocMem (sizeof (struct FileInfoBlock), MEMF_ANY);

	if (!fib)
	{
	    VPrintf ("Not enough memory\n", NULL);
	    goto error_exit;
	}

	for (size=0x10000L; size; size >>= 1)
	    if ((mem = AllocMem (size, MEMF_ANY)))
		break;

	if (!size)
	{
	    VPrintf ("Not enough memory\n", NULL);
	    goto error_exit;
	}

	DestIsDir = FALSE;

	lock = Lock (ARG_Dest, SHARED_LOCK);

	if (!lock)
	{
	    if (IoErr() != ERROR_OBJECT_NOT_FOUND)
	    {
		pargs[0] = (IPTR)ARG_Dest;
		VPrintf ("Cannot lock %s: ", pargs);
		PrintFault (IoErr (), "");
		goto error_exit;
	    }
	}
	else
	{
	    Examine (lock, fib);

	    UnLock (lock);
	    lock = (BPTR)NULL;

	    if (fib->fib_DirEntryType > 0) /* Directory ? */
		DestIsDir = TRUE;
	}

	if (!DestIsDir)
	{
	    if (ARG_Source[1])
	    {
		VPrintf ("Destination is not a directory\n", NULL);
		goto error_exit;
	    }
	    else
	    {
		if (!CopyFile (ARG_Source[0], ARG_Dest))
		    goto error_exit;
	    }
	}
	else
	{
	    UBYTE * ptr;

	    strcpy (Buffer, ARG_Dest);

	    if (*Buffer)
	    {
		ptr = Buffer + strlen (Buffer) - 1;

		if (*ptr == ':' || *ptr == '/')
		    ptr ++;
		else
		{
		    ptr ++;
		    *ptr ++ = '/';
		}
	    }
	    else
		ptr = Buffer;

	    for (t=0; ARG_Source[t]; t++)
	    {
		strcpy (ptr, ARG_Source[t]);

		if (!CopyFile (ARG_Source[0], ARG_Dest))
		    goto error_exit;
	    }
	}

	FreeArgs (rda);
    }
    else
    {
	PrintFault (IoErr (), "Copy");
	goto error_exit;
    }

    goto ok_exit;

error_exit:
    error = RETURN_ERROR;

ok_exit:
    if (lock)
	UnLock (lock);

    if (mem)
	FreeMem (mem, size);

    if (fib)
	FreeMem (fib, sizeof (struct FileInfoBlock));

    return error;
}

