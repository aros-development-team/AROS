/*
    Copyright © 2006, The AROS Development Team. All rights reserved.
    $Id$
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
    FILE *fh = fopen(PREFFILE, "w");
    if (!fh) return FALSE;

    fprintf(fh, "%s %d\n", opts[onlyShowFailsOpt], setup.onlyShowFails);
    fprintf(fh, "%s %d\n", opts[useDevNamesOpt],   setup.useDevNames);
    fprintf(fh, "%s %d\n", opts[showPathsOpt],     setup.showPaths);
    fprintf(fh, "%s %d\n", opts[showCliNrOpt],     setup.showCliNr);
    fprintf(fh, "%s %d\n", opts[ignoreWBOpt],      setup.ignoreWB);

    fprintf(fh, "%s %d\n", opts[ChangeDirOpt],     setup.enableChangeDir);
    fprintf(fh, "%s %d\n", opts[DeleteOpt],        setup.enableDelete);
    fprintf(fh, "%s %d\n", opts[ExecuteOpt],       setup.enableExecute);
    fprintf(fh, "%s %d\n", opts[GetVarOpt],        setup.enableGetVar);
    fprintf(fh, "%s %d\n", opts[LoadSegOpt],       setup.enableLoadSeg);
    fprintf(fh, "%s %d\n", opts[LockOpt],          setup.enableLock);
    fprintf(fh, "%s %d\n", opts[MakeDirOpt],       setup.enableMakeDir);
    fprintf(fh, "%s %d\n", opts[MakeLinkOpt],      setup.enableMakeLink);
    fprintf(fh, "%s %d\n", opts[OpenOpt],          setup.enableOpen);
    fprintf(fh, "%s %d\n", opts[RenameOpt],        setup.enableRename);
    fprintf(fh, "%s %d\n", opts[RunCommandOpt],    setup.enableRunCommand);
    fprintf(fh, "%s %d\n", opts[SetVarOpt],        setup.enableSetVar);
    fprintf(fh, "%s %d\n", opts[SystemOpt],        setup.enableSystem);

    fprintf(fh, "%s %d\n", opts[FindPortOpt],      setup.enableFindPort);
    fprintf(fh, "%s %d\n", opts[FindResidentOpt],  setup.enableFindResident);
    fprintf(fh, "%s %d\n", opts[FindSemaphoreOpt], setup.enableFindSemaphore);
    fprintf(fh, "%s %d\n", opts[FindTaskOpt],      setup.enableFindTask);
    fprintf(fh, "%s %d\n", opts[LockScreenOpt],    setup.enableLockScreen);
    fprintf(fh, "%s %d\n", opts[OpenDeviceOpt],    setup.enableOpenDevice);
    fprintf(fh, "%s %d\n", opts[OpenFontOpt],      setup.enableOpenFont);
    fprintf(fh, "%s %d\n", opts[OpenLibraryOpt],   setup.enableOpenLibrary);
    fprintf(fh, "%s %d\n", opts[OpenResourceOpt],  setup.enableOpenResource);
    fprintf(fh, "%s %d\n", opts[ReadToolTypesOpt], setup.enableReadToolTypes);

    fprintf(fh, "%s %d\n", opts[nameLenOpt],       setup.nameLen);
    fprintf(fh, "%s %d\n", opts[actionLenOpt],     setup.actionLen);
    fprintf(fh, "%s %d\n", opts[targetLenOpt],     setup.targetLen);
    fprintf(fh, "%s %d\n", opts[optionLenOpt],     setup.optionLen);

    fclose(fh);
    return TRUE;
}

BOOL setup_open(void)
{
    BOOL retval = TRUE;
    char buffer[60];
    char option[60];
    int value;
    FILE *fh = fopen(PREFFILE, "r");
    if (!fh) return FALSE;
    D(bug("Snoopy: File open\n"));
    while (fgets(buffer, sizeof(buffer), fh))
    {
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
    fclose(fh);
    return retval;
}

