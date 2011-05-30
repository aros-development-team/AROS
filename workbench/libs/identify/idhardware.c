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
