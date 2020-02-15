/*
    Copyright © 2018-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#define INTUITION_NO_INLINE_STDARG

#include <aros/debug.h>

#include <libraries/mui.h>

#include <dos/dos.h>
#include <exec/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <clib/alib_protos.h>

#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <gadgets/colorwheel.h>

#include <libraries/asl.h>
#include <libraries/expansionbase.h>

#include <devices/trackdisk.h>
#include <devices/scsidisk.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/partition.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include <mui/TextEditor_mcc.h>

#include "ia_locale.h"
#include "ia_install.h"
#include "ia_bootloader.h"

void create_environment_variable(CONST_STRPTR envarchiveDisk,
    CONST_STRPTR name, CONST_STRPTR value)
{
    BPTR env_variable_fh = BNULL;
    TEXT env_variable_path[100];
    int eaDLen;

    if ((envarchiveDisk == NULL) || (name == NULL) || (value == NULL))
        return;

    eaDLen = strlen(envarchiveDisk);
    sprintf(env_variable_path, "%s", envarchiveDisk);
    if (env_variable_path[eaDLen - 1] != ':')
    {
        env_variable_path[eaDLen] = ':';
        env_variable_path[eaDLen + 1] = 0;
    }

    AddPart(env_variable_path, "Prefs/Env-Archive/", 100);
    AddPart(env_variable_path, name, 100);

    D(bug
        ("[InstallAROS] create_environment_variable: Setting Var '%s' to '%s'\n",
            env_variable_path, value));

    if ((env_variable_fh = Open(env_variable_path, MODE_NEWFILE)) != BNULL)
    {
        FPuts(env_variable_fh, value);
        Close(env_variable_fh);
    }
}

BOOL read_environment_variable(CONST_STRPTR envarchiveDisk,
    CONST_STRPTR name, STRPTR buffer, ULONG size)
{
    BPTR env_variable_fh = BNULL;
    TEXT env_variable_path[100];
    int eaDLen;

    if ((envarchiveDisk == NULL) || (name == NULL) || (buffer == NULL))
        return FALSE;

    eaDLen = strlen(envarchiveDisk);
    sprintf(env_variable_path, "%s", envarchiveDisk);
    if (env_variable_path[eaDLen - 1] != ':')
    {
        env_variable_path[eaDLen] = ':';
        env_variable_path[eaDLen + 1] = 0;
    }

    AddPart(env_variable_path, "Prefs/Env-Archive/", 100);
    AddPart(env_variable_path, name, 100);

    D(bug("[InstallAROS] read_environment_variable: Getting Var '%s'\n",
        env_variable_path));

    if ((env_variable_fh = Open(env_variable_path, MODE_OLDFILE)) != BNULL)
    {
        FGets(env_variable_fh, buffer, size);
        Close(env_variable_fh);
        return TRUE;
    }
    return FALSE;
}
