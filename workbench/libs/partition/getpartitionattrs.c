/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

*/
#include "partition_support.h"

#ifndef DEBUG
#define DEBUG 1
#endif
#include <aros/debug.h>

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
		PT_TYPE     - LONG *           ; get partition type (MBR-PC)
		PT_POSITION - LONG *           ; position of partition (MBR-PC)
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

		if (handler->GetPartitionAttrs)
			return handler->GetPartitionAttrs(PartitionBase, ph, taglist);
	}
	return 1;
	AROS_LIBFUNC_EXIT
}
