/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add a DOS device to the system.
    Lang: English
*/
#include "expansion_intern.h"
#include <exec/io.h>
#include <proto/exec.h>
#include <proto/dos.h>

/*****************************************************************************

    NAME */
#include <dos/filehandler.h>
#include <dos/dosextens.h>
#include <libraries/expansion.h>
#include <proto/expansion.h>

	AROS_LH3(BOOL, AddDosNode,

/*  SYNOPSIS */
	AROS_LHA(LONG               , bootPri, D0),
	AROS_LHA(ULONG              , flags, D1),
	AROS_LHA(struct DeviceNode *, deviceNode, A0),

/*  LOCATION */
	struct ExpansionBase *, ExpansionBase, 25, Expansion)

/*  FUNCTION
	This is the old function for adding devices to the system. It
	is recommended that you use the AddBootNode() function.

	Unlike AddBootNode() you will have to add a BootNode to the
	system yourself.

    INPUTS
	bootPri     -   The priority of the device (-128 --> 127).
	flags       -   Flags (ADNF_STARTPROC etc)
	deviceNode  -   The device to add to the system.

    RESULT
	non-zero if everything succeeded, zero on failure.

    NOTES
	It is much better to use AddBootNode() as it will also
	construct the BootNode structure, and add it to the system.

    EXAMPLE
	//  Add a bootable disk to the system. This will start a
	//  file handler process immediately.

	if( AddDosNode( 0, ADNF_STARTPROC, MakeDosNode( paramPacket )))
	{
	    // AddDosNode() ok
	}

    BUGS

    SEE ALSO
	AddBootNode(), MakeDosNode()

    INTERNALS

    HISTORY
	19-05-07    sonic   Rewritten to use dos.library for starting up
			    a handler.
	27-11-96    digulla automatically created from
			    expansion_lib.fd and clib/expansion_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /*
     * Before V36 Kickstart, no public function existed to add BOOTNODES.
     * If an older expansion.library is in use, driver code would needed 
     * to manually construct a BootNode and Enqueue() it to the mount list.
     *
     * This maps to the pre v36 hidden function, that was identical
     * to AddBootNode(bootPri, flags, deviceNode, NULL);
     *
     * The reason for the difference is that the old function was
     * ROM internal, and it had no provision for specifying which
     * ConfigDev the DeviceNode was attached to.
     */
    return AddBootNode(bootPri, flags, deviceNode, NULL);

    AROS_LIBFUNC_EXIT
} /* AddDosNode */
