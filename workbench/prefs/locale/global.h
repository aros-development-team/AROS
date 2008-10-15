/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#ifndef GLOBAL_H
#define GLOBAL_H

#include <prefs/locale.h>

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif

#ifndef EXEC_MEMORY_H
#include <exec/memory.h>
#endif

#ifndef INTUITION_INTUITION_H
#include <intuition/intuition.h>
#endif

#ifndef INTUITION_SCREENS_H
#include <intuition/screens.h>
#endif

#ifndef INTUITION_IMAGECLASS_H
#include <intuition/imageclass.h>
#endif

#ifndef INTUITION_GADGETCLASS_H
#include <intuition/gadgetclass.h>
#endif

#ifndef INTUITION_CLASSUSR_H
#include <intuition/classusr.h>
#endif

#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif

#ifndef INTUITION_ICCLASS_H
#include <intuition/icclass.h>
#endif

#ifndef GRAPHICS_GFX_H
#include <graphics/gfx.h>
#endif

#ifndef GRAPHICS_GFXBASE_H
#include <graphics/gfxbase.h>
#endif

#ifndef GRAPHICS_CLIP_H
#include <graphics/clip.h>
#endif

#ifndef GRAPHICS_LAYERS_H
#include <graphics/layers.h>
#endif

#ifndef LIBRARIES_GADTOOLS_H
#include <libraries/gadtools.h>
#endif

#ifndef LIBRARIES_ASL_H
#include <libraries/asl.h>
#endif

#ifndef LIBRARIES_IFFPARSE_H
#include <libraries/iffparse.h>
#endif

#ifndef UTILITY_UTILITY_H
#include <utility/utility.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#ifndef LIBRARIES_LOCALE_H
#include <libraries/locale.h>
#endif

#ifndef CYBERGRAPHX_CYBERGRAPHICS_H
#include <cybergraphx/cybergraphics.h>
#endif

#ifndef DATATYPES_DATATYPES_H
#include <datatypes/datatypes.h>
#endif

#ifndef DATATYPES_DATATYPESCLASS_H
#include <datatypes/datatypesclass.h>
#endif

#define DT_V44_SUPPORT

#ifndef DATATYPES_PICTURECLASS_H
#include <datatypes/pictureclass.h>
#endif

#ifndef PREFS_LOCALE_H
#include <prefs/locale.h>
#endif

#ifndef PREFS_PREFHDR_H
#include <prefs/prefhdr.h>
#endif

#ifndef PROTO_EXEC_H
#include <proto/exec.h>
#endif

#ifndef PROTO_DOS_H
#include <proto/dos.h>
#endif

#ifndef PROTO_INTUITION_H
#include <proto/intuition.h>
#endif

#ifndef PROTO_GRAPHICS_H
#include <proto/graphics.h>
#endif

#ifndef PROTO_UTILITY_H
#include <proto/utility.h>
#endif

#ifndef PROTO_LOCALE_H
#include <proto/locale.h>
#endif

#ifndef PROTO_ALIB_H
#include <proto/alib.h>
#endif

#ifndef PROTO_GADTOOLS_H
#include <proto/gadtools.h>
#endif

#ifndef PROTO_ASL_H
#include <proto/asl.h>
#endif

#ifndef PROTO_IFFPARSE_H
#include <proto/iffparse.h>
#endif

#ifndef PROTO_CYBERGRAPHICS_H
#include <proto/cybergraphics.h>
#endif

#ifndef PROTO_DATATYPES_H
#include <proto/datatypes.h>
#endif

/*********************************************************************************************/

#define CONFIGNAME_ENV	    	"ENV:Sys/locale.prefs"
#define CONFIGNAME_ENVARC   	"ENVARC:Sys/locale.prefs"

#define MA_PrefsObject MUIA_UserData

#define LP_TAGBASE 0xfece0000 /* ok ?? */

enum
{
    MA_CountryName = LP_TAGBASE,
    MA_Preferred,
    MA_TimeOffset,
};

/*********************************************************************************************/

/* main.c */

VOID ShowMsg      (char *msg);

/* locale.c */

VOID InitLocale   (VOID);
VOID CleanupLocale(VOID);
CONST_STRPTR MSG  (ULONG id);

/* prefs.c */

ULONG InitPrefs   (STRPTR filename, BOOL use, BOOL save);
VOID  CleanupPrefs(void);
BOOL  LoadPrefs   (STRPTR filename);
BOOL  LoadPrefsFH (BPTR fh);
BOOL  SavePrefs   (STRPTR filename);
BOOL  SavePrefsFH (BPTR fh);
BOOL  SaveEnv     ();
BOOL  DefaultPrefs(void);
VOID  RestorePrefs(void);
VOID  BackupPrefs (void);
VOID  CopyPrefs   (struct LocalePrefs *s, struct LocalePrefs *d);

/*********************************************************************************************/


struct ListviewEntry
{
    struct Node node;
    char   	name[30];
    char   	realname[30];
};

struct CountryEntry
{
    struct ListviewEntry lve;
    struct BitMap       *flagbm;
    WORD                 flagw;
    WORD                 flagh;
    Object              *pic;
    Object              *list_pic;
    
};

struct LanguageEntry
{
    struct ListviewEntry lve;
    BOOL                 preferred;
};

struct List                  country_list, language_list, pref_language_list;
struct LocalePrefs           localeprefs;
APTR   mempool;

#endif /* GLOBAL_H */
