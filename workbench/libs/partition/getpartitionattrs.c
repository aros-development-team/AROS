/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

*/
#include "partition_support.h"
#include "platform.h"

#ifndef DEBUG
#define DEBUG 1
#endif
#include "debug.h"

/*****************************************************************************

    NAME */
#include <utility/tagitem.h>
#include <libraries/partition.h>

   AROS_LH2(LONG, GetPartitionAttrs,

/*  SYNOPSIS */
   AROS_LHA(struct PartitionHandle *, ph,       A1),
   AROS_LHA(struct TagItem *,    taglist,       A2),

/*  LOCATION */
   struct Library *, PartitionBase, 15, Partition)

/*  FUNCTION
    get attributes of a partition

    INPUTS
    ph      - PartitionHandle
    taglist - list of attributes; unknown tags are ignored
        PT_DOSENVEC - struct DosEnvec *; get DosEnvec values
        PT_TYPE     - struct PartitionType *           ; get partition type (MBR-PC)
        PT_POSITION - ULONG *           ; position of partition (MBR-PC)
        PT_ACTIVE   - LONG *           ; is partition active
        PT_NAME     - STRPTR    ; get name of partition (max 31 Bytes + NULL-byte)

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
    21-02-02    first version

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (ph->root)
    {
    struct PTFunctionTable *handler = ph->root->table->handler;

        if (handler->getPartitionAttrs)
            return handler->getPartitionAttrs(PartitionBase, ph, taglist);
    }
    else
    {
        /* we are the root partition */
        while (taglist[0].ti_Tag != TAG_DONE)
        {
            switch (taglist[0].ti_Tag)
            {
            case PT_GEOMETRY:
                {
                struct DriveGeometry *dg = (struct DriveGeometry *)taglist[0].ti_Data;
                    CopyMem(&ph->dg, dg, sizeof(struct DriveGeometry));
                }
                break;
            case PT_DOSENVEC:
                CopyMem(&ph->de, (struct DosEnvec *)taglist[0].ti_Data, sizeof(struct DosEnvec));
                break;
            }
            taglist++;
        }
    }
    return 0;
    AROS_LIBFUNC_EXIT
}
