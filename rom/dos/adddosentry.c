/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/debug.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
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
	Behaviour of this function is slightly different from AmigaOS 3.x
	and MorphOS. Instead of LDF_WRITE it locks DosList with LDF_READ
	flag. This is done because in AROS handlers are run with DosList
	locked with LDF_READ flag and this could cause a lockup if we use
	LDF_WRITE here.
	
	Due to nature of the DosList it is safe to read the list while
	someone is adding a node, adding operation is atomic to other
	readers. The only problem here would happen if more than one
	process	attempts to add a DosNode at the same time. In order to
	avoid this race condition we make this call single-threaded
	using an LDF_ENTRY lock in LDF_WRITE mode.
	
	LDF_ENTRY is NOT touched when a handler is started up in this
	dos.library implementation. LDF_DELETE is not used at all.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG success = DOSTRUE;
    struct DosList *dl;

    if (dlist == NULL)
    	return success;

    D(bug("[AddDosEntry] Adding '%b' type %d from addr %x Task '%s'\n",
        dlist->dol_Name, dlist->dol_Type, dlist,
        FindTask(NULL)->tc_Node.ln_Name));

    dl = LockDosList(LDF_ALL | LDF_READ);

    LockDosList(LDF_ENTRY|LDF_WRITE);
    if(dlist->dol_Type != DLT_VOLUME)
    {
	while(TRUE)
	{
	    dl = BADDR(dl->dol_Next);

	    if(dl == NULL)
		break;

	    if(dl->dol_Type != DLT_VOLUME && !CMPBSTR(dl->dol_Name, dlist->dol_Name))
	    {
		D(bug("[AddDosEntry] Name clash for %08lx->dol_Name: %b and %08lx->dol_Name %b\n", dl, dl->dol_Name, dlist, dlist->dol_Name));
		success = DOSFALSE;
		break;
	    }
	}
    }

    if(success)
    {
        struct DosInfo *dinf = BADDR(DOSBase->dl_Root->rn_Info);

	dlist->dol_Next = dinf->di_DevInfo;
	dinf->di_DevInfo = MKBADDR(dlist);
    }

    UnLockDosList(LDF_ENTRY|LDF_WRITE);
    UnLockDosList(LDF_ALL | LDF_READ);

    return success;    

    AROS_LIBFUNC_EXIT
} /* AddDosEntry */
