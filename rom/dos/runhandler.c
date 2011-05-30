#include <aros/debug.h>
#include <proto/exec.h>

#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(struct MsgPort *, RunHandler,

/*  SYNOPSIS */
	AROS_LHA(struct DeviceNode *, deviceNode, A0),
	AROS_LHA(const char *, path, A1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 27, Dos)

/*  FUNCTION
	Attempt to run a filesystem handler for the given device.

    INPUTS
	deviceNode	- A pointer to a DOS device node.
	path		- A path being accessed.

    RESULT
	A pointer to handler's message port or NULL on failure.

    NOTES
    	This function is private. Do not use it in any software.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* First check if already started */
    if (!(deviceNode->dn_Ext.dn_AROS.dn_Device))
    {
        struct IOFileSys iofs;
	STRPTR handler;
	struct FileSysStartupMsg *fssm;
	ULONG fssmFlags = 0;

        InitIOFS(&iofs, 0, DOSBase);

        if (deviceNode->dn_Handler == BNULL)
	    handler = "afs.handler";
	else
	    handler = AROS_BSTR_ADDR(deviceNode->dn_Handler);

	/*
	 * FIXME: this assumes that dol_Startup points to struct FileSysStartupMsg.
	 *        This is not true for plain handlers, dol_Startup is a BSTR in this case.
	 *        In order to make use of this we should implement direct support for
	 *        packet-style handlers in dos.library
	 */
	fssm = (struct FileSysStartupMsg *)BADDR(deviceNode->dn_Startup);
	if (fssm != NULL)
	{
#if __WORDSIZE > 32
	    if (fssm->fssm_Environ)
	    {
		/*
		 * EXPERIMENTAL: Fix up BufMemType on 64 bits.
		 * Many software sets Mask to 0x7FFFFFFF, assuming 31-bit memory, with BufMemType = PUBLIC.
		 * This is perfectly true on 32-bit architectures, where addresses from 0x80000000 and up
		 * belong to MMIO, however on 64 bits we might have memory beyond this address.
		 * And AllocMem(MEMF_PUBLIC) would prefer to return that memory. This might screw up
		 * filesystems expecting AllocMem() to return memory fully corresponding to the mask.
		 */
	    	struct DosEnvec *de = BADDR(fssm->fssm_Environ);

		if ((de->de_TableSize >= DE_MASK) && (!(de->de_Mask & 0x7FFFFFFF)))
	    	    de->de_BufMemType |= MEMF_31BIT;
	    }
#endif

	    iofs.io_Union.io_OpenDevice.io_DeviceName = AROS_BSTR_ADDR(fssm->fssm_Device);
	    iofs.io_Union.io_OpenDevice.io_Unit       = fssm->fssm_Unit;
	    iofs.io_Union.io_OpenDevice.io_Environ    = (IPTR *)BADDR(fssm->fssm_Environ);
	    fssmFlags = fssm->fssm_Flags;
	}
	iofs.io_Union.io_OpenDevice.io_DosName    = deviceNode->dn_Ext.dn_AROS.dn_DevName;
	iofs.io_Union.io_OpenDevice.io_DeviceNode = deviceNode;

	D(bug("Starting up %s\n", handler));
	if (!OpenDevice(handler, 0, &iofs.IOFS, fssmFlags) ||
            !OpenDevice("packet.handler", 0, &iofs.IOFS, fssmFlags))
	{
	    /* Ok, this means that the handler was able to open. */
	    D(bug("Handler started\n"));

	    deviceNode->dn_Ext.dn_AROS.dn_Device = iofs.IOFS.io_Device;
	    deviceNode->dn_Ext.dn_AROS.dn_Unit   = iofs.IOFS.io_Unit;
	}
    }

    return (struct MsgPort *)deviceNode->dn_Ext.dn_AROS.dn_Device;

    AROS_LIBFUNC_EXIT
}
