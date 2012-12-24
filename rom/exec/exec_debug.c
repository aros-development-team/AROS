/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Runtime debugging support
    Lang: english
*/

/*
 * You can #define this in order to omit the whole code. Perhaps ROM-based systems
 * will want to do this.
 * However it's not advised to do so because this is a great aid in debugging
 * on user's side.
 */
#ifndef NO_RUNTIME_DEBUG

#include <exec/execbase.h>
#include <exec/rawfmt.h>
#include <proto/exec.h>

#include <ctype.h>
#include <string.h>

#include "exec_debug.h"

const char * const ExecFlagNames[] =
{
    "InitResident",
    "InitCode",
    "FindResident",
    (char *)-1,		/* Reserved bit		*/
    "CreateLibrary",
    "SetFunction",
    "NewSetFunction",
    "ChipRam",
    "AddTask",
    "RemTask",
    "GetTaskAttr",
    "SetTaskAttr",
    "ExceptHandler",
    "AddDosNode",
    "PCI",
    "RamLib",
    (char *)-1,		/* NoLogServer		*/
    (char *)-1,		/* NoLogWindow		*/
    (char *)-1,		/* LogFile		*/
    (char *)-1,		/* LogKPrintF		*/
    (char *)-1,		/* PermMemTrack		*/
    "MemTrack",
    (char *)-1,		/* CyberGuardDelay	*/
    "LogExtended",
    "LoadSeg",
    "UnloadSeg",
    (char *)-1,		/* PPCStart		*/
    "CGXDebug",
    "InvZeroPage",
    (char *)-1,		/* Reserved bit		*/
    "Init",
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
 * The main problem is that we need it very early, before debug.library
 * and whatever else wakes up.
 */

/*
 * Return a set of flags specified on the command line.
 * Option format: <flag1>,<flags>,...,<flagN>
 * Or:            "<flag1> <flag2> ... <flagN>"
 */
ULONG ParseFlags(char *opts, const char * const *FlagNames)
{
    ULONG ret = 0;
    char quoted = 0;

    if (*opts == '"')
    {
        quoted = 1;
    	opts++;
    }

    while (isalpha(*opts))
    {
    	char *p = opts + 1;
    	unsigned int i;

	/* Find the end of the word */
    	while (isalpha(*p))
	    p++;

	/* "ALL" means all flags */
	if (!strnicmp(opts, "all", 3))
	    return -1;

	/* Decode flag name */
	for (i = 0; FlagNames[i]; i++)
    	{
    	    const char *flagName = FlagNames[i];

    	    if (flagName == (char *)-1)
    	    	continue;

    	    if (!strnicmp(opts, flagName, strlen(flagName)))
    	    {
    	    	ret |= (1UL << i);
    	    	break;
    	    }
    	}

	if (quoted)
	{
	    /* Skip separator characters */
	    while (!isalpha(*p))
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

void VLog(struct ExecBase *SysBase, ULONG flags, const char * const *FlagNames, const char *format, va_list args)
{
    unsigned int i;

    /* Prepend tag (if known) */
    for (i = 0; FlagNames[i]; i++)
    {
    	if (FlagNames[i] == (char *)-1)
    	    continue;

    	if (flags & (1UL << i))
    	{
	    RawDoFmt("[%s] ", (APTR)&FlagNames[i], (VOID_FUNC)RAWFMTFUNC_SERIAL, NULL);
	    break;
	}
    }

    /* Output the message and append a newline (in order not to bother about it every time) */
    VNewRawDoFmt(format, (VOID_FUNC)RAWFMTFUNC_SERIAL, NULL, args);
    RawPutChar('\n');
}

#endif
