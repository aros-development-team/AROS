/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: prefs.h 24051 2007-09-30 12:00:00 olivieradam, dariusb $

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include <aros/macros.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "global.h"

/*********************************************************************************************/

#define ARRAY_TO_LONG(x) ( ((x)[0] << 24UL) + ((x)[1] << 16UL) + ((x)[2] << 8UL) + ((x)[3]) )
#define ARRAY_TO_WORD(x) ( ((x)[0] << 8UL) + ((x)[1]) )

#define LONG_TO_ARRAY(x,y) (y)[0] = (UBYTE)(ULONG)((x) >> 24UL); \
    	    	    	   (y)[1] = (UBYTE)(ULONG)((x) >> 16UL); \
			   (y)[2] = (UBYTE)(ULONG)((x) >>  8UL); \
			   (y)[3] = (UBYTE)(ULONG)((x));

#define WORD_TO_ARRAY(x,y) (y)[0] = (UBYTE)(ULONG)((x) >>  8UL); \
			   (y)[1] = (UBYTE)(ULONG)((x));

/*********************************************************************************************/

struct FilePrefHeader
{
    UBYTE ph_Version;
    UBYTE ph_Type;
    UBYTE ph_Flags[4];
};

struct FileInputPrefs
{
    char    ip_Keymap[16];
    UBYTE   ip_PointerTicks[2];
    UBYTE   ip_DoubleClick_secs[4];
    UBYTE   ip_DoubleClick_micro[4];
    UBYTE   ip_KeyRptDelay_secs[4];
    UBYTE   ip_KeyRptDelay_micro[4];
    UBYTE   ip_KeyRptSpeed_secs[4];
    UBYTE   ip_KeyRptSpeed_micro[4];
    UBYTE   ip_MouseAccel[2];
};

struct nameexp
{
    STRPTR shortname;
    STRPTR longname;
    STRPTR flag;
};


struct ListviewEntry
{
    struct Node node;
    UBYTE                           layoutname[30];
    UBYTE                           realname[30];
    UBYTE                           flagname[30];
    UBYTE                           displayname[128];
};

struct KeymapEntry
{
    struct ListviewEntry lve;
};

/*********************************************************************************************/

void ScanDirectory(STRPTR pattern, struct List *list, LONG entrysize);
BOOL LoadPrefs(BPTR fh);
BOOL SavePrefs(BPTR fh);
void update_inputdev(void);
void try_setting_mousespeed(void);
void try_setting_test_keymap(void);
void kbd_cleanup(void);
void RestorePrefs(void);
BOOL DefaultPrefs(void);
void CopyPrefs(struct InputPrefs *s, struct InputPrefs *d);





