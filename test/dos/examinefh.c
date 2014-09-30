/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <proto/dos.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    BPTR fh;

    if (argc < 2)
    {
	printf("Usage: %s <file name>\n", argv[0]);
	return 1;
    }

    fh = Open(argv[1], MODE_OLDFILE);

    if (fh != BNULL)
    {
        struct FileInfoBlock *fib;

	printf("IsInteractive: %d\n", (int)IsInteractive(fh));
	
	fib = AllocDosObject(DOS_FIB, NULL);
        if (fib != NULL)
        {
            if (ExamineFH(fh, fib))
            {
                printf("Got FIB:\n");
		printf("Filename   = %s\n"    , fib->fib_FileName);
		printf("Protection = 0x%08X\n", (unsigned)fib->fib_Protection);
	    }
            else
            {
                printf("examinefh failed, ioerr = %d\n", (int)IoErr());
            }
            FreeDosObject(DOS_FIB, fib);
        }
        else
        {
            printf("couldn't allocate fileinfoblock\n");
        }

        Close(fh);
    }
    else
    {
        printf("Couldn't open file\n");
    }

    return 0;
}
