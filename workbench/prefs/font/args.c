/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <dos/dos.h>
#include <prefs/font.h>

#include <proto/dos.h>

#include <string.h>

#include "prefs.h"
#include "args.h"

#define DEBUG 1
#include <aros/debug.h>

#define ARG_FROM 0
#define ARG_EDIT 1
#define ARG_USE 2
#define ARG_SAVE 3
#define NUM_ARGS 4

IPTR argArray[NUM_ARGS];
struct RDArgs *readArgs;

// Grab any shell arguments given by the user
struct RDArgs *getArguments(void)
{
    memset(argArray, NULL, sizeof(argArray));
    
    return ReadArgs("FROM,EDIT/S,USE/S,SAVE/S", argArray, NULL);
}

// Deal with shell arguments. If failure, should we quit or not? If started
// from shell, do we assume the user wants to run Font Preferences on a non
// (GUI) interactive basis? Request for comments! (petah)
UBYTE processArguments(void)
{
    if(!(readArgs = getArguments()))
        PrintFault(IoErr(), NULL);

 /* If FROM is set, then also check for the USE and SAVE keywords - but only then. There isn't
    any point in just replacing the same settings with the very same values? */

    if(argArray[ARG_FROM] != NULL)
    {
        if (!(FP_LoadFrom((CONST_STRPTR) argArray[ARG_FROM]))) return APP_FAIL;

     /* If USE or SAVE is set, write the FROM file to ENV: and/or ENVARC: and then quit. Is this
        what the "Classic" Font Preferences does? Look it up! (As a side note, if FILE is not
        found, the old settings will be overwritten with default values. Should we avoid this and
        implement some error checking in writeIFF() ? What if FROM is not set? Should we still 
        react for USE and SAVE (which we currently don't)? Request for comments to author! */

        if (argArray[ARG_USE] || argArray[ARG_SAVE])
        {
            if (argArray[ARG_USE] && !FP_Use()) return APP_FAIL;
            if (argArray[ARG_SAVE] && !FP_Save()) return APP_FAIL;
            
            /* Don't launch the rest of the program, just exit */
            return APP_STOP;
        }
    }
    else
    {
        if (!FP_Load()) return APP_FAIL;
    }
    
    // FIXME: What is "EDIT" supposed to do? Look it up!
    if (argArray[ARG_EDIT])
    {
        kprintf("EDIT keyword set!\n");
    }
    
    return APP_RUN;
}
