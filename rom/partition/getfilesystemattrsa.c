/*
    Copyright (C) 2011-2017, The AROS Development Team. All rights reserved.
*/

#include <proto/utility.h>

#include "partition_support.h"
#include "platform.h"

/*****************************************************************************

    NAME */
#include <utility/tagitem.h>
#include <libraries/partition.h>

        AROS_LH2(void, GetFileSystemAttrsA,

/*  SYNOPSIS */
        AROS_LHA(struct Node *, handle, A1),
        AROS_LHA(const struct TagItem *, taglist, A2),

/*  LOCATION */
        struct Library *, PartitionBase, 22, Partition)

/*  FUNCTION
        get attributes of a partition table

    INPUTS
        handle      - Filesystem handle
        taglist - list of attributes; unknown tags are ignored

    TAGS
        FST_ID      (ULONG *)           - Get 4-character FS ID
        FST_NAME    (STRPTR *)          - Get a pointer to FS name
        FST_FSENTRY (struct FileSysEntry *) - Fill in given FileSysEntry.

    RESULT
        None.

    NOTES
        Name is returned as a pointer to an internally allocated string. You
        should copy it if you want to keep it after the filesystem's
        partition table handle has been closed.

        The following fields in struct FileSysEntry will not be filled in:
          - Node name
          - fse_Handler
          - fse_SegList
        You need to query for the filesystem's name separately and copy it
        into a BSTR yourself, if you need to. Loading the handler is done by
        the LoadFileSystem() function.

    EXAMPLE

    BUGS

    SEE ALSO
        FindFileSystemA()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    const struct FSFunctionTable *handler = ((struct FileSysHandle *)handle)->handler;
    struct TagItem *tag;

    while ((tag = NextTagItem((struct TagItem **)&taglist)))
    {
        handler->getFileSystemAttr(PartitionBase, (struct FileSysHandle *)handle, tag);
        
        /*
         * TODO: handler returns TRUE if it knows the attribute and FALSE otherwise.
         * If we ever have more partition table types which can handle embedded
         * filesystem handlers, this can be expanded similar to GetPartitionAttrs(),
         * and we will have some generic code here.
         */
    }

    AROS_LIBFUNC_EXIT
}
