/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include "dos_intern.h"
#include "../devs/filesys/packet/packet.h"

#define DEBUG 0
#include <aros/debug.h>

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

#ifndef AROS_DOS_PACKETS
    /* If the passed entry has dol_Task defined, then it's a packet-based
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
    if (dlist->dol_Task != NULL)
    {
    	struct DosList *scan;

        for (scan = dl; scan != NULL; scan = BADDR(scan->dol_Next))
            if (scan->dol_Task == dlist->dol_Task && scan->dol_Type == DLT_DEVICE) {
                /* Do patching only if found DeviceNode belongs to packet.handler. Otherwise do not touch anything.
                   This lets filesystems with own IOFS wrappers to set up dol_Task field on their own. This can be
                   used for example to enable utilities to pass packets directly to the filesystem without the need
                   to interact with wrapper. This will be used by SFS - Pavel Fedin <sonic.amiga@gmail.com> */
                if (!strcmp(scan->dol_Ext.dol_AROS.dol_Device->dd_Library.lib_Node.ln_Name, "packet.handler")) {
                    struct ph_handle *vol_handle;

                    dlist->dol_Ext.dol_AROS.dol_DevName = AROS_BSTR_ADDR(dlist->dol_Name);
                    dlist->dol_Ext.dol_AROS.dol_Device = scan->dol_Ext.dol_AROS.dol_Device;

                    /* A separate handle needs to be allocated for each volume,
                     * because it may need to stay around after its original
                     * DOS device has been destroyed */
                    vol_handle = AllocMem(sizeof(struct ph_handle),
                        MEMF_PUBLIC);
                    if (vol_handle != NULL)
                    {
                        CopyMem(scan->dol_Ext.dol_AROS.dol_Unit, vol_handle,
                            sizeof(struct ph_handle));
                        vol_handle->msgport = dlist->dol_Task;
                        vol_handle->volume = dlist;
                        dlist->dol_Ext.dol_AROS.dol_Unit =
                            (struct Unit *)vol_handle;
                    }
                    else
                        success = 0;
                }
                break;
            }
    }
    /* Software ported from AmigaOS may be unaware of dol_DevName existance.
     * In this case dol_DevName will be NULL (this assumes that it allocates
     * the DosNode in a system-friendly manner using AllocDosObject().
     */
    if (!dlist->dol_Ext.dol_AROS.dol_DevName) {
	dlist->dol_Ext.dol_AROS.dol_DevName = AROS_BSTR_ADDR(dlist->dol_Name);
	D(bug("[AddDosEntry] Filling in dol_DevName: %s\n", dlist->dol_Ext.dol_AROS.dol_DevName));
    }
#endif

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
