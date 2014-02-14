/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: FindConfigDev() - Find a specific configurable device.
    Lang: english
*/
#include "expansion_intern.h"

/*****************************************************************************

    NAME */
#include <proto/expansion.h>

	AROS_LH3(struct ConfigDev *, FindConfigDev,

/*  SYNOPSIS */
	AROS_LHA(struct ConfigDev *, oldConfigDev, A0),
	AROS_LHA(LONG              , manufacturer, D0),
	AROS_LHA(LONG              , product, D1),

/*  LOCATION */
	struct ExpansionBase *, ExpansionBase, 12, Expansion)

/*  FUNCTION
	FindConfigDev() will search through the list of ConfigDevs and find
	the one with the matching manufacturer and product identifiers.

	The search will start with the ConfigDev after the oldConfigDev,
	or at the beginning of oldConfigDev is NULL.

	A manufacturer or product of -1 is treated as a wildcard and will
	match any value.

    INPUTS
	oldConfigDev    -   The device to start the search after. If NULL
			    the search will start from the beginning of the
			    list.
	manufacturer    -   The manufacturer id of the requested ConfigDev.
			    A value of -1 will match any device.
	product         -   The product id of the ConfigDev. A value of -1
			    will match any device.

    RESULT
	The address of the first matching ConfigDev structure, or NULL if
	none could be found.

    NOTES

    EXAMPLE
	// Find all the config devs in the system
	struct ConfigDev *cd = NULL;

	while((cd = FindConfigDev(NULL, -1, -1)))
	{
	    Printf("Found a device:\tMan = %5d\tProd = %d\n",
		cd->cd_Rom.er_Manufacturer,
		cd->cd_Rom.er_Product);
	}

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ConfigDev *cd = NULL;

    /* Search through the list, we might as well lock it */
    ObtainConfigBinding();

    if( oldConfigDev == NULL )
	cd = (struct ConfigDev *)
	   ((struct IntExpansionBase *)ExpansionBase)->BoardList.lh_Head;
    else
	cd = (struct ConfigDev *)(oldConfigDev->cd_Node.ln_Succ);

    if (cd)
    {
        while( cd->cd_Node.ln_Succ != NULL )
        {
            if
            (
                ((manufacturer == -1) || (cd->cd_Rom.er_Manufacturer == manufacturer))
             && ((product == -1) || (cd->cd_Rom.er_Product == product))
            )
                break;

            cd = (struct ConfigDev *)cd->cd_Node.ln_Succ;
        }
        if (cd->cd_Node.ln_Succ == NULL)
            cd = NULL;
    }

    ReleaseConfigBinding();
    return cd;

    AROS_LIBFUNC_EXIT
} /* FindConfigDev */
