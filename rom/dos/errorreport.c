/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "dos_intern.h"
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <intuition/intuition.h>
#include <exec/ports.h>

#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH4(BOOL, ErrorReport,

/*  SYNOPSIS */
	AROS_LHA(LONG            , code  , D1),
	AROS_LHA(LONG            , type  , D2),
	AROS_LHA(ULONG           , arg1  , D3),
	AROS_LHA(struct MsgPort *, device, D4),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 80, Dos)

/*  FUNCTION

    INPUTS

    code    --  The error to put up the requester for
    type    --  Type of request

                REPORT_LOCK    --  arg1 is a lock (BPTR).
                REPORT_FH      --  arg1 is a filehandle (BPTR).
		REPORT_VOLUME  --  arg1 is a volumenode (C pointer).
		REPORT_INSERT  --  arg1 is the string for the volumename

    arg1    --  Argument according to type (see above)
    device  --  Optional handler task address (obsolete!)

    RESULT

    NOTES

    Locks and filehandles are the same in AROS so there is redundancy in
    the parameters. Furthermore, the 'device' argument is not cared about
    as AROS don't build filesystems via handlers.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG  idcmp = 0;
    STRPTR format;
    LONG   res;

    APTR   args[4] = { NULL, NULL, NULL, NULL };

    /* I've probably missed out a few of the possibilities... for example,
       when is REPORT_STREAM used and what does it really mean? */

    if(type == REPORT_INSERT)
    {
	switch(code)
	{
	case ERROR_DEVICE_NOT_MOUNTED:
	    /* "Please insert\n<name>\n in any drive" */
	    format = DosGetString(STRING_INSERT_VOLUME);
	    args[0] = (APTR)arg1;
	    idcmp = IDCMP_DISKINSERTED;
	    break;

	default:
	    format = "%s";
	    args[0] = DosGetString(code);
	    break;
	}

    }
    else if(type == REPORT_VOLUME)
    {
	switch(code)
	{
	case ERROR_DISK_FULL:
	    /* "The volume <name> is full"; */
	    format = DosGetString(STRING_VOLUME_FULL);
	    args[0] = ((struct DeviceList *)arg1)->dl_Ext.dl_AROS.dl_DevName;
	    break;
	    
	case ERROR_NO_DISK:
	    /* "No disk in drive <name>"; */
	    format = DosGetString(STRING_NO_DISK);
	    args[0] = ((struct DeviceList *)arg1)->dl_Ext.dl_AROS.dl_DevName;
	    break;

	case ERROR_NOT_A_DOS_DISK:
	    /* "Not a DOS disk in unit <name>"; */
	    format = DosGetString(STRING_NO_DOS_DISK);
	    args[0] = ((struct DeviceList *)arg1)->dl_Ext.dl_AROS.dl_DevName;
	    kprintf("%s %s\n", format, ((struct DeviceList *)arg1)->dl_Ext.dl_AROS.dl_DevName);
	    break;
	    
	case ABORT_BUSY:
	    /* "You MUST replace volume <name> in drive <name>"; */
	    format = DosGetString(STRING_MUST_REPLACE);
	    args[0] = "Where do I get this argument from?"; /* GetFileSysTask()? */
	    args[1] = ((struct DeviceList *)arg1)->dl_Ext.dl_AROS.dl_DevName;
	    break;

	default:
	    format = "%s";
	    args[0] = DosGetString(code);
	    break;
	}
    }
    else
    {
	args[0] = DosGetString(code);
	format = "%s";
    }	 

    res = DisplayError(format, idcmp, &args);
   
    SetIoErr(code);

    return (BOOL)res;

    AROS_LIBFUNC_EXIT
} /* ErrorReport */
