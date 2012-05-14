/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <aros/macros.h>

#define DEBUG 1
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/iffparse.h>
#include <proto/dos.h>

#include "prefs.h"
#include "misc.h"

/*********************************************************************************************/

#define PREFS_PATH_ENVARC "ENVARC:Iconbar.prefs"
#define PREFS_PATH_ENV    "ENV:Iconbar.prefs"

/*********************************************************************************************/

struct BIBPrefs bibprefs;

/*********************************************************************************************/

static BOOL Prefs_Load(STRPTR from)
{
    BOOL retval = FALSE;

    BPTR fh = Open(from, MODE_OLDFILE);
    if (fh)
    {
        retval = Prefs_ImportFH(fh);
        Close(fh);
    }

    return retval;
}

/*********************************************************************************************/

BOOL Prefs_ImportFH(BPTR fh)
{
    BOOL retval = TRUE;

    LONG dock = -1;
    LONG program = 0;
    LONG line = 0;
    LONG slen;
    char buffer[500];

    while ((FGets(fh, buffer, sizeof(buffer))) != NULL)
    {
        slen = strlen(buffer);
        if (buffer[slen - 1] == '\n')
            buffer[slen - 1] = 0;

        if (line == 0)
        {
            if (strncmp(buffer, "BOING_PREFS", 11 ) != 0)
            {
                D(bug("[IconBarPrefs] %s: is not BOING BAR configuration file!\n"));
                retval = FALSE;
                break;
            }
        }
        else
        {
            if (buffer[0] == ';' )
            {
                dock++;
                strcpy(bibprefs.docks[dock].name, &buffer[1]);
                program = 0;
                D(bug("[IconBarPrefs] %s buffer\n", &buffer[1]));
            }
            else
            {
                strcpy(bibprefs.docks[dock].programs[program], buffer);
                D(bug("[IconBarPrefs] data set %d / %d   - %s\n", dock, program, bibprefs.docks[dock].programs[program]));
                program++;
            }
        }
        line++;
    }

#if 0
    LONG i,j;
    for (i = 0; (i < BIB_MAX_DOCKS) && (bibprefs.docks[i].name[0] != '\0'); i++)
    {
        bug("Dock %s\n", bibprefs.docks[i].name);

        for (j = 0; (j < BIB_MAX_PROGRAMS) && (bibprefs.docks[i].programs[j][0] != 0); j++)
        {
            bug("   App %s\n", bibprefs.docks[i].programs[j]);
        }
    }
#endif

    return retval;
}

/*********************************************************************************************/

BOOL Prefs_ExportFH(BPTR fh)
{
    BOOL retval = TRUE;
    LONG i, j;

    FPuts(fh, "BOING_PREFS\n");

    for (i = 0; (i < BIB_MAX_DOCKS) && (bibprefs.docks[i].name[0] != '\0'); i++)
    {
        FPutC(fh, ';');
        FPuts(fh, bibprefs.docks[i].name);
        FPutC(fh, '\n');

        for (j = 0; (j < BIB_MAX_PROGRAMS) && (bibprefs.docks[i].programs[j][0] != 0); j++)
        {
            FPuts(fh, bibprefs.docks[i].programs[j]);
            FPutC(fh, '\n');
        }
    }

    return retval;
}

/*********************************************************************************************/

BOOL Prefs_HandleArgs(STRPTR from, BOOL use, BOOL save)
{
    BPTR fh;

    if (from)
    {
        if (!Prefs_Load(from))
        {
            ShowMessage("Can't read from input file");
            return FALSE;
        }
    }
    else
    {
        if (!Prefs_Load(PREFS_PATH_ENV))
        {
            if (!Prefs_Load(PREFS_PATH_ENVARC))
            {
                ShowMessage
                (
                    "Can't read from file " PREFS_PATH_ENVARC
                    ".\nUsing default values."
                );
                Prefs_Default();
            }
        }
    }

    if (use || save)
    {
        fh = Open(PREFS_PATH_ENV, MODE_NEWFILE);
        if (fh)
        {
            Prefs_ExportFH(fh);
            Close(fh);
        }
        else
        {
            ShowMessage("Cant' open " PREFS_PATH_ENV " for writing.");
        }
    }
    if (save)
    {
        fh = Open(PREFS_PATH_ENVARC, MODE_NEWFILE);
        if (fh)
        {
            Prefs_ExportFH(fh);
            Close(fh);
        }
        else
        {
            ShowMessage("Cant' open " PREFS_PATH_ENVARC " for writing.");
        }
    }
    return TRUE;
}

/*********************************************************************************************/

BOOL Prefs_Default(VOID)
{
    D(bug("[IconBarPrefs] Prefs_Default\n"));
    
    memset(&bibprefs, 0, sizeof bibprefs);

    return TRUE;
}
