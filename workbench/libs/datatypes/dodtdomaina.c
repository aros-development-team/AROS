/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <proto/intuition.h>
#include <intuition/gadgetclass.h>
#include <intuition/intuition.h>
#include "datatypes_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH7(ULONG, DoDTDomainA,

/*  SYNOPSIS */
	AROS_LHA(Object           *, o     , A0),
	AROS_LHA(struct Window    *, win   , A1),
	AROS_LHA(struct Requester *, req   , A2),
	AROS_LHA(struct RastPort  *, rport , A3),
	AROS_LHA(ULONG             , which , D0),
	AROS_LHA(struct IBox      *, domain, A4),
	AROS_LHA(struct TagItem   *, attrs , A5),


/*  LOCATION */
        struct Library *, DataTypesBase, 51, DataTypes)

/*  FUNCTION

    Obtain the maximum/minimum/nominal domains of a data type object.

    INPUTS

    o       --  data type object in question
    win     --  window that the object is attached to
    req     --  requester the object is attached to
    rport   --  rastport; used for domain calculations
    which   --  the domain to obtain (GDOMAIN_, see <intuition/gadgetclass.h>
    domain  --  the result will be put here
    attrs   --  additional attributes (subclass specific)

    RESULT

    The return value of GM_DOMAIN or 0 if an error occurred. The 'domain' 
    IBox will be filled with the requested values as a side effect.

    NOTES

    This function requires an object to perform the GM_DOMAIN method. To
    achieve similar results without an object, you must use CoerceMethodA()
    manually.

    EXAMPLE

    BUGS

    SEE ALSO

    <intuition/gadgetclass.h>

    INTERNALS

    HISTORY

    7.8.99  SDuvan  implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct gpDomain gpd;
    ULONG           ret;

    if(o == NULL)
	return 0;

    gpd.MethodID   = GM_DOMAIN;
    gpd.gpd_GInfo  = ((struct Gadget *)o)->SpecialInfo;
    gpd.gpd_RPort  = rport;
    gpd.gpd_Which  = which;
    gpd.gpd_Attrs  = attrs;

    ret = DoGadgetMethodA((struct Gadget *)o, win, req, (Msg)&gpd);

    *domain = gpd.gpd_Domain;

    return ret;

    AROS_LIBFUNC_EXIT
} /* DoDTDomainA */

