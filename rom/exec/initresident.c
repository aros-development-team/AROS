/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Build a library or device from a resident structure.
    Lang: english
*/
#include <dos/dos.h>
#include <aros/asmcall.h>
#include "exec_intern.h"
#include <exec/devices.h>

#include "exec_debug.h"
#ifndef DEBUG_InitResident
#   define DEBUG_InitResident 0
#endif
#undef DEBUG
#if DEBUG_InitResident
#   define DEBUG 1
#endif
#include <aros/debug.h>
#undef kprintf

/*****************************************************************************

    NAME */
#include <exec/resident.h>
#include <dos/bptr.h>
#include <proto/exec.h>

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

	The Init() vector is called with the following registers:
		D0 = 0
		A0 = segList
		A6 = ExecBase

    INPUTS
	resident - Pointer to resident structure.
	segList  - Pointer to loaded module, 0 for resident modules.

    RESULT
	A pointer returned from the Init() vector. Usually this is the
	base of the library/device/resource. NULL for failure.

    NOTES
	AUTOINIT modules are automatically added to the correct exec list.
	Non AUTOINIT modules have to do all the work themselves.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("InitResident $%lx (\"%s\")\n", resident, resident->rt_Name));

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

	/*
	    Make the library. Don't call the Init routine yet, but delay
	    that until we can copy stuff from the tag to the libbase.
	*/
	library = MakeLibrary(init->vectors, init->structure,
			      NULL, init->dSize, segList);

	if(library != NULL)
	{
	    /*
		Copy over the interesting stuff from the ROMtag, and set the
		library state to indicate that this lib has changed and
		should be checksummed at the next opportunity.

		Don't copy the priority, because a tag's priority doesn't
		mean the same as a lib's priority.
	    */
	    library->lib_Node.ln_Type = resident->rt_Type;
	    library->lib_Node.ln_Name = resident->rt_Name;
	    if (resident->rt_Type != NT_RESOURCE)
	    {
		library->lib_Version      = resident->rt_Version;
		library->lib_IdString     = resident->rt_IdString;
		library->lib_Flags	  = LIBF_SUMUSED|LIBF_CHANGED;
	    }

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
		    case NT_HIDD:   /* XXX Remove when new Hidd system ok. */
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
    {
	/* ...or let the library do it. */
	return AROS_UFC3(struct Library *, resident->rt_Init,
	    AROS_UFCA(ULONG,             0L,      D0),
	    AROS_UFCA(BPTR,              segList, A0),
	    AROS_UFCA(struct ExecBase *, SysBase, A6)
	);
    }

    AROS_LIBFUNC_EXIT
} /* InitResident */
