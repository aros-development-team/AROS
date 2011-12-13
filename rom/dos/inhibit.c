/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/
#include <proto/exec.h>

#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(LONG, Inhibit,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, name,  D1),
	AROS_LHA(LONG,         onoff, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 121, Dos)

/*  FUNCTION

    Stop a filesystem from being used.

    INPUTS

    name   --  Name of the device to inhibit (including a ':')
    onoff  --  Specify whether to inhibit (DOSTRUE) or uninhibit (DOSFALSE)
               the device

    RESULT

    A boolean telling whether the action was carried out.

    NOTES

    After uninhibiting a device anything might have happened like the disk
    in the drive was removed.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PacketHelperStruct phs;
    LONG status = DOSFALSE;

    if (!getdevpacketinfo(DOSBase, name, NULL, &phs))
    	return DOSFALSE;
 
    status = dopacket1(DOSBase, NULL, phs.port, ACTION_INHIBIT, onoff);

    freepacketinfo(DOSBase, &phs);
    
    return status;

    AROS_LIBFUNC_EXIT
} /* Inhibit */
