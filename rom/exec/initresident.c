/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang:
*/
#include <dos/dos.h>
#include <aros/asmcall.h>
#include "exec_intern.h"
#include <exec/resident.h>
#include <exec/devices.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH2(APTR, InitResident,

/*  SYNOPSIS */
	AROS_LHA(struct Resident *, resident, A1),
	AROS_LHA(BPTR,              segList,  D1),

/*  LOCATION */
	struct ExecBase *, SysBase, 17, Exec)

/*  FUNCTION
	Test the resident structure and build the library or device
	with the information given therein. The Init() vector is
	called and the base address returned.

    INPUTS
	resident - Pointer to resident structure.
	segList  - Pointer to loaded module, 0 for resident modules.

    RESULT
	A pointer to the library or device ready to add to the exec lists.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Check for validity */
    if(resident->rt_MatchWord != RTC_MATCHWORD ||
       resident->rt_MatchTag != resident)
	return NULL;

    /* Depending on the autoinit flag... */
    if(resident->rt_Flags & RTF_AUTOINIT)
    {
	/* ...initialize automatically... */
	struct init
	{
	    ULONG dSize;
	    APTR vectors;
	    APTR structure;
	    ULONG_FUNC init;
	};
	struct init *init = (struct init *)resident->rt_Init;
	struct Library *library;
	library = MakeLibrary(init->vectors, init->structure,
			      NULL, init->dSize, segList);

	if(library != NULL)
	{
	    /*
		Copy over the interesting stuff from the ROMtag, and set the
		library state to indicate that this lib has changed and
		should be checksummed at the next opportunity.
	    */
	    library->lib_Node.ln_Type = resident->rt_Type;
	    library->lib_Node.ln_Name = resident->rt_Name;
	    library->lib_Version      = resident->rt_Version;
	    library->lib_IdString     = resident->rt_IdString;
	    library->lib_Flags	      = LIBF_SUMUSED|LIBF_CHANGED;

	    /*
		Call the library init vector, if set.
	    */
	    if(init->init)
	    {
		library = AROS_UFC3(struct Library *, init->init,
		    AROS_UFCA(struct Library *,  library, D0),
		    AROS_UFCA(BPTR,              segList, A0),
		    AROS_UFCA(struct ExecBase *, SysBase, A6)
		);
	    }

	    /*
		Test the library base, in case the init routine failed in
		some way.
	    */
	    if(library != NULL)
	    {
		/*
		    Add the initialized module to the system.
		*/
		switch(resident->rt_Type)
		{
		    case NT_DEVICE:
			AddDevice((struct Device *)library);
			break;
		    case NT_LIBRARY:
			AddLibrary(library);
			break;
		    case NT_RESOURCE:
			AddResource(library);
			break;
		}
	    }
	}

	return library;
    }
    else
	/* ...or let the library do it. */
	return AROS_UFC3(struct Library *, resident->rt_Init,
	    AROS_UFCA(ULONG,             0L,      D0),
	    AROS_UFCA(BPTR,              segList, A0),
	    AROS_UFCA(struct ExecBase *, SysBase, A6)
	);

    AROS_LIBFUNC_EXIT
} /* InitResident */
