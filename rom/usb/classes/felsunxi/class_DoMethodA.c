/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 1

#include "felsunxi_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH2(IPTR, DoMethodA,

/*  SYNOPSIS */
    	AROS_LHA(ULONG,  methodid,   D0),
    	AROS_LHA(IPTR *, methoddata, A1),
	
/*  LOCATION */
	LIBBASETYPEPTR, LIBBASE, 7, FELSunxi)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    switch(methodid) {

        case UCM_AttemptDeviceBinding:
            mybug(-1,("FELSunxi DoMethodA AttemptDeviceBinding\n"));
            return((IPTR) AttemptDeviceBinding(LIBBASE, (struct PsdDevice *) methoddata[0]));
            break;
        case UCM_ForceDeviceBinding:
            mybug(-1,("FELSunxi DoMethodA ForceDeviceBinding\n"));
            return((IPTR) ForceDeviceBinding(LIBBASE, (struct PsdDevice *) methoddata[0]));
            break;
        case UCM_ReleaseDeviceBinding:
            mybug(-1,("FELSunxi DoMethodA ReleaseDeviceBinding\n"));
            ReleaseDeviceBinding(LIBBASE, (struct FELSunxiDevice *) methoddata[0]);
            return(TRUE);
            break;

        case UCM_AttemptInterfaceBinding:
            mybug(-1,("FELSunxi DoMethodA AttemptInterfaceBinding\n"));
            break;
        case UCM_ForceInterfaceBinding:
            mybug(-1,("FELSunxi DoMethodA ForceInterfaceBinding\n"));
            break;
        case UCM_ReleaseInterfaceBinding:
            mybug(-1,("FELSunxi DoMethodA ReleaseInterfaceBinding\n"));
            break;

        case UCM_OpenCfgWindow:
            mybug(-1,("FELSunxi DoMethodA OpenCfgWindow\n"));
            break;
        case UCM_OpenBindingCfgWindow:
            mybug(-1,("FELSunxi DoMethodA OpenBindingCfgWindow\n"));
            break;
        case UCM_ConfigChangedEvent:
            mybug(-1,("FELSunxi DoMethodA ConfigChangedEvent\n"));
            break;

        default:
            mybug(-1,("FELSunxi DoMethodA default\n"));
            break;
    }

    return(0);
    AROS_LIBFUNC_EXIT

} /* DoMethodA */
