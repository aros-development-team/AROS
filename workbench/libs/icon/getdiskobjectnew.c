/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/debug.h>
#include <proto/dos.h>
#include "icon_intern.h"

/*****************************************************************************

    NAME */
#include <proto/icon.h>

	AROS_LH1(struct DiskObject *, GetDiskObjectNew,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, name, A0),

/*  LOCATION */
	struct Library *, IconBase, 22, Icon)

/*  FUNCTION
	Tries to open the supplied info file via GetDiskObject(). If this
	not succeeds it will try to read the default info file for
	that type of file.

    INPUTS
	name - name of the file to read an icon for.

    RESULT
	DiskObject - pointer ta diskobject struct.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	GetDiskObject(), GetDefDiskObject()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IconBase)
    struct DiskObject	 * dobject;
    struct FileInfoBlock * fib;

    BPTR lock,
	 parentlock;
    LONG def_type = NULL; /* Hopefully NULL != any of the possibel constants
			    (WBDISK, etc) */

    /* First try to see if name.info exists */
    if ( (dobject = GetDiskObject (name)) )
	return (dobject);
    else
    {
	/* Try to get the default icon of the file, but first we must find
	   out the type of file (or directory) */

	if ( (lock = Lock (name, ACCESS_READ)) )
	{
		/* If the parent is NULL then we are looking at a root,
		   OR if we are looking at a 'Disk' in the root, then
		   it is also a root*/
		if ((!(parentlock=ParentDir(lock))) ||
			strcasecmp(name + strlen(name)-5, ":Disk")==0)
			def_type=WBDISK;
	    else
	    {
		/* We do not need the lock of the parent anymore */
		UnLock(parentlock);

		/* Now we should exmine the file-attributes */
		if ( (fib = AllocDosObject (DOS_FIB, TAG_DONE)) )
		{
		    if (Examine(lock, fib))
		    {
			/* Do we have a directory ? */
			if (fib->fib_DirEntryType > 0)
                        {
			    def_type = WBDRAWER;
			}
                        else if (fib->fib_DirEntryType < 0) /* or a file ? */
			{
			    /* executable ? */
			    if (~fib->fib_Protection & FIBF_EXECUTE)
                            {
                                def_type = WBTOOL;
			    }
                            else
                            {
				/* Project is default */
				def_type = WBPROJECT;
                            }
                        }
		    }
                    
		    FreeDosObject(DOS_FIB,fib);
		}
	    }
            
	    UnLock(lock);
	}
	else
	{
		/* It can only be a disk if the file is in the root */
		if(strcasecmp(name+strlen(name)-5, ":Disk")!=0)
			return NULL;

	    def_type = WBDISK;
	}

	/* Try to open the default icon */
	return(GetDefDiskObject(def_type));
    }

    return (FALSE);
    AROS_LIBFUNC_EXIT
} /* GetDiskObjectNew */





