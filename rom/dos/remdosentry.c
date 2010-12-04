/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <dos/dosextens.h>
#include <proto/exec.h>
#include "../devs/filesys/packet/packet.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(LONG, RemDosEntry,

/*  SYNOPSIS */
	AROS_LHA(struct DosList *, dlist, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 112, Dos)

/*  FUNCTION
	Removes a given dos list entry from the dos list. Automatically
	locks the list for writing.

    INPUTS
	dlist - pointer to dos list entry.

    RESULT
	!=0 if all went well, 0 otherwise.

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
    struct DosList *dl;

    if(dlist == NULL)
	return 0;

    dl = LockDosList(LDF_ALL | LDF_WRITE);

    while(TRUE)
    {
        struct DosList *dl2 = BADDR(dl->dol_Next);

        if(dl2 == dlist)
	{
	    dl->dol_Next = dlist->dol_Next;
	    break;
	}

	dl = dl2;
    }

    UnLockDosList(LDF_ALL | LDF_WRITE);

#ifndef AROS_DOS_PACKETS

    if (dlist->dol_Type == DLT_VOLUME && !dlist->dol_Ext.dol_AROS.dol_Device) {
      bug("WARNING: dol_Type == DLT_VOLUME, but not dol_Device was set; this handle probably won't work\n");
    }
    
    /* Free a volume's packet.handler handle where applicable */
    if(dlist->dol_Type == DLT_VOLUME && 
       dlist->dol_Ext.dol_AROS.dol_Device &&
       !strcmp(dlist->dol_Ext.dol_AROS.dol_Device->dd_Library.lib_Node.ln_Name,
        "packet.handler"))
        FreeMem(dlist->dol_Ext.dol_AROS.dol_Unit, sizeof(struct ph_handle));
#endif

    return 1;

    AROS_LIBFUNC_EXIT
} /* RemDosEntry */
