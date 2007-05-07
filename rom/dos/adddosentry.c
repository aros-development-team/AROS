/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <dos/dosextens.h>
#include <proto/utility.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(LONG, AddDosEntry,

/*  SYNOPSIS */
	AROS_LHA(struct DosList *, dlist, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 113, Dos)

/*  FUNCTION
	Adds a given dos list entry to the dos list. Automatically
	locks the list for writing. There may be not more than one device
	or assign node of the same name. There are no restrictions on
	volume nodes.

    INPUTS
	dlist - pointer to dos list entry.

    RESULT
	!= 0 if all went well, 0 otherwise.

    NOTES
	Since anybody who wants to use a device or volume node in the
	dos list has to lock the list, filesystems may be called with
	the dos list locked. So if you want to add a dos list entry
	out of a filesystem don't just wait on the lock but serve all
	incoming requests until the dos list is free instead.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG            success = 1;
    struct DosList *dl, *scan;

    if (dlist == NULL) return success;

    dl = LockDosList(LDF_ALL | LDF_WRITE);

    /* If the passed entry has dol_Task defined, then its a packet-based
     * handler, and probably doesn't have valid dol_DevName, dol_Device and
     * dol_Unit fields, which will be needed. So we search through the DOS
     * list looking for the packet.handler entry for the same process, and
     * fill in the values from there.
     *
     * This all falls down if the handler does somehow know these fields and
     * adds different values. We can't just test for NULL, as the handler may
     * not have cleared it. I can't think of a single good reason why a
     * handler would do this, so I'm not worrying about it for now.
     *
     * It will also break if the handler has set dol_Task to something other
     * than its original packet.handler task. In that case we won't be able to
     * match it correctly in the DOS list, and so the three fields will remain
     * bogus, probably causing crashes shortly after. Again, I'll worry about
     * it if and when it happens.
     */
    if (dlist->dol_Task != NULL) {
        for (scan = dl; scan != NULL; scan = scan->dol_Next)
            if (scan->dol_Task == dlist->dol_Task && scan->dol_Type == DLT_DEVICE) {
                dlist->dol_Ext.dol_AROS.dol_DevName = AROS_BSTR_ADDR(dlist->dol_Name);
                dlist->dol_Ext.dol_AROS.dol_Device = scan->dol_Ext.dol_AROS.dol_Device;
                dlist->dol_Ext.dol_AROS.dol_Unit = scan->dol_Ext.dol_AROS.dol_Unit;
                break;
            }
    }

    if(dlist->dol_Type != DLT_VOLUME)
    {
	while(TRUE)
	{
	    dl = dl->dol_Next;

	    if(dl == NULL)
		break;

	    if(dl->dol_Type != DLT_VOLUME &&
	       !Stricmp(dl->dol_Ext.dol_AROS.dol_DevName, dlist->dol_Ext.dol_AROS.dol_DevName))
	    {
		success = 0;
		break;
	    }
	}
    }

    if(success)
    {
	dlist->dol_Next = DOSBase->dl_DevInfo;
	DOSBase->dl_DevInfo = dlist;
    }

    UnLockDosList(LDF_ALL | LDF_WRITE);

    return success;    

    AROS_LIBFUNC_EXIT
} /* AddDosEntry */
