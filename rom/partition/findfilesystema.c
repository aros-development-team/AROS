/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

*/
#include "partition_support.h"
#include "platform.h"

/*****************************************************************************

    NAME */
#include <libraries/partition.h>

    AROS_LH2(struct Node *, FindFileSystemA,

/*  SYNOPSIS */
    AROS_LHA(struct PartitionHandle *, table, A1),
    AROS_LHA(struct TagItem *, taglist, A2),

/*  LOCATION */
   struct Library *, PartitionBase, 20, Partition)

/*  FUNCTION
    Locate a filesystem handler in the partition

    INPUTS
    ph      - PartitionHandle to the table
    taglist - Taglist specifying arguments. Possible tags are:

	      FST_ID   - specify ID of the filesystem.
	      FST_NAME - specify name of the filesystem.

              If more than one condition is specified for the search, logical
              AND will be applied to them. Empty taglist will give you the first
              filesystem in the list.

    RESULT
    Abstract handle of a filesystem or NULL if the filesystem with the given
    parameters was not located in the partition.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (table->table)
    {
    	struct PTFunctionTable *handler = table->table->handler;

        if (handler->findFileSystem)
            return handler->findFileSystem(PartitionBase, table, taglist);
    }

    return NULL;

    AROS_LIBFUNC_EXIT
}
