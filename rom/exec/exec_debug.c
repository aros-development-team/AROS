/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: rawdofmt.c 37154 2011-02-22 18:42:00Z jmcmullan $

    Desc: Runtime debugging support
    Lang: english
*/

/*
 * You can #define this in order to omit the whole code. Perhaps ROM-based systems
 * will want to do this.
 * However it's not adviced to to so because this is a great aid in debugging on user's side.
 */
#ifndef NO_RUNTIME_DEBUG

#include <exec/execbase.h>
#include <exec/rawfmt.h>
#include <proto/exec.h>

#include <string.h>

#include "exec_debug.h"

const char *ExecFlagNames[] =
{
    "InitResident",
    "InitCode",
    "FindResident",
    (char *)-1,
    "CreateLibrary",
    "SetFunction",
    "NewSetFunction",
    "ChipRam",
    "AddTask",
    "RemTask",
    /* This array is incomplete, TODO */
    NULL
};

void ExecLog(struct ExecBase *SysBase, ULONG flags, const char *format, ...)
{
    va_list ap;

    flags &= SysBase->ex_DebugFlags;
    if (!flags)
    	return;

    va_start(ap, format);
    VLog(SysBase, flags, ExecFlagNames, format, ap);
    va_end(ap);
}

/*
 * The following stuff is a candidate to become a public API.
 * Currently i have no idea into what component to put it, so for now
 * it's exec.library's private property.
 */

/* Only shared version of our libc has isalpha(), so we use own implementation */
static int IsAlpha(char c)
{
    if ((c >= 'A') && (c <= 'Z'))
    	return 1;
    if ((c >= 'a') && (c <= 'z'))
    	return 1;

    return 0;
}

/*
 * Return a set of flags specified on the command line.
 * Option format: <flag1>,<flags>,...,<flagN>
 * Or:            "<flag1> <flag2> ... <flagN>"
 */
ULONG ParseFlags(char *opts, const char **FlagNames)
{
    ULONG ret = 0;
    char quoted = 0;

    if (*opts == '"')
    {
        quoted = 1;
    	opts++;
    }

    while (IsAlpha(*opts))
    {
    	char *p = opts + 1;
    	unsigned int i;

	/* Find the end of the word */
    	while (IsAlpha(*p))
	    p++;

	/* "ALL" means all flags */
	if (!strnicmp(opts, "all", 3))
	    return -1;

	/* Decode flag name */
	for (i = 0; FlagNames[i]; i++)
    	{
    	    if (FlagNames[i] == (char *)-1)
    	    	continue;

    	    if (!strnicmp(opts, FlagNames[i], p - opts))
    	    {
    	    	ret |= (1UL << i);
    	    	break;
    	    }
    	}

	if (quoted)
	{
	    /* Skip separator characters */
	    while (!IsAlpha(*p))
	    {
	    	/* If we hit closing quotes or end of line, this is the end */
	    	if (*p == '"')
	    	    return ret;

	    	if (*p == 0)
	    	    return ret;
	    }

	    /* Next word is found */
	    opts = p;
	}
	else
	{
	    /* If the string is not quoted, only single comma is allowed as a separator */
    	    if (*p != ',')
		break;

	    opts = p + 1;
	}
    }

    return ret;
}

void VLog(struct ExecBase *SysBase, ULONG flags, const char **FlagNames, const char *format, va_list args)
{
    unsigned int i;

    /* Prepend tag (if known) */
    for (i = 0; FlagNames[i]; i++)
    {
    	if (FlagNames[i] == (char *)-1)
    	    continue;

    	if (flags & (1UL << i))
    	{
	    RawDoFmt("[%s] ", &FlagNames[i], (VOID_FUNC)RAWFMTFUNC_SERIAL, NULL);
	    break;
	}
    }

    /* Output the message and append a newline (in order not to bother about it every time) */
    VNewRawDoFmt(format, (VOID_FUNC)RAWFMTFUNC_SERIAL, NULL, args);
    RawPutChar('\n');
}

#endif
