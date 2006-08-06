/*
    Copyright © 2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef SETUP_H
#define SETUP_H

#include <exec/types.h>

void setup_init(void);

struct Setup
{
    BOOL onlyShowFails;
    BOOL showCliNr;
    BOOL useDevNames;
    BOOL showPaths;
    BOOL ignoreWB;

    BOOL enableChangeDir;
    BOOL enableDelete;
    BOOL enableExecute;
    BOOL enableGetVar;
    BOOL enableLoadSeg;
    BOOL enableLock;
    BOOL enableMakeDir;
    BOOL enableMakeLink;
    BOOL enableOpen;
    BOOL enableRename;
    BOOL enableRunCommand;
    BOOL enableSetVar;
    BOOL enableSystem;

    BOOL enableFindPort;
    BOOL enableFindResident;
    BOOL enableFindSemaphore;
    BOOL enableFindTask;
    BOOL enableLockScreen;
    BOOL enableOpenDevice;
    BOOL enableOpenFont;
    BOOL enableOpenLibrary;
    BOOL enableOpenResource;
    BOOL enableReadToolTypes;
}; 

extern struct Setup setup;
extern struct Setup oldsetup;

#endif

