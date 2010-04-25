/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include "identify_intern.h"
#include "identify.h"

/*****************************************************************************

    NAME */
#include <proto/identify.h>

        AROS_LH2(ULONG, IdHardwareNum,

/*  SYNOPSIS */
        AROS_LHA(ULONG           , type   , D0),
        AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */
        struct Library *, IdentifyBase, 9, Identify)

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

    ULONG result = 0;

    // no tags

    switch(type)
    {
        case IDHW_SYSTEM:
            break;

        case IDHW_CPU:
            break;

        case IDHW_FPU:
            break;

        case IDHW_MMU:
            break;

        case IDHW_OSVER:
            break;

        case IDHW_EXECVER:
            break;

        case IDHW_WBVER:
            break;

        case IDHW_ROMSIZE:
            break;

        case IDHW_CHIPSET:
            break;

        case IDHW_GFXSYS:
            break;

        case IDHW_CHIPRAM:
            break;

        case IDHW_FASTRAM:
            break;

        case IDHW_RAM:
            break;

        case IDHW_SETPATCHVER:
            break;

        case IDHW_AUDIOSYS:
            break;

        case IDHW_OSNR:
            break;

        case IDHW_VMMCHIPRAM:
            break;

        case IDHW_VMMFASTRAM:
            break;

        case IDHW_VMMRAM:
            break;

        case IDHW_PLNCHIPRAM:
            break;

        case IDHW_PLNFASTRAM:
            break;

        case IDHW_PLNRAM:
            break;

        case IDHW_VBR:
            break;

        case IDHW_LASTALERT:
            break;

        case IDHW_VBLANKFREQ:
            break;

        case IDHW_POWERFREQ:
            break;

        case IDHW_ECLOCK:
            break;

        case IDHW_SLOWRAM:
            break;

        case IDHW_GARY:
            break;

        case IDHW_RAMSEY:
            break;

        case IDHW_BATTCLOCK:
            break;

        case IDHW_CHUNKYPLANAR:
            break;

        case IDHW_POWERPC:
            break;

        case IDHW_PPCCLOCK:
            break;

        case IDHW_CPUREV:
            break;

        case IDHW_CPUCLOCK:
            break;

        case IDHW_FPUCLOCK:
            break;

        case IDHW_RAMACCESS:
            break;

        case IDHW_RAMWIDTH:
            break;

        case IDHW_RAMCAS:
            break;

        case IDHW_RAMBANDWIDTH:
            break;

        case IDHW_TCPIP:
            break;

        case IDHW_PPCOS:
            break;

        case IDHW_AGNUS:
            break;

        case IDHW_AGNUSMODE:
            break;

        case IDHW_DENISE:
            break;

        case IDHW_DENISEREV:
            break;
    }

    return result;

    AROS_LIBFUNC_EXIT
} /* IdHardwareNum */
