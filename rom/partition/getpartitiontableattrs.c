/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

*/

#include <proto/utility.h>

#include "partition_support.h"
#include "platform.h"
#include "debug.h"

/*****************************************************************************

    NAME */
#include <utility/tagitem.h>
#include <libraries/partition.h>

   AROS_LH2(LONG, GetPartitionTableAttrs,

/*  SYNOPSIS */
   AROS_LHA(struct PartitionHandle *, root,       A1),
   AROS_LHA(struct TagItem *,   taglist,    A2),

/*  LOCATION */
   struct Library *, PartitionBase, 13, Partition)

/*  FUNCTION
    get attributes of a partition table

    INPUTS
    ph      - PartitionHandle of the partition table
    taglist - list of attributes; unknown tags are ignored
        PTT_TYPE     - ULONG *           ; get partition table type
        PTT_MAXLEADIN     - LONG *
        PTT_RESERVED - ULONG *           ; get number of reserved blocks

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

    if (root->table)
    {
    	struct PTFunctionTable *handler = root->table->handler;
    	struct TagItem *tag;

    	while ((tag = NextTagItem((struct TagItem **)&taglist)))
    	{
    	    LONG sup;

	    if (handler->getPartitionTableAttr)
                sup = handler->getPartitionTableAttr(PartitionBase, root, tag);
            else
                sup = 0;

	    if (!sup)
	    {
	        switch (tag->ti_Tag)
	        {
	        case PTT_TYPE:
            	    *((LONG *)tag->ti_Data) = root->table->type;
            	    break;

		case PTT_MAXLEADIN:
	        case PTT_RESERVED:
	        case PTT_MAX_PARTITIONS:
	            *((LONG *)tag->ti_Data) = 0;
	            break;
	        }
	    }
	}

        return 1;
    }

    /* There's no partition table here */
    return 0;

    AROS_LIBFUNC_EXIT
}
