/*
    Copyright (C) 1995-1998 AROS - The Amiga Research OS
    $Id$

    Desc: Check if a device is a filesystem.
    Lang: english
*/
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include "dos_intern.h"
#include <string.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(BOOL, IsFileSystem,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, devicename, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 118, Dos)

/*  FUNCTION
	Query the device whether it is a filesystem.

    INPUTS
	devicename	- Name of the device to query.

    RESULT
	TRUE if the device is a filesystem, FALSE otherwise.

    NOTES
	DF0:, HD0:, ... are filesystems.
	CON:, PIPE:, AUX:, ... are not

        In AmigaOS if devicename contains no ":" then result
	is always TRUE (-1). Also volume and assign names return
	TRUE.
	
    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* Get pointer to process structure */
    struct Process *me = (struct Process *)FindTask(NULL);
    struct DosList *dl;
    STRPTR devicename_copy;
    STRPTR colon;
    
    BOOL success = TRUE;
    
    colon = strchr(devicename, ':');
    if (colon != NULL)
    {
	UWORD stringlen = (UWORD)(colon - devicename);
	
	devicename_copy = AllocVec(stringlen + 1, MEMF_PUBLIC | MEMF_CLEAR);
	if (devicename_copy != NULL)
	{
	    CopyMem(devicename, devicename_copy, stringlen);
	   	
	    success = FALSE;
	    	
	    dl = LockDosList(LDF_DEVICES | LDF_VOLUMES | LDF_ASSIGNS | LDF_READ);
	    dl = FindDosEntry(dl, devicename_copy, LDF_DEVICES);

	    if (dl != NULL)
	    {
		switch(dl->dol_Type)
		{
		    case DLT_DEVICE:
			{
        		    /* Get pointer to I/O request. Use stackspace for now. */
        		    struct IOFileSys io,*iofs=&io;

        		    /* Prepare I/O request. */
        		    iofs->IOFS.io_Message.mn_Node.ln_Type	= NT_REPLYMSG;
        		    iofs->IOFS.io_Message.mn_ReplyPort  	= &me->pr_MsgPort;
        		    iofs->IOFS.io_Message.mn_Length     	= sizeof(struct IOFileSys);
        		    iofs->IOFS.io_Device 			= dl->dol_Device;
        		    iofs->IOFS.io_Unit  			= dl->dol_Unit;
        		    iofs->IOFS.io_Command			= FSA_IS_FILESYSTEM;
        		    iofs->IOFS.io_Flags  			= 0;

        		    /* Send the request. */
        		    DoIO(&iofs->IOFS);

        		    /* Set return code */
        		    if(!iofs->io_DosError) {
        			success = iofs->io_Union.io_IS_FILESYSTEM.io_IsFilesystem;
			    }
			 }
			 break;
		     
		     case DLT_VOLUME:
		     case DLT_DIRECTORY:	/* normal assign */
		         success = TRUE;
			 break;
		
		     case DLT_LATE:
		     case DLT_NONBINDING:
		         {
			     APTR old_windowptr = me->pr_WindowPtr;
			     BPTR lock;
			     
			     me->pr_WindowPtr = (APTR)-1;
			     
			     if ((lock = Lock(devicename_copy, ACCESS_READ)))
			     {
			         success = TRUE;
				 UnLock(lock);
			     }
			     
			     me->pr_WindowPtr = old_windowptr;
			     
			 }
			 break;
			 
		} /* switch(dl->dol_Type) */
		 
	    } /* if (dl != NULL) */

	    /* All Done. */

	    UnLockDosList(LDF_DEVICES | LDF_VOLUMES | LDF_ASSIGNS | LDF_READ);
	    
	    FreeVec(devicename_copy);
	    
	} /* if (devicename_copy != NULL) */
	
    } /* if (colon != NULL) */
    
    return success;

    AROS_LIBFUNC_EXIT
} /* IsFilesystem */
