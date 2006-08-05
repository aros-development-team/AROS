/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$
*/
#include <string.h>
#include <dos/dosextens.h>

/*****************************************************************************

    NAME */

// #  define  DEBUG  1
// #include <aros/debug.h>

#include <libraries/nonvolatile.h>
#include <proto/exec.h>
#include <proto/nvdisk.h>

AROS_LH5(LONG, StoreNV,

/*  SYNOPSIS */

	AROS_LHA(STRPTR, appName,        A0),
        AROS_LHA(STRPTR, itemName,       A1),
	AROS_LHA(APTR,   data,           A2),
	AROS_LHA(ULONG,  length,         D0),
	AROS_LHA(BOOL,   killRequesters, D1),

/*  LOCATION */

	struct Library *, nvBase, 7, Nonvolatile)

/*  FUNCTION

    Save data in the nonvolatile storage.

    INPUTS

    appName         --  the application to save an item in the nonvolatile
                        storage
    itemName        --  the name of the item to save
    data            --  the data to save
    length          --  number of tens of bytes of the data to save rounded
                        upwards (for instance to save 24 bytes specify 3).
    killRequesters  --  if TRUE no system requesters will be displayed during
                        the operation of this function

    RESULT

    Indication of the success of the operation
    
        0                --  no error
	NVERR_BADNAME    --  'appName' or 'itemName' were not correctly
	                     specified names
	NVERR_WRITEPROT  --  the nonvolatile storage is read only
	NVERR_FAIL       --  failure in data saving (storage is full or write
	                     protected)
	NVERR_FATAL      --  fatal error (possible loss of previously saved
	                     data)
    
    NOTES

    The strings 'appName' and 'itemName' should be descripive but short as the
    size of the nonvolatile storage may be very limited. The strings may not
    contatin the characters ':' or '/'. The maximum length for each of these
    strings is 31.

    EXAMPLE

    BUGS

    SEE ALSO

    GetCopyNV(), GetNVInfo()

    INTERNALS

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    struct Process *me = (struct Process *)FindTask(NULL);
    APTR            oldReq = me->pr_WindowPtr;
    LONG            retval;

    if(data == NULL)
	return NVERR_FAIL;	/* There is no good (defined) error to
				   report... */

    if(appName == NULL || itemName == NULL)
	return NVERR_BADNAME;

    if(strpbrk(appName, ":/") != NULL ||
       strpbrk(itemName, ":/") != NULL)
	return NVERR_BADNAME;

    if(killRequesters)
	me->pr_WindowPtr = (APTR)-1;

    //    kprintf("Calling writedata");
    
    retval = WriteData(appName, itemName, data, length*10);

    if(killRequesters)
	me->pr_WindowPtr = oldReq;

    return retval;

    AROS_LIBFUNC_EXIT
} /* StoreNV */
