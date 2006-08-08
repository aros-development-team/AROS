/*
    Copyright © 2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "setup.h"

struct Setup setup;
struct Setup oldsetup;

void setup_init(void)
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

    setup.nameLen = 15;
    setup.actionLen = 15;
    setup.targetLen = 40;
    setup.optionLen = 15;

    oldsetup = setup;
}

