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

#include <proto/utility.h>

#include <stdio.h>

#include "identify_intern.h"
#include "identify.h"

static CONST_STRPTR handle_version(TEXT *buffer, Tag tag);
static CONST_STRPTR handle_size(TEXT *buffer, Tag tag);
static CONST_STRPTR handle_freq(TEXT *buffer, Tag tag);
static CONST_STRPTR handle_number(TEXT *buffer, Tag tag);
static CONST_STRPTR handle_avail(Tag tag, BOOL null4na);
static CONST_STRPTR handle_notavail(BOOL null4na);

/*****************************************************************************

    NAME */
#include <proto/identify.h>

        AROS_LH2(CONST_STRPTR, IdHardware,

/*  SYNOPSIS */
        AROS_LHA(ULONG           , type   , D0),
        AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */
        struct IdentifyBaseIntern *, IdentifyBase, 6, Identify)

/*  FUNCTION
        Gets information about the current system environment. The result
        is returned as read only string. This function is fully DraCo
        compatible!

        Once a information has been evaluated, it will be cached internally,
        so changes will be ignored. Use IdHardwareUpdate() to clear the cache
        contents.

    INPUTS
       Type -- (ULONG) Information type. These types are known:

               IDHW_SYSTEM     -- What system is used?
               (e. g. "Amiga 4000")

               IDHW_CPU        -- What kind of CPU is available?
               (e. g. "68060")

               IDHW_FPU        -- What kind of FPU is available?
               (e. g. "68060")

               IDHW_MMU        -- What kind of MMU is available?
               (e. g. "68060")

               IDHW_OSVER      -- What OS version is used?
               (e.g. "V39.106")

               IDHW_EXECVER    -- What exec version is used?
               (e.g. "V39.47")

               IDHW_WBVER      -- What WorkBench version is used?
               (e.g. "V39.29")

               IDHW_ROMSIZE    -- Size of AmigaOS ROM
               (e.g. "512KB")

               IDHW_CHIPSET    -- What Chipset is available?
               (e.g. "AGA")

               IDHW_GFXSYS     -- What Graphic OS is used?
               (e.g. "CyberGraphX")

               IDHW_CHIPRAM    -- Size of complete Chip RAM
               (e.g. "~2.0MB")

               IDHW_FASTRAM    -- Size of complete Fast RAM
               (e.g. "12.0MB")

               IDHW_RAM        -- Size of complete System RAM
               (e.g. "~14.0MB")

               IDHW_SETPATCHVER -- [V4] Version of current SetPatch
               (e.g. "V40.14")

               IDHW_AUDIOSYS   -- [V5] What Audio OS is used?
               (e.g. "AHI")

               IDHW_OSNR       -- [V5] What AmigaOS is used?
               (e.g. "3.1")

               IDHW_VMMCHIPRAM -- [V5] Size of virtual Chip RAM
               (e.g. "0")

               IDHW_VMMFASTRAM -- [V5] Size of virtual Fast RAM
               (e.g. "40.0MB")

               IDHW_VMMRAM     -- [V5] Size of total virtual RAM
               (e.g. "40.0MB")

               IDHW_PLNCHIPRAM -- [V5] Size of non-virtual Chip RAM
               (e.g. "2.0MB")

               IDHW_PLNFASTRAM -- [V5] Size of non-virtual Fast RAM
               (e.g. "12.0MB")

               IDHW_PLNRAM     -- [V5] Size of total non-virtual RAM
               (e.g. "14.0MB")

               IDHW_VBR        -- [V6] Vector Base Register contents
               (e.g. "0x0806C848")

               IDHW_LASTALERT  -- [V6] Last Alert code
               (e.g. "80000003")

               IDHW_VBLANKFREQ -- [V6] VBlank frequency (see execbase.h)
               (e.g. "50 Hz")

               IDHW_POWERFREQ  -- [V6] Power supply frequency (see execbase.h)
               (e.g. "50 Hz")

               IDHW_ECLOCK     -- [V6] System E clock frequency
               (e.g. "709379 Hz")

               IDHW_SLOWRAM    -- [V6] A500/A2000 "Slow" RAM expansion
               (e.g. "512.0KB")

               IDHW_GARY       -- [V6] GARY revision
               (e.g. "Normal")

               IDHW_RAMSEY     -- [V6] RAMSEY revision
               (e.g. "F")

               IDHW_BATTCLOCK  -- [V6] Battery backed up clock present?
               (e.g. "Found")

               IDHW_CHUNKYPLANAR -- [V7] Chunky to planar hardware present?
               (e.g. "Found")

               IDHW_POWERPC    -- [V7] PowerPC CPU present?
               (e.g. "603e")

               IDHW_PPCCLOCK   -- [V7] PowerPC processor clock
               (e.g. "200 MHz")

               IDHW_CPUREV     -- [V8] Revision of the main processor, if
               available (e.g. "Rev 1")

               IDHW_CPUCLOCK   -- [V8] CPU clock
               (e.g. "50 MHz")

               IDHW_FPUCLOCK   -- [V8] FPU clock, if available
               (e.g. "50 MHz")

               IDHW_RAMACCESS  -- [V8] Access time of the main board RAM
               (e.g. "80 ns")

               IDHW_RAMWIDTH   -- [V8] Width of the main board RAM
               (e.g. "16 bit")

               IDHW_RAMCAS     -- [V8] CAS mode of the main board RAM
               (e.g. "Double")

               IDHW_RAMBANDWIDTH -- [V8] Bandwidth of the main board RAM
               (e.g. "2")

               IDHW_TCPIP      -- [V9] Used TCP/IP stack
               (e.g. "AmiTCP/IP")

               IDHW_PPCOS      -- [V9] Used PowerPC OS
               (e.g. "PowerUp")

               IDHW_AGNUS      -- [V9] Agnus chip type and revision
               (e.g. "Alice 8374 Rev. 3-4")

               IDHW_AGNUSMODE  -- [V9] Agnus chip mode
               (e.g. "PAL")

               IDHW_DENISE     -- [V10] Denise chip type
               (e.g. "Lisa 8364")

               IDHW_DENISEREV  -- [V10] Denise chip revision
               (e.g. "0")

        TagList -- (struct TagItem *) tags that describe further
                   options. You may provide NULL.

    TAGS
        IDTAG_Localize  -- [V8] (BOOL) FALSE to get English strings
                           only, TRUE for localized strings. This is useful for applications
                           with English as only language. Defaults to TRUE.

        IDTAG_NULL4NA   -- [V8] (BOOL) TRUE to get NULL pointer instead
                           of a 'not available' string. Defaults to FALSE.

    RESULT
        String  -- (STRPTR) String containing the desired
                   information, or NULL if not available. Note that
                   all strings are READ ONLY!

    NOTES
        Some results are nonsense on AROS.

    EXAMPLE

    BUGS

    SEE ALSO
        IdHardwareNum(), IdHardwareUpdate()

    INTERNALS

    HISTORY


*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tag;
    const struct TagItem *tags;

    CONST_STRPTR result = NULL;
    BOOL null4na = FALSE;
    BOOL localize = TRUE;

    for (tags = taglist; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
            case IDTAG_NULL4NA:
                null4na = tag->ti_Data ? TRUE : FALSE;
                break;

            case IDTAG_Localize:
                localize = tag->ti_Data ? TRUE : FALSE;
                break;
        }
    }

    ObtainSemaphore(&IdentifyBase->sem);

    switch(type)
    {
        case IDHW_SYSTEM:
            result = "AROS";
            break;

        case IDHW_CPU:
            result = "68000"; // FIXME
            break;

        case IDHW_FPU:
            result = handle_notavail(null4na);
            break;

        case IDHW_MMU:
            result = handle_notavail(null4na);
            break;

        case IDHW_OSVER:
            result = handle_version(IdentifyBase->hwb.buf_OsVer, IDHW_OSVER);
            break;

        case IDHW_EXECVER:
            result = handle_version(IdentifyBase->hwb.buf_ExecVer, IDHW_EXECVER);
            break;

        case IDHW_WBVER:
            result = handle_version(IdentifyBase->hwb.buf_WbVer, IDHW_WBVER);
            break;

        case IDHW_ROMSIZE:
            result = handle_size(IdentifyBase->hwb.buf_RomSize, IDHW_ROMSIZE);
            break;

        case IDHW_CHIPSET:
            result = "OCS";
            break;

        case IDHW_GFXSYS:
            result = "CyberGraphX";
            break;

        case IDHW_CHIPRAM:
            result = handle_size(IdentifyBase->hwb.buf_ChipRAM, IDHW_CHIPRAM);
            break;

        case IDHW_FASTRAM:
            result = handle_size(IdentifyBase->hwb.buf_FastRAM, IDHW_FASTRAM);
            break;

        case IDHW_RAM:
            result = handle_size(IdentifyBase->hwb.buf_RAM, IDHW_RAM);
            break;

        case IDHW_SETPATCHVER:
            result = handle_version(IdentifyBase->hwb.buf_SetPatchVer, IDHW_SETPATCHVER);
            break;

        case IDHW_AUDIOSYS:
            result = "AHI";
            break;

        case IDHW_OSNR:
            result = "AROS";
            break;

        case IDHW_VMMCHIPRAM:
            result = "0 Bytes";
            break;

        case IDHW_VMMFASTRAM:
            result = "0 Bytes";
            break;

        case IDHW_VMMRAM:
            result = "0 Bytes";
            break;

        case IDHW_PLNCHIPRAM:
            result = handle_size(IdentifyBase->hwb.buf_PlainChipRAM, IDHW_PLNCHIPRAM);
            break;

        case IDHW_PLNFASTRAM:
            result = handle_size(IdentifyBase->hwb.buf_PlainFastRAM, IDHW_PLNFASTRAM);
            break;

        case IDHW_PLNRAM:
            result = handle_size(IdentifyBase->hwb.buf_PlainRAM, IDHW_PLNCHIPRAM);
            break;

        case IDHW_VBR:
            result = "0x00000000";
            break;

        case IDHW_LASTALERT:
            result = "0x00000000";
            break;

        case IDHW_VBLANKFREQ:
            result = handle_freq(IdentifyBase->hwb.buf_VBlankFreq, IDHW_VBLANKFREQ);
            break;

        case IDHW_POWERFREQ:
            result = "0 Hz";
            break;

        case IDHW_ECLOCK:
            result = handle_freq(IdentifyBase->hwb.buf_EClock, IDHW_ECLOCK);
            break;

        case IDHW_SLOWRAM:
            result = handle_size(IdentifyBase->hwb.buf_SlowRAM, IDHW_SLOWRAM);
            break;

        case IDHW_GARY:
            result = handle_notavail(null4na);
            break;

        case IDHW_RAMSEY:
            result = handle_notavail(null4na);
            break;

        case IDHW_BATTCLOCK:
            result = handle_avail(IDHW_BATTCLOCK, null4na);
            break;

        case IDHW_CHUNKYPLANAR:
            result = handle_avail(IDHW_CHUNKYPLANAR, null4na);
            break;

        case IDHW_POWERPC:
            #ifdef __powerpc__
                result = "Found";
            #else
                result = handle_notavail(null4na);
            #endif
            break;

        case IDHW_PPCCLOCK:
            result = "0 MHz";
            break;

        case IDHW_CPUREV:
            result = handle_notavail(null4na);
            break;

        case IDHW_CPUCLOCK:
            result = "0 MHz";
            break;

        case IDHW_FPUCLOCK:
            result = "0 MHz";
            break;

        case IDHW_RAMACCESS:
            result = handle_notavail(null4na);
            break;

        case IDHW_RAMWIDTH:
            result = handle_number(IdentifyBase->hwb.buf_RAMWidth, IDHW_RAMWIDTH);
            break;

        case IDHW_RAMCAS:
        {
            ULONG num = IdHardwareNum(IDHW_RAMCAS, NULL);
            if (num == 2)
            {
                result = "Double";
            }
            else
            {
                result = "Normal";
            }
            break;
        }

        case IDHW_RAMBANDWIDTH:
            result = handle_notavail(null4na);
            break;

        case IDHW_TCPIP:
            result = "AmiTCP/IP";
            break;

        case IDHW_PPCOS:
            result = "None";
            break;

        case IDHW_AGNUS:
            result = "None";
            break;

        case IDHW_AGNUSMODE:
            result = "None";
            break;

        case IDHW_DENISE:
            result = "None";
            break;

        case IDHW_DENISEREV:
            result = handle_notavail(null4na);
            break;

        default:
            result = handle_notavail(null4na);
            break;
    }

    ReleaseSemaphore(&IdentifyBase->sem);

    return result;

    AROS_LIBFUNC_EXIT
} /* IdHardware */


static CONST_STRPTR handle_version(TEXT *buffer, Tag tag)
{
    CONST_STRPTR result = buffer;
    if (*buffer == '\0')
    {
        ULONG num = IdHardwareNum(tag, NULL);
        if (num != 0 && num != -1)
        {
            ULONG version = num & 0xffff;
            ULONG revision = num >> 16;
            snprintf(buffer, STRBUFSIZE, "V%u.%u", version, revision);
        }
        else
        {
            sprintf(buffer, "N/A");
        }
    }
    return result;
}

static CONST_STRPTR handle_size(TEXT *buffer, Tag tag)
{
    CONST_STRPTR result = buffer;
    STRPTR unit = "";

    if (*buffer == '\0')
    {
        UQUAD num = IdHardwareNum(tag, NULL);
        if (num > 1024 * 1024 * 1024)
        {
            num = num / 1024 / 1024 / 1024;
            unit = "GB";
        }
        else if (num > 1024 * 1024)
        {
            num = num / 1024 / 1024;
            unit = "MB";
        }
        else if (num > 1024)
        {
            num = num / 1024;
            unit = "KB";
        }
        snprintf(buffer, STRBUFSIZE, "%lu %s", (long unsigned int)num, unit);
    }
    return result;
}

static CONST_STRPTR handle_freq(TEXT *buffer, Tag tag)
{
    CONST_STRPTR result = buffer;
    if (*buffer == '\0')
    {
        ULONG num = IdHardwareNum(tag, NULL);
        {
            snprintf(buffer, STRBUFSIZE, "%u Hz", num);
        }
    }
    return result;
}

static CONST_STRPTR handle_number(TEXT *buffer, Tag tag)
{
    CONST_STRPTR result = buffer;
    if (*buffer == '\0')
    {
        ULONG num = IdHardwareNum(tag, NULL);
        {
            snprintf(buffer, STRBUFSIZE, "%u", num);
        }
    }
    return result;
}

static CONST_STRPTR handle_avail(Tag tag, BOOL null4na)
{
    ULONG num = IdHardwareNum(tag, NULL);
    if (num)
    {
        return "Found";
    }
    else
    {
        return handle_notavail(null4na);
    }
}

static CONST_STRPTR handle_notavail(BOOL null4na)
{
    if (null4na == FALSE)
        return "None";
    else
        return NULL;
}
