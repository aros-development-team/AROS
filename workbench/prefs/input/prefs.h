#ifndef _PREFS_H_
#define _PREFS_H_

/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include <aros/macros.h>

#include <dos/dos.h>
#include <prefs/prefhdr.h>
#include <prefs/input.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*********************************************************************************************/

#define ARRAY_TO_LONG(x) ( ((x)[0] << 24UL) + ((x)[1] << 16UL) + ((x)[2] << 8UL) + ((x)[3]) )
#define ARRAY_TO_WORD(x) ( ((x)[0] << 8UL) + ((x)[1]) )

#define LONG_TO_ARRAY(x,y)  (y)[0] = (UBYTE)(ULONG)((x) >> 24UL); \
                            (y)[1] = (UBYTE)(ULONG)((x) >> 16UL); \
                            (y)[2] = (UBYTE)(ULONG)((x) >>  8UL); \
                            (y)[3] = (UBYTE)(ULONG)((x));

#define WORD_TO_ARRAY(x,y)  (y)[0] = (UBYTE)(ULONG)((x) >>  8UL); \
                            (y)[1] = (UBYTE)(ULONG)((x));

/* Name of default keymap, the same as on AmigaOS, for compatibility */
#define DEFAULT_KEYMAP "usa"

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
    UBYTE   ip_ClassicKeyboard[4];
    char    ip_KeymapName[64];
    UBYTE   ip_SwitchMouseButtons[4];
};

struct FileKMSPrefs
{
    UBYTE kms_Enabled;
    UBYTE kms_Reserved;
    UBYTE kms_SwitchQual[2];
    UBYTE kms_SwitchCode[2];
    char  kms_AltKeymap[64];
};

struct nameexp
{
    STRPTR shortname;
    STRPTR longname;
    STRPTR flag;
};

struct typeexp
{
    STRPTR shortname;
    STRPTR longname;
};

#define KEYMAP_NAME_LEN 30
#define KEYMAP_FLAG_LEN 128

struct ListviewEntry
{
    struct Node node;
    UBYTE                           layoutname[KEYMAP_NAME_LEN];
    UBYTE                           realname[30];
    UBYTE                           displayflag[KEYMAP_FLAG_LEN];
};

struct KeymapEntry
{
    struct ListviewEntry lve;
};

/*********************************************************************************************/

void Prefs_ScanDirectory(STRPTR pattern, struct List *list, LONG entrysize);
BOOL Prefs_ImportFH(BPTR fh);
BOOL Prefs_ExportFH(BPTR fh);
BOOL Prefs_Default(void);
BOOL Prefs_HandleArgs(STRPTR from, BOOL use, BOOL save);

/*********************************************************************************************/

extern struct timerequest *InputIO;
extern APTR                mempool;
extern struct List         keymap_list;
extern struct InputPrefs   inputprefs;
extern struct KMSPrefs     kmsprefs;

#endif
