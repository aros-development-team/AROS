/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/config.h>
#include <exec/types.h>
#include <utility/tagitem.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include "aros_intern.h"

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

/* Kickstart ROM location offsets */
#define LOC_COOKIE  0x00
#define LOC_ADDRESS 0x04
#define LOC_MAJORV  0x0c
#define LOC_MINORV  0x0e
#define LOC_ROMSIZE 0x14    /* offset from end of ROM! */
#define ROM_END     0x1000000

#define AROS_VERSION_MAJOR      1
#define AROS_VERSION_MINOR      12
#define AROS_ABI_VERSION_MAJOR  -1      /* Change only value, name is used in external script */
#define AROS_RELEASE_DATE       7560    /* in days since 1978-01-01 */

#if (AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
/* Amiga hardware support functions */
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
        queried with another function. All queries understood by this call
        will have appropriate values assigned to the location the query tag's
        ti_Data field points to.

    INPUTS
        tags - a tag list with appropriate queries. Each tag's ti_Data field
            should point to the location where the result of the query
            is to be stored. Do not forget to clear the location before, as
            queries not understood will be left untouched.

    TAGS
        AI_KickstartBase APTR
        AI_KickstartSize ULONG
        AI_KickstartVersion UWORD
        AI_KickstartRevision UWORD
            Only support these tags if we are on the native machine. On other
            machines this call will not touch the storage space. Set the
            storage space to 0 beforehand if you want to see if this call
            touches it.

        AI_ArosVersion IPTR
            aros.library version masquerades as AROS version. This means
            that all AROS modules must have the same major version number.

        AI_ArosReleaseMajor IPTR
            Update this whenever a new AROS is released.

        AI_ArosReleaseMinor IPTR
            Update this whenever a new AROS is released.

        AI_ArosReleaseDate IPTR
            Update this whenever a new AROS is released.

        AI_ArosBuildDate IPTR
            Given in the format: <d>.<m>.<y>

        AI_ArosVariant IPTR
            Distribution name.

        AI_ArosArchitecture IPTR
            Return the target architecture.

        AI_ArosABIMajor IPTR
            Update this whenever a new ABI is introduced in AROS. Special
            value of -1 means that the ABI is under development and subject
            to change.

    RESULT
        index - the index of the first tag that could not be processed, plus
            one (e.g. 1 for taglist[0], 2 for taglist[1] etc.). Zero if all
            tags were handled.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        aros/arosbase.h

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    APTR UtilityBase;
    struct TagItem *tag;
    ULONG ret = 0;
    int i = 0;
    IPTR data = 0;

    if (!(UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY)))
        return 1;

#   define SetData(tag,type,value)  \
    D(bug("   Data was: %d\n", *((type *)(tag->ti_Data)))); \
    (*((type *)(tag->ti_Data)) = value); \
    D(bug("   Data is : %d\n", *((type *)(tag->ti_Data))))

    D(bug("ArosInquireA(taglist=%p)\n", taglist));

    while( (tag = NextTagItem(&taglist)))
    {
        D(bug("  tag[%d] = 0x%lx  data = 0x%lx\n", i, tag->ti_Tag, tag->ti_Data));
        i++;

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

#ifdef VARIANT
	/*
	 * Reserved for distribution maintainers.
	 * DO NOT set this to configure-time variant name. That name is used to identify
	 * sub-architecture name, and changing it may break compatibility between
	 * different AROS modules, especially on hosted. Full complete platform name
	 * (including variant, if appropriate) is available via KATTR_Architecture
	 * kernel's attribute.
	 * Add one more configure option, or, better, provide some another way
	 * (env variable, whatever).
	 */
        case AI_ArosVariant:
            SetData (tag, IPTR, (IPTR) VARIANT);
            break;
#endif

        case AI_ArosABIMajor:
            SetData (tag, IPTR, AROS_ABI_VERSION_MAJOR);
            break;
        
        case AI_ArosArchitecture:
#ifdef KrnGetSystemAttr
            if (ArosBase->aros_KernelBase)
            {
                APTR KernelBase = ArosBase->aros_KernelBase;
                data = KrnGetSystemAttr(KATTR_Architecture);
            }
#else
	    /*
	     * This is a legacy hack for old PPC-native kernel.resource implementations.
	     * Please do not support this. aros.library is a part of base kickstart, and
	     * it is platform-independent. Consequently, it should not include any hardcoded
	     * references to platform name. Platform name is specified by kernel.resource.
	     * See boot/modular_kickstart.txt for more info.
	     */
            data = (IPTR)AROS_ARCHITECTURE;
#endif
            SetData(tag, IPTR, data);
            break;
        
        default:
            if (ret == 0)
                ret = i;
            break;
        
        }
    }

    CloseLibrary(UtilityBase);

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
