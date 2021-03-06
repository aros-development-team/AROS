/*
    Copyright (C) 1995-2015, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    BPTR fh = BNULL;
    LONG result = RETURN_OK, error = 0, size;
    struct FileInfoBlock *fib;

    fib = AllocDosObject(DOS_FIB, NULL);
    if (fib == NULL)
    {
        error = IoErr();
        result = RETURN_FAIL;
    }

    if (error == 0 && argc != 3)
    {
        printf("Usage: %s filename newsize\n", argv[0]);
        error = ERROR_REQUIRED_ARG_MISSING;
        result = RETURN_WARN;
    }

    if (error == 0)
    {
        fh = Open(argv[1], MODE_READWRITE);
        if (fh == BNULL)
        {
            error = IoErr();
            result = RETURN_ERROR;
        }
    }

    if (error == 0)
    {
        size = atol(argv[2]);
        if (SetFileSize(fh, atol(argv[2]), OFFSET_BEGINNING) != size)
        {
            error = IoErr();
            result = RETURN_ERROR;
        }
    }

    if (error == 0)
    {
        if (!ExamineFH(fh, fib))
        {
            error = IoErr();
            result = RETURN_ERROR;
        }
    }

    if (error == 0)
    {
        Printf("New size is %ld bytes\n", fib->fib_Size);
        if (fib->fib_Size != size)
        {
            if (fib->fib_Size > size)
                error = ERROR_OBJECT_TOO_LARGE;
            else
                error = ERROR_UNKNOWN;
            result = RETURN_ERROR;
        }
    }

    if (fib != NULL)
        FreeDosObject(DOS_FIB, fib);
    if (fh != BNULL)
        Close(fh);

    PrintFault(error, "SetFileSize");
    return result;
}
