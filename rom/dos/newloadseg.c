/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: DOS function NewLoadSeg()
    Lang: english

*/
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <utility/tagitem.h>
#include <proto/dos.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(BPTR, NewLoadSeg,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, file, D1),
        AROS_LHA(struct TagItem *, tags, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 128, Dos)

/*  FUNCTION
        Loads an executable file into memory via LoadSeg() and takes
        additional actions based upon the supplied tags.

    INPUTS
        file - NULL terminated name of the file
        tags - pointer to the tagitems

    RESULT
        Handle to the loaded executable or 0 if the load failed.
        IoErr() gives additional information in that case.

    NOTES

    EXAMPLE

    BUGS
        As there are no tags currently defined, all this function does is
        call LoadSeg()

    SEE ALSO
        LoadSeg(), UnLoadSeg(), InternalLoadSeg(), InternalUnloadSeg()

    INTERNALS

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

  return LoadSeg(file);

  AROS_LIBFUNC_EXIT
} /* NewLoadSeg */
