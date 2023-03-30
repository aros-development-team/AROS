/*
    Copyright (C) 2022-2023, The AROS Development Team. All rights reserved.

    Desc: Runtime debug flag support
*/

#ifndef NO_RUNTIME_DEBUG

#include <exec/types.h>

#include <ctype.h>
#include <string.h>

const char * const ExecFlagNames[] =
{
    "InitResident",
    "InitCode",
    "FindResident",
    (char *)-1,         /* Reserved bit         */
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
    (char *)-1,         /* NoLogServer          */
    (char *)-1,         /* NoLogWindow          */
    (char *)-1,         /* LogFile              */
    (char *)-1,         /* LogKPrintF           */
    (char *)-1,         /* PermMemTrack         */
    "MemTrack",
    (char *)-1,         /* CyberGuardDelay      */
    "LogExtended",
    "LoadSeg",
    "UnloadSeg",
    (char *)-1,         /* PPCStart             */
    "CGXDebug",
    "InvZeroPage",
    "Shutdown",         /* Reserved bit         */
    "Init",
    NULL
};

/*
 * The following stuff is a candidate to become a public API.
 * Currently i have no idea into what component to put it, so for now
 * it's exec.library's private property.
 * The main problem is that we need it very early, before debug.library
 * and whatever else wakes up.
 */

const char *GetFlagName(ULONG flags, const char * const *FlagNames)
{
    unsigned int i;

    for (i = 0; FlagNames[i]; i++)
    {
        if (FlagNames[i] == (char *)-1)
            continue;

        if (flags & (1UL << i))
        {
            return FlagNames[i];
        }
    }
    return NULL;
}

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

#endif
