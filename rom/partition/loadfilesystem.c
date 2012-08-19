/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

*/
#include "partition_support.h"
#include "platform.h"

/*****************************************************************************

    NAME */
#include <libraries/partition.h>

    AROS_LH1(BPTR, LoadFileSystem,

/*  SYNOPSIS */
    AROS_LHA(struct Node *, handle, A1),

/*  LOCATION */
    struct Library *, PartitionBase, 21, Partition)

/*  FUNCTION
    Load the specified filesystem as DOS segment list.

    INPUTS
    handle - Filesystem handle obtained by FindFileSystemA()

    RESULT
    DOS seglist or NULL in case of failure.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    const struct FSFunctionTable *handler = ((struct FileSysHandle *)handle)->handler;

    return handler->loadFileSystem((struct PartitionBase_intern *)PartitionBase, (struct FileSysHandle *)handle);

    AROS_LIBFUNC_EXIT
}
