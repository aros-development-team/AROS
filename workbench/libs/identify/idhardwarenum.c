/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <exec/execbase.h>
#include <graphics/gfxbase.h>
#include <aros/inquire.h>
#include <resources/battclock.h>

#include <proto/exec.h>
#include <proto/aros.h>
#include <proto/graphics.h>
#include <proto/battclock.h>

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
        struct IdentifyBaseIntern *, IdentifyBase, 9, Identify)

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

    // no tags

    switch(type)
    {
        case IDHW_SYSTEM:
            return IDSYS_AROS;

        case IDHW_CPU:
            return IDCPU_68000; // FIXME: original identify.library assumes
                                // there's always a X68.

        case IDHW_FPU:
            return IDFPU_NONE;

        case IDHW_MMU:
            return IDMMU_NONE;

        case IDHW_OSVER:
        {
            IPTR version = 0;
            IPTR revision = 0;
            ArosInquire
            (
                AI_ArosReleaseMajor, &version,
                AI_ArosReleaseMinor, &revision,
                TAG_DONE
            );
            return version || (revision << 16);
        }

        case IDHW_EXECVER:
            return
                ((struct Library *)SysBase)->lib_Version
                ||
                (((struct Library *)SysBase)->lib_Revision << 16);

        case IDHW_WBVER:
            return 0;

        case IDHW_ROMSIZE:
        {
            ULONG storage = 0;
            ArosInquire(AI_KickstartSize, &storage, TAG_DONE);
            return storage;
        }

        case IDHW_CHIPSET:
            return IDCS_OCS; // FIXME: there's no IDCS_NONE

        case IDHW_GFXSYS:
            return IDGOS_CGX;

        case IDHW_CHIPRAM:
            return AvailMem(MEMF_CHIP|MEMF_TOTAL);

        case IDHW_FASTRAM:
            return AvailMem(MEMF_FAST|MEMF_TOTAL);

        case IDHW_RAM:
            return AvailMem(MEMF_ANY|MEMF_TOTAL);

        case IDHW_SETPATCHVER:
            return 0;

        case IDHW_AUDIOSYS:
            return IDAOS_AHI;

        case IDHW_OSNR:
            return IDOS_UNKNOWN;

        case IDHW_VMMCHIPRAM:
            return 0;

        case IDHW_VMMFASTRAM:
            return 0;

        case IDHW_VMMRAM:
            return 0;

        case IDHW_PLNCHIPRAM:
            return AvailMem(MEMF_CHIP|MEMF_TOTAL);

        case IDHW_PLNFASTRAM:
            return AvailMem(MEMF_FAST|MEMF_TOTAL);

        case IDHW_PLNRAM:
            return AvailMem(MEMF_ANY|MEMF_TOTAL);

        case IDHW_VBR:
            return 0;

        case IDHW_LASTALERT:
            return -1;

        case IDHW_VBLANKFREQ:
            return SysBase->VBlankFrequency;

        case IDHW_POWERFREQ:
            return SysBase->PowerSupplyFrequency;

        case IDHW_ECLOCK:
            return SysBase->ex_EClockFrequency;

        case IDHW_SLOWRAM:
            return 0;

        case IDHW_GARY:
            return IDGRY_NONE;

        case IDHW_RAMSEY:
            return IDRSY_NONE;

        case IDHW_BATTCLOCK:
        {
            struct Library *BattClockBase = OpenResource(BATTCLOCKNAME);
            if (BattClockBase)
            {
                return ReadBattClock() ? TRUE : FALSE;
            }
            return FALSE;
        }

        case IDHW_CHUNKYPLANAR:
            //return (GfxBase->ChunkyToPlanarPtr) ? FALSE : TRUE;
            return GfxBase->HWEmul[0] ? FALSE : TRUE;

        case IDHW_POWERPC:
            #ifdef __powerpc__
                return IDPPC_OTHER;
            #else
                return IDPPC_NONE;
            #endif

        case IDHW_PPCCLOCK:
            return 0;

        case IDHW_CPUREV:
            return -1;

        case IDHW_CPUCLOCK:
            return 0;

        case IDHW_FPUCLOCK:
            return 0;

        case IDHW_RAMACCESS:
            return 0;

        case IDHW_RAMWIDTH:
            #ifdef __x86_64__
                return 64;
            #else
                return 32;
            #endif

        case IDHW_RAMCAS:
            return IDCAS_NONE;

        case IDHW_RAMBANDWIDTH:
            return 0;

        case IDHW_TCPIP:
            return IDTCP_AMITCP; // FIXME: AROSTCP ?

        case IDHW_PPCOS:
            return IDPOS_NONE;

        case IDHW_AGNUS:
            return IDAG_NONE;

        case IDHW_AGNUSMODE:
            return IDAM_NONE;

        case IDHW_DENISE:
            return IDDN_NONE;

        case IDHW_DENISEREV:
            return -1;
    }

    return 0;

    AROS_LIBFUNC_EXIT
} /* IdHardwareNum */
