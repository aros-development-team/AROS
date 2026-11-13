/*
 * Copyright (c) 2010-2011 Matthias Rustler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *   
 * $Id$
 */

#include <exec/execbase.h>
#include <graphics/gfxbase.h>
#include <aros/inquire.h>
#include <resources/battclock.h>
#include <resources/processor.h>

#include <proto/exec.h>
#include <proto/aros.h>
#include <proto/graphics.h>
#include <proto/battclock.h>
#include <proto/processor.h>

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
        Gets information about the current system environment. The result
        is returned numerical. This function is fully DraCo compatible!

        Once a information has been read, it will be cached internally, so
        changes will be ignored. Use IdHardwareUpdate() to clear the cache
        contents.

    INPUTS
        Type    -- (ULONG) Information type. These types are known
                   (see include file and NOTE for detailed description):

                   IDHW_SYSTEM     -- What system is used?
                   (include file: IDSYS_...)

                   IDHW_CPU        -- What kind of CPU is available?
                   (include file: IDCPU_...)

                   IDHW_FPU        -- What kind of FPU is available?
                   (include file: IDFPU_...)

                   IDHW_MMU        -- What kind of MMU is available?
                   (include file: IDMMU_...)

                   IDHW_OSVER      -- What OS version is used?
                   (version, revision)

                   IDHW_EXECVER    -- What exec version is used?
                   (version, revision)

                   IDHW_WBVER      -- What WorkBench version is used?
                   (version, revision; 0 if not available)

                   IDHW_ROMSIZE    -- Size of AmigaOS ROM
                   (size in bytes)

                   IDHW_CHIPSET    -- What Chipset is available?
                   (include file: IDCS_...)

                   IDHW_GFXSYS     -- What Graphic OS is used?
                   (include file: IDGOS_...)

                   IDHW_CHIPRAM    -- Size of complete Chip RAM
                   (size in bytes)

                   IDHW_FASTRAM    -- Size of complete Fast RAM
                   (size in bytes)

                   IDHW_RAM        -- Size of complete System RAM
                   (size in bytes)

                   IDHW_SETPATCHVER -- Version of current SetPatch
                   (version, revision; 0 if not available)

                   IDHW_AUDIOSYS   -- What Audio OS is used?
                   (include file: IDAOS_...)

                   IDHW_OSNR       -- What AmigaOS is used?
                   (include file: IDOS_...)

                   IDHW_VMMCHIPRAM -- Size of virtual Chip RAM
                   (size in bytes)

                   IDHW_VMMFASTRAM -- Size of virtual Fast RAM
                   (size in bytes)

                   IDHW_VMMRAM     -- Size of total virtual RAM
                   (size in bytes)

                   IDHW_PLNCHIPRAM -- Size of non-virtual Chip RAM
                   (size in bytes)

                   IDHW_PLNFASTRAM -- Size of non-virtual Fast RAM
                   (size in bytes)

                   IDHW_PLNRAM     -- Size of total non-virtual RAM
                   (size in bytes)

                   IDHW_VBR        -- Vector Base Register contents
                   (address)

                   IDHW_LASTALERT  -- Last Alert code
                   (ULONG, 0xFFFFFFFF: no last alert yet)

                   IDHW_VBLANKFREQ -- VBlank frequency (see execbase.h)
                   (ULONG, Unit Hertz)

                   IDHW_POWERFREQ  -- Power supply frequency (see execbase.h)
                   (ULONG, Unit Hertz)

                   IDHW_ECLOCK     -- System E clock frequency
                   (ULONG, Unit Hertz)

                   IDHW_SLOWRAM    -- A500/A2000 "Slow" RAM expansion
                   (size in bytes)

                   IDHW_GARY       -- GARY revision
                   (include file: IDGRY_...)

                   IDHW_RAMSEY     -- RAMSEY revision
                   (include file: IDRSY_...)

                   IDHW_BATTCLOCK  -- Battery backed up clock present?
                   (BOOL)

                   IDHW_CHUNKYPLANAR -- [V7] Chunky to planar hardware present?
                   (BOOL)

                   IDHW_POWERPC    -- [V7] PowerPC CPU present?
                   (include file: IDPPC_...)

                   IDHW_PPCCLOCK   -- [V7] PowerPC processor clock
                   (ULONG clock in MHz units, or 0: not available)

                   IDHW_CPUREV     -- [V8] Revision of the main processor
                   (LONG revision or -1 if not available)

                   IDHW_CPUCLOCK   -- [V8] CPU clock
                   (ULONG clock in MHz units)

                   IDHW_FPUCLOCK   -- [V8] FPU clock, if available
                   (ULONG clock in MHz units, or 0: not available)

                   IDHW_RAMACCESS  -- [V8] Access time of the main board RAM
                   (ULONG in ns units, or 0: not available)

                   IDHW_RAMWIDTH   -- [V8] Width of the main board RAM
                   (ULONG in bit, or 0: not available)

                   IDHW_RAMCAS     -- [V8] CAS mode of the main board RAM
                   (include file: IDCAS_...)

                   IDHW_RAMBANDWIDTH -- [V8] Bandwidth of the main board RAM
                   (ULONG in times, or 0: not available)

                   IDHW_TCPIP      -- [V9] Used TCP/IP stack
                   (include file: IDTCP_...)

                   IDHW_PPCOS      -- [V9] Used PowerPC OS
                   (include file: IDPOS_...)

                   IDHW_AGNUS      -- [V9] Agnus chip type and revision
                   (include file: IDAG_...)

                   IDHW_AGNUSMODE  -- [V9] Agnus chip mode
                   (include file: IDAM_...)

                   IDHW_DENISE     -- [V10] Denise chip type
                   (include file: IDDN_...)

                   IDHW_DENISEREV  -- [V10] Denise chip revision
                   (LONG, -1 means not available)

        TagList -- (struct TagItem *) tags that describe further
                   options. You may provide NULL.

    TAGS
        None yet.

    RESULT
        Result -- (ULONG) Numerical result containing the desired
                  information.

    NOTES
        Some results are nonsense on AROS.

        If you queried a version, you'll find the version in the *lower*
        UWORD (because it is more important) and the revision in the
        *upper* UWORD.

        All memory sizes are always in bytes.

        Boolean results are ==0 for FALSE, !=0 for TRUE.

        If you have to look up the result in the include file, you might
        also get a numerical result that is beyond the maximum value you'll
        find there. Be prepared for it! In this case, just print "not known"
        or anything similar, or use the IdHardware() result.

    EXAMPLE

    BUGS

    SEE ALSO
        IdHardware(), IdHardwareUpdate()

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
            #if defined __i386__
            return IDCPU_i386;
            #elif defined __x86_64__
            return IDCPU_x86_64;
            #elif defined __mc68000__
            return IDCPU_68000;
            #elif defined __powerpc__
            return IDCPU_ppc;
            #elif defined __arm__
            return IDCPU_arm;
            #else
            return IDCPU_unknown;
            #endif

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
            return version | (revision << 16);
        }

        case IDHW_EXECVER:
            return
                ((struct Library *)SysBase)->lib_Version
                |
                (((struct Library *)SysBase)->lib_Revision << 16);

        case IDHW_WBVER:
            return 40; // i.e. OS3.1

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
            return IDOS_3_1;

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
            return SysBase->LastAlert[0];

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
            return GfxBase->ChunkyToPlanarPtr ? FALSE : TRUE;

        case IDHW_POWERPC:
            return IDPPC_NONE;

        case IDHW_PPCCLOCK:
            return 0;

        case IDHW_CPUREV:
            return -1;

        case IDHW_CPUCLOCK:
        {
            QUAD speed = 0;
            APTR ProcessorBase = OpenResource(PROCESSORNAME);
            if (ProcessorBase)
            {
                struct TagItem tags [] = 
                {
                    { GCIT_ProcessorSpeed, (IPTR)&speed },
                    { TAG_DONE }
                };
                GetCPUInfo(tags);
            }
            return speed / 1000000;
        }
        case IDHW_FPUCLOCK:
            return 0;

        case IDHW_RAMACCESS:
            return 0;

        case IDHW_RAMWIDTH:
            if (GfxBase->MemType & 1)
                return 32;
            else
                return 16;

        case IDHW_RAMCAS:
            if (GfxBase->MemType & 2)
                return IDCAS_DOUBLE;
            else
                return IDCAS_NORMAL;

        case IDHW_RAMBANDWIDTH:
            return 0;

        case IDHW_TCPIP:
            return IDTCP_AMITCP;

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

        case IDHW_BOINGBAG:
            return 0;

        case IDHW_EMULATED:
            return 0;

        case IDHW_XLVERSION:
            return 0;

        case IDHW_HOSTOS:
            return 0;

        case IDHW_HOSTVERS:
            return 0;

        case IDHW_HOSTMACHINE:
            return 0;

        case IDHW_HOSTCPU:
            return 0;

        case IDHW_HOSTSPEED:
            return 0;
    }

    return 0;

    AROS_LIBFUNC_EXIT
} /* IdHardwareNum */
