/*
    Copyright (C) 2017-2021, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <errno.h>

#include "__fdesc.h"
#include "__stdio.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

        int getc_unlocked (

/*  SYNOPSIS */
        FILE * stream)

/*  FUNCTION

    INPUTS
        stream - Read from this stream

    RESULT
        The character read or EOF on end of file or error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        getc(), __posixc_fputc(), putc()

    INTERNALS

******************************************************************************/
{
    return fgetc(stream);
}
