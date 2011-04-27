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

   AROS_LH2(LONG, GetPartitionAttrs,

/*  SYNOPSIS */
   AROS_LHA(struct PartitionHandle *, ph,       A1),
   AROS_LHA(const struct TagItem *, taglist,    A2),

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

    LONG (*getPartitionAttr)(struct Library *, struct PartitionHandle *, struct TagItem *) = NULL;

    struct TagItem *tag;

    if (ph->root)
    {
	struct PTFunctionTable *handler = ph->root->table->handler;

	getPartitionAttr = handler->getPartitionAttr;
    }

    while ((tag = NextTagItem(&taglist)))
    {
    	LONG sup;

	/* If we have partition handler, call its function first */
        if (getPartitionAttr)
            sup = getPartitionAttr(PartitionBase, ph, tag);
        else
            sup = FALSE;

	if (!sup)
	{
	    /*
	     * No handler (root partition) or the handler didn't process the attribute.
	     * Return defaults.
	     */
            switch (tag->ti_Tag)
            {
            case PT_GEOMETRY:
                CopyMem(&ph->dg, (APTR)tag->ti_Data, sizeof(struct DriveGeometry));
                break;

            case PT_DOSENVEC:
                CopyMem(&ph->de, (APTR)tag->ti_Data, sizeof(struct DosEnvec));
                break;
                
	    case PT_TYPE:
	    	/* We have no type semantics */
		PTYPE(tag->ti_Data)->id_len = 0;
		break;

	    case PT_LEADIN:
	    case PT_POSITION:
	    	*((ULONG *)tag->ti_Data) = 0;
	    	break;

	    case PT_ACTIVE:
	    case PT_BOOTABLE:
	    case PT_AUTOMOUNT:
		*((BOOL *)tag->ti_Data) = FALSE;
		break;

	    case PT_NAME:
	        if (ph->ln.ln_Name)
	            strncpy((STRPTR)tag->ti_Data, ph->ln.ln_Name, 32);
	        else
	    	    ((STRPTR)tag->ti_Data)[0] = 0;
	    	break;
            }
        }
    }

    return 0;

    AROS_LIBFUNC_EXIT
}
