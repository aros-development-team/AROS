/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <workbench/icon.h>
#include <proto/icon.h>

#include "icon_intern.h"

#   include <aros/debug.h>

/*****************************************************************************

    NAME */

    AROS_LH1(struct DiskObject *, NewDiskObject,

/*  SYNOPSIS */
        AROS_LHA(ULONG, type, D0),
        
/*  LOCATION */
        struct Library *, IconBase, 29, Icon)

/*  FUNCTION
	Create empty diskobject structure.
	
    INPUTS
	type - WBDISK, WBDRAWER, WBTOOL, WBPROJECT,
	       WBGARBAGE, WBDEVICE or WBKICK

    RESULT

    NOTES
	Not implemented.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
#   warning TODO: Implement icon/NewDiskObject()
    aros_print_not_implemented("icon/NewDiskObject()");

    return NULL;
    
    AROS_LIBFUNC_EXIT
} /* NewDiskObject() */
