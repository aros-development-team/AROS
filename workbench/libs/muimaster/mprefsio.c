/* Zune -- a free Magic User Interface implementation
 * Copyright (C) 1999 David Le Corfec
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <exec/types.h>

#ifdef _AROS
#include <proto/exec.h>
#include <proto/graphics.h>
#endif

#include <zunepriv.h>
#include <prefs.h>
#include <stdio.h>
#include <uproperties.h>
#include <Configdata.h>
#include <Dataspace.h>
#include <sys/stat.h>

int
__zune_prefs_read(struct ZunePrefs *prefs, STRPTR path)
{
#warning FIXME: prefs_read
#if 0
    FILE *f;
    Object *configdata;

g_print("__zune_prefs_read %s\n", path);
    f = fopen(path, "r");
    if (!f)
    {
	return FALSE;
    }
g_print("__zune_prefs_read %s found\n", path);

    configdata = MUI_NewObject(MUIC_Configdata,
			       MUIA_Dataspace_Comments, prefs->comments);

    DoMethod(configdata, MUIM_Dataspace_ReadASCII, (IPTR)f);
    fclose(f);
    g_print("zune_prefs_read begin\n");
    DoMethod(configdata, MUIM_Configdata_FindPens, (IPTR)prefs);
    DoMethod(configdata, MUIM_Configdata_FindWindows, (IPTR)prefs);
    DoMethod(configdata, MUIM_Configdata_FindGroups, (IPTR)prefs);
    DoMethod(configdata, MUIM_Configdata_FindButtons, (IPTR)prefs);
    DoMethod(configdata, MUIM_Configdata_FindCycles, (IPTR)prefs);
    DoMethod(configdata, MUIM_Configdata_FindSliders, (IPTR)prefs);
    DoMethod(configdata, MUIM_Configdata_FindScrollbars, (IPTR)prefs);
    DoMethod(configdata, MUIM_Configdata_FindListviews, (IPTR)prefs);
    DoMethod(configdata, MUIM_Configdata_FindStrings, (IPTR)prefs);
    DoMethod(configdata, MUIM_Configdata_FindNavigation, (IPTR)prefs);
    DoMethod(configdata, MUIM_Configdata_FindSpecial, (IPTR)prefs);
    g_print("zune_prefs_read end\n");

    MUI_DisposeObject(configdata);
#endif

    return TRUE;
}

#if 0
static void
zune_mkdirs (STRPTR path)
{
    struct stat stat_buf;
    gchar *dir;
   
    dir = g_dirname(path);
    if (stat (dir, &stat_buf) != 0)
    {
	zune_mkdirs(dir);
    }
    mkdir(path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
}
#endif


int
__zune_prefs_write(struct ZunePrefs *prefs, STRPTR path)
{
#warning FIXME: prefs_write
#if 0
    FILE *f;
    Object *configdata;
    gchar *dir;
    struct stat stat_buf;

    dir = g_dirname(path);
    if (stat (dir, &stat_buf) != 0)
    {
	zune_mkdirs(dir);
    }

    f = fopen(path, "w");
    if (!f)
    {
	g_warning("zune_prefs_write: can't write file %s\n", path);
	return FALSE;
    }

    configdata = MUI_NewObject(MUIC_Configdata,
			       MUIA_Dataspace_Comments, prefs->comments);

    DoMethod(configdata, MUIM_Configdata_AddPens, (IPTR)prefs);
    DoMethod(configdata, MUIM_Configdata_AddWindows, (IPTR)prefs);
    DoMethod(configdata, MUIM_Configdata_AddGroups, (IPTR)prefs);
    DoMethod(configdata, MUIM_Configdata_AddButtons, (IPTR)prefs);
    DoMethod(configdata, MUIM_Configdata_AddCycles, (IPTR)prefs);
    DoMethod(configdata, MUIM_Configdata_AddSliders, (IPTR)prefs);
    DoMethod(configdata, MUIM_Configdata_AddScrollbars, (IPTR)prefs);
    DoMethod(configdata, MUIM_Configdata_AddListviews, (IPTR)prefs);
    DoMethod(configdata, MUIM_Configdata_AddStrings, (IPTR)prefs);
    DoMethod(configdata, MUIM_Configdata_AddNavigation, (IPTR)prefs);
    DoMethod(configdata, MUIM_Configdata_AddSpecial, (IPTR)prefs);

    DoMethod(configdata, MUIM_Dataspace_WriteASCII, (IPTR)f);
    fclose(f);
    MUI_DisposeObject(configdata);
#endif
    return TRUE;
}
