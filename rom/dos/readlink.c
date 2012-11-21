/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Read the soft-link information.
    Lang: English
*/
#include "dos_intern.h"
#include <exec/lists.h>
#include <proto/exec.h>

struct ReadLinkDeviceUnit
{
    struct MinNode node;
    struct Device *device;
    struct Unit *unit;
};

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH5(LONG, ReadLink,

/*  SYNOPSIS */
        AROS_LHA(struct MsgPort *, port, D1),
        AROS_LHA(BPTR            , lock, D2),
        AROS_LHA(CONST_STRPTR    , path, D3),
        AROS_LHA(STRPTR          , buffer, D4),
        AROS_LHA(ULONG           , size, D5),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 73, Dos)

/*  FUNCTION
        Read the filename referred to by the soft-linked object contained
        in |path| (relative to the lock |lock|) into the buffer |buffer|.
        The variable |path| should contain the name of the object that
        caused the original OBJECT_IS_SOFT_LINK error.

    INPUTS
        port            - The handler to send the request to.
        lock            - Object that |path| is relative to.
        path            - Name of the object that caused the error.
        buffer          - Buffer to fill with resolved filename.
        size            - Length of the buffer.

    RESULT
        >= 0    length of resolved filename in case of success
        == -1   failure, see IoErr() for more information
        == -2   buffer size was too small to store resolved filename

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        MakeLink()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG status;

    status = dopacket4(DOSBase, NULL, port, ACTION_READ_LINK, lock, (SIPTR)path, (SIPTR)buffer, size);

    return status;

    AROS_LIBFUNC_EXIT
} /* ReadLink */
