/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUIPREFS_PREFSIO_H
#define _MUIPREFS_PREFSIO_H

struct ZunePrefs
{
	char *font_normal;
	char *font_big;
	char *font_small;
};

void zune_prefs_save_as_iff(char *filename, struct ZunePrefs *prefs);

struct ZunePrefs *zune_prefs_load(char *filename);
void zune_prefs_free(struct ZunePrefs *prefs);

#endif
