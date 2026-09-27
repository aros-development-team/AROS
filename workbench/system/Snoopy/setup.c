/*
    Copyright (C) 2006-2026, The AROS Development Team. All rights reserved.
*/

//#define DEBUG 1
#include <aros/debug.h>

#include "setup.h"

#include <proto/dos.h>
#include <proto/exec.h>
#include <stdio.h>
#include <string.h>

struct Setup setup;
struct Setup oldsetup;

enum
{
    onlyShowFailsOpt,
    useDevNamesOpt,
    showPathsOpt,
    showCliNrOpt,
    ignoreWBOpt,
    breakPointOpt,

    ChangeDirOpt,
    DeleteOpt,
    ExecuteOpt,
    GetVarOpt,
    LoadSegOpt,
    LockOpt,
    MakeDirOpt,
    MakeLinkOpt,
    OpenOpt,
    RenameOpt,
    RunCommandOpt,
    SetVarOpt,
    SystemOpt,

    FindPortOpt,
    FindResidentOpt,
    FindSemaphoreOpt,
    FindTaskOpt,
    LockScreenOpt,
    OpenDeviceOpt,
    OpenFontOpt,
    OpenLibraryOpt,
    OpenResourceOpt,
    ReadToolTypesOpt,

    nameLenOpt,
    actionLenOpt,
    targetLenOpt,
    optionLenOpt
};

// must be same order than previous enum
static CONST_STRPTR opts[] =
{
    "ShowFails",
    "DeviceNames",
    "Paths",
    "CliNr",
    "IgnoreWB",
    "BreakPoint",

    "ChangeDir",
    "Delete",
    "Execute",
    "GetVar",
    "LoadSeg",
    "LockOpt",
    "MakeDir",
    "MakeLink",
    "OpenOpt",
    "RenameOpt",
    "RunCommandOpt",
    "SetVarOpt",
    "SystemOpt",

    "FindPort",
    "FindResident",
    "FindSemaphore",
    "FindTask",
    "LockScreen",
    "OpenDevice",
    "OpenFont",
    "OpenLibrary",
    "OpenResource",
    "ReadToolTypes",

    "NameLen",
    "ActionLen",
    "TargetLen",
    "OptionLen"
};

static CONST_STRPTR patternOpt = "Pattern";

static BOOL setup_write_parameter(BPTR fh, int argindex, int value)
{
    static char buffer[50];
    sprintf(buffer, "%s %d\n", opts[argindex], value);
    return FPuts(fh, (STRPTR)buffer) == 0;
}

static BOOL setup_write_pattern(BPTR fh, CONST_STRPTR pattern)
{
    if (FPuts(fh, (STRPTR)patternOpt) != 0)
        return FALSE;
    if (FPuts(fh, " ") != 0)
        return FALSE;
    if (FPuts(fh, (STRPTR)pattern) != 0)
        return FALSE;
    return FPuts(fh, "\n") == 0;
}

void setup_init(void)
{
    setup_reset();
    setup_open();
}
    
void setup_reset(void)
{
    setup.onlyShowFails       = FALSE;
    setup.useDevNames         = FALSE;
    setup.showPaths           = FALSE;
    setup.showCliNr           = FALSE;
    setup.ignoreWB            = FALSE;
    setup.breakPoint          = FALSE;

    setup.match               = FALSE;
    setup.pattern[0]          = '\0';
    setup.parsedpattern[0]    = '\0';

    setup.enableChangeDir     = FALSE;
    setup.enableDelete        = FALSE;
    setup.enableExecute       = FALSE;
    setup.enableGetVar        = FALSE;
    setup.enableLoadSeg       = FALSE;
    setup.enableLock          = FALSE;
    setup.enableMakeDir       = FALSE;
    setup.enableMakeLink      = FALSE;
    setup.enableOpen          = TRUE;
    setup.enableRename        = FALSE;
    setup.enableRunCommand    = FALSE;
    setup.enableSetVar        = FALSE;
    setup.enableSystem        = FALSE;

    setup.enableFindPort      = FALSE;
    setup.enableFindResident  = FALSE;
    setup.enableFindSemaphore = FALSE;
    setup.enableFindTask      = FALSE;
    setup.enableLockScreen    = FALSE;
    setup.enableOpenDevice    = FALSE;
    setup.enableOpenFont      = FALSE;
    setup.enableOpenLibrary   = TRUE;
    setup.enableOpenResource  = FALSE;
    setup.enableReadToolTypes = FALSE;

    setup.nameLen   = 15;
    setup.actionLen = 15;
    setup.targetLen = 40;
    setup.optionLen = 15;

    oldsetup = setup;
}

BOOL setup_save(void)
{
    // temporary disable breakpoint option to ensure that
    // a) Open() and other functions aren't interrupted while saving the prefs file
    // b) the breakpoint option is always saved as "FALSE" to the prefs file
    BOOL breakPoint = setup.breakPoint;
    setup.breakPoint = FALSE;

    BOOL retvalue = FALSE;

    BPTR fh = Open(PREFFILE, MODE_NEWFILE);
    if (fh)
    {
        retvalue = TRUE;
        retvalue &= setup_write_parameter(fh, onlyShowFailsOpt, setup.onlyShowFails);
        retvalue &= setup_write_parameter(fh, useDevNamesOpt,   setup.useDevNames);
        retvalue &= setup_write_parameter(fh, showPathsOpt,     setup.showPaths);
        retvalue &= setup_write_parameter(fh, showCliNrOpt,     setup.showCliNr);
        retvalue &= setup_write_parameter(fh, ignoreWBOpt,      setup.ignoreWB);
        retvalue &= setup_write_parameter(fh, breakPointOpt,    setup.breakPoint);

        retvalue &= setup_write_pattern(fh, setup.pattern);

        retvalue &= setup_write_parameter(fh, ChangeDirOpt,     setup.enableChangeDir);
        retvalue &= setup_write_parameter(fh, DeleteOpt,        setup.enableDelete);
        retvalue &= setup_write_parameter(fh, ExecuteOpt,       setup.enableExecute);
        retvalue &= setup_write_parameter(fh, GetVarOpt,        setup.enableGetVar);
        retvalue &= setup_write_parameter(fh, LoadSegOpt,       setup.enableLoadSeg);
        retvalue &= setup_write_parameter(fh, LockOpt,          setup.enableLock);
        retvalue &= setup_write_parameter(fh, MakeDirOpt,       setup.enableMakeDir);
        retvalue &= setup_write_parameter(fh, MakeLinkOpt,      setup.enableMakeLink);
        retvalue &= setup_write_parameter(fh, OpenOpt,          setup.enableOpen);
        retvalue &= setup_write_parameter(fh, RenameOpt,        setup.enableRename);
        retvalue &= setup_write_parameter(fh, RunCommandOpt,    setup.enableRunCommand);
        retvalue &= setup_write_parameter(fh, SetVarOpt,        setup.enableSetVar);
        retvalue &= setup_write_parameter(fh, SystemOpt,        setup.enableSystem);

        retvalue &= setup_write_parameter(fh, FindPortOpt,      setup.enableFindPort);
        retvalue &= setup_write_parameter(fh, FindResidentOpt,  setup.enableFindResident);
        retvalue &= setup_write_parameter(fh, FindSemaphoreOpt, setup.enableFindSemaphore);
        retvalue &= setup_write_parameter(fh, FindTaskOpt,      setup.enableFindTask);
        retvalue &= setup_write_parameter(fh, LockScreenOpt,    setup.enableLockScreen);
        retvalue &= setup_write_parameter(fh, OpenDeviceOpt,    setup.enableOpenDevice);
        retvalue &= setup_write_parameter(fh, OpenFontOpt,      setup.enableOpenFont);
        retvalue &= setup_write_parameter(fh, OpenLibraryOpt,   setup.enableOpenLibrary);
        retvalue &= setup_write_parameter(fh, OpenResourceOpt,  setup.enableOpenResource);
        retvalue &= setup_write_parameter(fh, ReadToolTypesOpt, setup.enableReadToolTypes);

        retvalue &= setup_write_parameter(fh, nameLenOpt,       setup.nameLen);
        retvalue &= setup_write_parameter(fh, actionLenOpt,     setup.actionLen);
        retvalue &= setup_write_parameter(fh, targetLenOpt,     setup.targetLen);
        retvalue &= setup_write_parameter(fh, optionLenOpt,     setup.optionLen);

        if (!Flush(fh))
            retvalue = FALSE;
        if (!Close(fh))
            retvalue = FALSE;
    }
    
    // restore breakpoint option
    setup.breakPoint = breakPoint;
    
    return retvalue;
}

static BOOL setup_line_length(CONST_STRPTR buffer, ULONG capacity, ULONG *length)
{
    ULONG i;

    for (i = 0; i < capacity; i++)
    {
        if (buffer[i] == '\0')
        {
            *length = i;
            return TRUE;
        }
    }

    return FALSE;
}

static BOOL setup_parse_parameter(STRPTR buffer, STRPTR option,
    ULONG optionSize, LONG *value)
{
    ULONG src = 0;
    ULONG dst = 0;
    LONG chars;

    while ((buffer[src] == ' ') || (buffer[src] == '\t'))
        src++;

    while (buffer[src] &&
           (buffer[src] != ' ') &&
           (buffer[src] != '\t') &&
           (buffer[src] != '\r') &&
           (buffer[src] != '\n'))
    {
        if (dst + 1 >= optionSize)
            return FALSE;

        option[dst++] = buffer[src++];
    }

    if (dst == 0)
        return FALSE;

    option[dst] = '\0';

    while ((buffer[src] == ' ') || (buffer[src] == '\t'))
        src++;

    chars = StrToLong(buffer + src, value);
    if (chars < 0)
        return FALSE;

    src += chars;

    while ((buffer[src] == ' ') || (buffer[src] == '\t'))
        src++;

    if (buffer[src] == '\r')
        src++;
    if (buffer[src] == '\n')
        src++;

    return buffer[src] == '\0';
}

BOOL setup_open(void)
{
    struct Setup candidate = setup;
    BOOL breakPoint = setup.breakPoint;
    BOOL retval = TRUE;
    char buffer[PATTERNLEN + 10];
    char option[60];
    LONG value;

    setup.breakPoint = FALSE;

    /* Old preference files predate Pattern and therefore represent no filter. */
    candidate.match = FALSE;
    candidate.pattern[0] = '\0';
    candidate.parsedpattern[0] = '\0';

    BPTR fh = Open(PREFFILE, MODE_OLDFILE);
    if (fh)
    {
        D(bug("Snoopy: File open\n"));
        while (FGets(fh, buffer, sizeof(buffer)))
        {
            ULONG len;
            int argindex = -1;

            if (!setup_line_length(buffer, sizeof(buffer), &len))
            {
                retval = FALSE;
                continue;
            }

            D(bug("Snoopy: %s read\n", buffer));

            if ((len >= 8) &&
                (strnicmp(buffer, patternOpt, 7) == 0) &&
                (buffer[7] == ' '))
            {
                ULONG start = 8;
                ULONG end = len;
                ULONG patternlen;
                ULONG i;

                if ((end == 0) || (buffer[end - 1] != '\n'))
                {
                    retval = FALSE;
                    continue;
                }

                end--;
                if ((end > start) && (buffer[end - 1] == '\r'))
                    end--;

                patternlen = end - start;
                if (patternlen >= PATTERNLEN)
                {
                    retval = FALSE;
                    continue;
                }

                for (i = 0; i < patternlen; i++)
                    candidate.pattern[i] = buffer[start + i];
                candidate.pattern[patternlen] = '\0';
            }
            else if (setup_parse_parameter(buffer, option, sizeof(option), &value))
            {
                ULONG i;

                for (i = 0; i < sizeof(opts) / sizeof(opts[0]); i++)
                {
                    if (!stricmp(option, opts[i]))
                    {
                        argindex = i;
                        break;
                    }
                }

                D(bug("Snoopy: %s | %ld\n", option, value));

                switch (argindex)
                {
                    case onlyShowFailsOpt: candidate.onlyShowFails = value; break;
                    case useDevNamesOpt:   candidate.useDevNames = value; break;
                    case showPathsOpt:     candidate.showPaths = value; break;
                    case showCliNrOpt:     candidate.showCliNr = value; break;
                    case ignoreWBOpt:      candidate.ignoreWB = value; break;
                    case breakPointOpt:    candidate.breakPoint = value; break;
                    case ChangeDirOpt:     candidate.enableChangeDir = value; break;
                    case DeleteOpt:        candidate.enableDelete = value; break;
                    case ExecuteOpt:       candidate.enableExecute = value; break;
                    case GetVarOpt:        candidate.enableGetVar = value; break;
                    case LoadSegOpt:       candidate.enableLoadSeg = value; break;
                    case LockOpt:          candidate.enableLock = value; break;
                    case MakeDirOpt:       candidate.enableMakeDir = value; break;
                    case MakeLinkOpt:      candidate.enableMakeLink = value; break;
                    case OpenOpt:          candidate.enableOpen = value; break;
                    case RenameOpt:        candidate.enableRename = value; break;
                    case RunCommandOpt:    candidate.enableRunCommand = value; break;
                    case SetVarOpt:        candidate.enableSetVar = value; break;
                    case SystemOpt:        candidate.enableSystem = value; break;
                    case FindPortOpt:      candidate.enableFindPort = value; break;
                    case FindResidentOpt:  candidate.enableFindResident = value; break;
                    case FindSemaphoreOpt: candidate.enableFindSemaphore = value; break;
                    case FindTaskOpt:      candidate.enableFindTask = value; break;
                    case LockScreenOpt:    candidate.enableLockScreen = value; break;
                    case OpenDeviceOpt:    candidate.enableOpenDevice = value; break;
                    case OpenFontOpt:      candidate.enableOpenFont = value; break;
                    case OpenLibraryOpt:   candidate.enableOpenLibrary = value; break;
                    case OpenResourceOpt:  candidate.enableOpenResource = value; break;
                    case ReadToolTypesOpt: candidate.enableReadToolTypes = value; break;
                    case nameLenOpt:       candidate.nameLen = value; break;
                    case actionLenOpt:     candidate.actionLen = value; break;
                    case targetLenOpt:     candidate.targetLen = value; break;
                    case optionLenOpt:     candidate.optionLen = value; break;
                    default: break;
                }
            }
            else if ((len != 0) &&
                     (buffer[0] != '\n') &&
                     (buffer[0] != '\r'))
            {
                retval = FALSE;
            }
        }
        Close(fh);
    }
    else
    {
        retval = FALSE;
    }

    if (retval)
    {
        LONG error = main_parsepattern(&candidate);
        if (error)
        {
            SetIoErr(error);
            retval = FALSE;
        }
    }

    if (retval)
    {
        Forbid();
        setup = candidate;
        Permit();
    }
    else
    {
        setup.breakPoint = breakPoint;
    }

    return retval;
}
