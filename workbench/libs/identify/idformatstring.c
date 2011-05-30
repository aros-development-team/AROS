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

#include <aros/debug.h>

#include <string.h>

#include "identify_intern.h"

static CONST_STRPTR commands[] =
{
    "SYSTEM$",
    "CPU$",
    "FPU$",
    "MMU$",
    "OSVER$",
    "EXECVER$",
    "WBVER$",
    "ROMSIZE$",
    "CHIPSET$",
    "GFXSYS$",
    "CHIPRAM$",
    "FASTRAM$",
    "RAM$",
    "SETPATCHVER$",
    "AUDIOSYS$",
    "OSNR$",
    "VMMCHIPRAM$",
    "VMMFASTRAM$",
    "VMMRAM$",
    "PLNCHIPRAM$",
    "PLNFASTRAM$",
    "PLNRAM$",
    "VBR$",
    "LASTALERT$",
    "VBLANKFREQ$",
    "POWERFREQ$",
    "ECLOCK$",
    "SLOWRAM$",
    "GARY$",
    "RAMSEY$",
    "BATTCLOCK$",
    "CHUNKYPLANAR$",
    "POWERPC$",
    "PPCCLOCK$",
    "CPUREV$",
    "CPUCLOCK$",
    "FPUCLOCK$",
    "RAMACCESS$",
    "RAMWIDTH$",
    "RAMCAS$",
    "RAMBANDWIDTH$",
    "TCPIP$",
    "PPCOS$",
    "AGNUS$",
    "AGNUSMODE$",
    "DENISE$",
    "DENISEREV$",
    "BOINGBAG$",
    "EMULATED$",
    "XLVERSION$",
    "HOSTOS$",
    "HOSTVERS$",
    "HOSTMACHINE$",
    "HOSTCPU$",
    "HOSTSPEED$"
};

static LONG findcommand(TEXT *);

/*****************************************************************************

    NAME */
#include <proto/identify.h>

        AROS_LH4(ULONG, IdFormatString,

/*  SYNOPSIS */
        AROS_LHA(STRPTR          , string, A0),
        AROS_LHA(STRPTR          , buffer, A1),
        AROS_LHA(ULONG           , len   , D0),
        AROS_LHA(struct TagItem *, tags  , A2),

/*  LOCATION */
        struct IdentifyBaseIntern *, IdentifyBase, 11, Identify)

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

    TEXT *from = string;
    TEXT *to = buffer;
    ULONG size = len;
    LONG commandindex;
    CONST_STRPTR toinsert;
    ULONG cpycnt;

    if (from == NULL || to == NULL || len < 1)
    {
        return 0;
    }

    len--; // for '\0'

	while (len > 0 && *from)
	{
	    if (*from == '$')
	    {
	        from++;
	        if (*from == '$')
	        {
	            *to++ = '$';
	            from++;
	            len--;
	        }
	        else if ((commandindex = findcommand(from)) != -1)
	        {
                toinsert = IdHardware(commandindex, NULL);
                cpycnt = strlen(toinsert);
                if (cpycnt > len)
                {
                    cpycnt = len;
                }
                memcpy(to, toinsert, cpycnt);
                from += strlen(commands[commandindex]);
                to += cpycnt;
                len -= cpycnt;
	        }
	    }
	    else
	    {
	        *to++ = *from++;
	        len--;
	    }
	}
	*to = '\0';

    return size - len;

    AROS_LIBFUNC_EXIT
} /* IdFormatString */


static LONG findcommand(TEXT *t)
{
    int i;

    for (i = 0; i < sizeof commands / sizeof (STRPTR); i++)
    {
        if (strncmp(t, commands[i], strlen(commands[i])) == 0)
        {
            return i;
        }
    }
    return -1;
}
