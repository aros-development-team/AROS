/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/config.h>
#include <exec/types.h>
#include <utility/tagitem.h>
#include <aros/libcall.h>
#include <proto/utility.h>
#include "aros_intern.h"

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

/* Kickstart ROM location offsets */
#define LOC_COOKIE	0x00
#define LOC_ADDRESS	0x04
#define LOC_MAJORV	0x0c
#define LOC_MINORV	0x0e
#define LOC_ROMSIZE	0x14		/* offset from end of ROM! */
#define ROM_END 	0x1000000

#define AROS_VERSION_MAJOR 1
#define AROS_VERSION_MINOR 12
#define AROS_RELEASE_DATE  7560         /* in days since 1978-01-01 */

#if (AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
/* Native AROS support functions */
IPTR kicksize(void);
IPTR kickbase(void);
#endif

/*****************************************************************************

    NAME */
#include <aros/inquire.h>

	AROS_LH1(ULONG, ArosInquireA,

/*  SYNOPSIS */

	AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */

	struct ArosBase *, ArosBase, 5, Aros)

/*  FUNCTION
	This function is used to query system characteristics not easily
	queried with another function.

    INPUTS
	tags -- taglist with appropriate queries. The tag's ti_Data field
		should point to the location where the result of the query
		is stored. Do not forget to clear the location before, as
		queries not understood will be left untouched.

	AI_KickstartBase APTR
	AI_KickstartSize ULONG
	AI_KickstartVersion UWORD
	AI_KickstartRevision UWORD
		Only support these tags if we are on the native machine. On other machines this
		call will not touch the storage space. Set the storage space to 0 if you want to
		see if this call touches it.

	AI_ArosVersion IPTR
		aros.library version masquerades as AROS version. This means
		that all aros modules must have the same major version number.

	AI_ArosReleaseMajor IPTR
		Update this whenever a new AROS is released.

	AI_ArosReleaseMinor IPTR
		Update this whenever a new AROS is released.

	AI_ArosReleaseDate IPTR
		Update this whenever a new AROS is released.

	AI_ArosBuildDate IPTR
		Given in the format: <d>.<m>.<y>
	AI_ArosVariant IPTR
		Configure time variant name.

    RESULT
	All queries understood by this call will have appropriate values
	assigned to the location a tag's ti_Data pointed to.

	This function will (for now) always return 0.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	aros/arosbase.h

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tag;
    ULONG ret = 0;

#   define SetData(tag,type,value)  \
	D(bug("   Data was: %d\n", *((type *)(tag->ti_Data)))); \
	(*((type *)(tag->ti_Data)) = value); \
	D(bug("   Data is : %d\n", *((type *)(tag->ti_Data))))

    D(bug("ArosInquireA(taglist=%p)\n", taglist));

    while( (tag = NextTagItem((const struct TagItem**)&taglist)))
    {
	D(bug("  tag = 0x%lx  data = 0x%lx\n", tag->ti_Tag, tag->ti_Data));

	switch(tag->ti_Tag)
	{

#if (AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
	/*
	    Only support these tags if we are on the native machine. On other
	    machines this call will not touch the storage space. Set the
	    storage space to 0 if you want to see if this call touches it.
	*/

	case AI_KickstartBase:
	    SetData (tag, APTR, kickbase());
	    break;

	case AI_KickstartSize:
	    SetData (tag, ULONG, kicksize());
	    break;

	case AI_KickstartVersion:
	    SetData (tag, UWORD, *(UWORD *)(kickbase() + LOC_MAJORV));
	    break;

	case AI_KickstartRevision:
	    SetData (tag, UWORD, *(UWORD *)(kickbase() + LOC_MINORV));
	    break;
#else
	case AI_KickstartSize:
	    SetData (tag, ULONG, 0);
	    break;

#endif

	case AI_ArosVersion:
	    /*
		aros.library version masquerades as AROS version. This means
		that all aros modules must have the same major version number.
	    */
	    SetData (tag, IPTR, VERSION_NUMBER);
	    break;

	case AI_ArosReleaseMajor:
	    /* Update this whenever a new AROS is released */
	    SetData (tag, IPTR, AROS_VERSION_MAJOR);
	    break;

	case AI_ArosReleaseMinor:
	    /* Update this whenever a new AROS is released */
	    SetData (tag, IPTR, AROS_VERSION_MINOR);
	    break;

        case AI_ArosReleaseDate:
            /* Update this whenever a new AROS is released */
            SetData (tag, IPTR, AROS_RELEASE_DATE);
            break;

        case AI_ArosBuildDate:
            SetData (tag, IPTR, (IPTR)__DATE__);

            break;

        case AI_ArosVariant:
            SetData (tag, IPTR, (IPTR) VARIANT);
            break;

        default:
            SetData (tag, IPTR, 0);
            break;

	}
    }

    return ret;
    AROS_LIBFUNC_EXIT
} /* ArosInquireA */

#if (AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
/* Native AROS support functions */
IPTR kicksize(void)
{
    return *(ULONG *)(ROM_END - LOC_ROMSIZE);
}

IPTR kickbase(void)
{
    return (ROM_END - kicksize());
}
#endif
