/*
    Copyright � 2006-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef SETUP_H
#define SETUP_H

#include <exec/types.h>

#include "main.h"

#define PREFFILE "envarc:snoopy.prefs"

void setup_init(void);
void setup_reset(void);
BOOL setup_open(void);
BOOL setup_save(void);

struct Setup
{
    BOOL onlyShowFails;
    BOOL showCliNr;
    BOOL useDevNames;
    BOOL showPaths;
    BOOL ignoreWB;
    BOOL breakPoint; // stop AROS e.g. with asm("int3") on i386.

    BOOL match;  // TRUE if pattern could be successfully parsed.
    STRPTR pattern;
    TEXT parsedpattern[PARSEDPATTERNLEN];

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

    // min. field len for output
    WORD nameLen;
    WORD actionLen;
    WORD targetLen;
    WORD optionLen;
}; 

extern struct Setup setup;
extern struct Setup oldsetup;

#endif

