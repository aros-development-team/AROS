/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.6  1996/10/24 15:50:28  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.5  1996/10/10 13:20:49  digulla
    Use dol_DevName(STRPTR) instead of dol_Name(BSTR) (Fleischer)

    Revision 1.4  1996/09/11 12:58:46  digulla
    Determine the size of the name (M. Fleischer)

    Revision 1.3  1996/08/13 13:52:46  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced AROS_LA by AROS_LHA

    Revision 1.2  1996/08/01 17:40:51  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <dos/dosextens.h>
#include <clib/utility_protos.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	AROS_LH3(struct DosList *, FindDosEntry,

/*  SYNOPSIS */
	AROS_LHA(struct DosList *, dlist, D1),
	AROS_LHA(STRPTR,           name,  D2),
	AROS_LHA(ULONG,            flags, D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 114, Dos)

/*  FUNCTION
	Looks for the next dos list entry with the right name. The list
	must be locked for this. There may be not more than one device
	or assign node of the same name. There are no restrictions on
	volume nodes.

    INPUTS
	dlist - the value given by LockDosList() or the last call to
	        FindDosEntry().
	name  - logical device name without colon. Case insensitive.
	flags - the same flags as given to LockDosList() or a subset
	        of them.

    RESULT
	Pointer to dos list entry found or NULL if the are no more entries.

    NOTES

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
    
    static const ULONG flagarray[]=
    { LDF_DEVICES, LDF_ASSIGNS, LDF_VOLUMES, LDF_ASSIGNS, LDF_ASSIGNS };
    
    /* Determine the size of the name (-1 if the last character is a ':') */
    STRPTR end=name;
    ULONG size;
    while(*end++)
        ;
    size=~(name-end);
    if(size&&*--end==':')
        size--;

    /* Follow the list */   
    for(;;)
    {
        /* Get next entry. Return NULL if there is none. */
	dlist=dlist->dol_Next;
	if(dlist==NULL)
	    return NULL;

	/* Check type and name */
	if(flags&flagarray[dlist->dol_Type]&&
	   !Strnicmp(name,dlist->dol_DevName,size)&&!dlist->dol_DevName[size])
	    return dlist;
    }
    AROS_LIBFUNC_EXIT
} /* FindDosEntry */
