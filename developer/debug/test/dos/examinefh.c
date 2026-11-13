/*
    Copyright (C) 1995-2015, The AROS Development Team. All rights reserved.
*/

#include <dos/dos.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    BPTR fh;
    LONG result = RETURN_OK;

    if (argc < 2)
    {
        printf("Usage: %s <file name>\n", argv[0]);
        return RETURN_FAIL;
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
                printf("Filename   = %s\n", fib->fib_FileName);
                printf("Protection = 0x%08X\n",
                    (unsigned)fib->fib_Protection);
            }
            else
            {
                printf("ExamineFH() failed, ioerr = %d\n", (int)IoErr());
                result = RETURN_FAIL;
            }

            if (strcasecmp(fib->fib_FileName, FilePart(argv[1])) != 0)
            {
                printf("File name from FIB does not match"
                    " file name from argument\n");
                result = RETURN_ERROR;
            }

            FreeDosObject(DOS_FIB, fib);
        }
        else
        {
            printf("couldn't allocate fileinfoblock\n");
            result = RETURN_FAIL;
        }

        Close(fh);
    }
    else
    {
        printf("Couldn't open file\n");
        result = RETURN_FAIL;
    }

    return result;
}
