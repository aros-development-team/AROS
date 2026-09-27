/*
    Copyright (C) 2006-2026, The AROS Development Team. All rights reserved.
*/

//#define DEBUG 1
#include <aros/debug.h>

#include "setup.h"

#include <proto/dos.h>
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

static BOOL setup_write_parameter(BPTR fh, int argindex, int value)
{
    static char buffer[50];
    sprintf(buffer, "%s %d\n", opts[argindex], value);
    return FPuts(fh, (STRPTR)buffer) == 0;
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

        // TODO: write pattern

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

BOOL setup_open(void)
{
    // temporary disable breakpoint option
    BOOL breakPoint = setup.breakPoint;
    setup.breakPoint = FALSE;

    BOOL retval = TRUE;
    char buffer[60];
    char option[60];
    int value;

    BPTR fh = Open(PREFFILE, MODE_OLDFILE);
    if (fh)
    {
        D(bug("Snoopy: File open\n"));
        while (FGets(fh, buffer, sizeof(buffer)))
        {
            // TODO: read pattern
            D(bug("Snoopy: %s read\n", buffer));
            if (sscanf(buffer,"%59s %d", option, &value) == 2)
            {
                D(bug("Snoopy: %s | %d\n", option, value));
                if ( ! stricmp(option, opts[onlyShowFailsOpt]))
                    setup.onlyShowFails = value;
                else if ( ! stricmp(option, opts[useDevNamesOpt]))
                    setup.useDevNames = value;
                else if ( ! stricmp(option, opts[showPathsOpt]))
                    setup.showPaths = value;
                else if ( ! stricmp(option, opts[showCliNrOpt]))
                    setup.showCliNr = value;
                else if ( ! stricmp(option, opts[ignoreWBOpt]))
                    setup.ignoreWB = value;
                else if ( ! stricmp(option, opts[breakPointOpt]))
                    breakPoint = value; // use temp variable
                else if ( ! stricmp(option, opts[ChangeDirOpt]))
                    setup.enableChangeDir = value;
                else if ( ! stricmp(option, opts[DeleteOpt]))
                    setup.enableDelete = value;
                else if ( ! stricmp(option, opts[ExecuteOpt]))
                    setup.enableExecute = value;
                else if ( ! stricmp(option, opts[GetVarOpt]))
                    setup.enableGetVar = value;
                else if ( ! stricmp(option, opts[LoadSegOpt]))
                    setup.enableLoadSeg = value;
                else if ( ! stricmp(option, opts[LockOpt]))
                    setup.enableLock = value;
                else if ( ! stricmp(option, opts[MakeDirOpt]))
                    setup.enableMakeDir = value;
                else if ( ! stricmp(option, opts[MakeLinkOpt]))
                    setup.enableMakeLink = value;
                else if ( ! stricmp(option, opts[OpenOpt]))
                    setup.enableOpen = value;
                else if ( ! stricmp(option, opts[RenameOpt]))
                    setup.enableRename = value;
                else if ( ! stricmp(option, opts[RunCommandOpt]))
                    setup.enableRunCommand = value;
                else if ( ! stricmp(option, opts[SetVarOpt]))
                    setup.enableSetVar = value;
                else if ( ! stricmp(option, opts[SystemOpt]))
                    setup.enableSystem = value;
                else if ( ! stricmp(option, opts[FindPortOpt]))
                    setup.enableFindPort = value;
                else if ( ! stricmp(option, opts[FindResidentOpt]))
                    setup.enableFindResident = value;
                else if ( ! stricmp(option, opts[FindSemaphoreOpt]))
                    setup.enableFindSemaphore = value;
                else if ( ! stricmp(option, opts[FindTaskOpt]))
                    setup.enableFindTask = value;
                else if ( ! stricmp(option, opts[LockScreenOpt]))
                    setup.enableLockScreen = value;
                else if ( ! stricmp(option, opts[OpenDeviceOpt]))
                    setup.enableOpenDevice = value;
                else if ( ! stricmp(option, opts[OpenFontOpt]))
                    setup.enableOpenFont = value;
                else if ( ! stricmp(option, opts[OpenLibraryOpt]))
                    setup.enableOpenLibrary = value;
                else if ( ! stricmp(option, opts[OpenResourceOpt]))
                    setup.enableOpenResource = value;
                else if ( ! stricmp(option, opts[ReadToolTypesOpt]))
                    setup.enableReadToolTypes = value;
                else if ( ! stricmp(option, opts[nameLenOpt]))
                    setup.nameLen = value;
                else if ( ! stricmp(option, opts[actionLenOpt]))
                    setup.actionLen = value;
                else if ( ! stricmp(option, opts[targetLenOpt]))
                    setup.targetLen = value;
                else if ( ! stricmp(option, opts[optionLenOpt]))
                    setup.optionLen = value;
            } // if
            else if ((buffer[0] != '\0') && (buffer[0] != '\n'))
            {
                retval = FALSE;
            }
        } // while
        Close(fh);
    }
    else
    {
        retval = FALSE;
    }

    // restore breakpoint option
    setup.breakPoint = breakPoint;

    return retval;
}
