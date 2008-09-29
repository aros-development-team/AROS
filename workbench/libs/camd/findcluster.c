/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

#include <proto/exec.h>

#include "camd_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH1(struct MidiCluster *, FindCluster,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, name, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 22, Camd)

/*  FUNCTION
		Finds a midicluster from camd's internal list of midiclusters.

    INPUTS
		name - Name of cluster to find.

    RESULT
		NULL if cluster could not be found.

    NOTES
		- CL_Linkages must be locked before calling.

    EXAMPLE

    BUGS

    SEE ALSO
		FindMidi()

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	return (struct MidiCluster *)FindName(&CB(CamdBase)->midiclusters,name);

   AROS_LIBFUNC_EXIT
}
