/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef GLOBAL_H
#define GLOBAL_H

/*********************************************************************************************/


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

#ifndef DEVICES_INPUTEVENT_H
#include <devices/inputevent.h>
#endif

#ifndef DEVICES_RAWKEYCODES_H
#include <devices/rawkeycodes.h>
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

/*********************************************************************************************/

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

#include "vars.h"

#undef CATCOMP_STRINGS
#undef CATCOMP_NUMBERS

#define CATCOMP_NUMBERS

#include "strings.h"

/*********************************************************************************************/

#define USE_SHARED_COOLIMAGES 1

/*********************************************************************************************/

#define CONFIGNAME_ENV	    	"ENV:Sys/locale.prefs"
#define CONFIGNAME_ENVARC   	"ENVARC:Sys/locale.prefs"

#define PAGECMD_INIT         	1
#define PAGECMD_LAYOUT       	2
#define PAGECMD_GETMINWIDTH  	3
#define PAGECMD_GETMINHEIGHT 	4
#define PAGECMD_SETDOMLEFT   	5
#define PAGECMD_SETDOMTOP    	6
#define PAGECMD_SETDOMWIDTH  	7
#define PAGECMD_SETDOMHEIGHT 	8
#define PAGECMD_MAKEGADGETS  	9
#define PAGECMD_ADDGADGETS   	10
#define PAGECMD_REMGADGETS   	11
#define PAGECMD_REFRESH     	12
#define PAGECMD_HANDLEINPUT  	13
#define PAGECMD_PREFS_CHANGING  14
#define PAGECMD_PREFS_CHANGED   15
#define PAGECMD_CLEANUP      	16

#define BORDER_X    	    	4
#define BORDER_Y    	    	4
#define TABBORDER_X 	    	8
#define TABBORDER_Y 	    	8
#define SPACE_X     	    	4
#define SPACE_Y     	    	4

#define BUTTON_EXTRAWIDTH   	16
#define BUTTON_EXTRAHEIGHT  	6
#define IMBUTTON_EXTRAWIDTH  	4
#define IMBUTTON_EXTRAHEIGHT 	4

/*********************************************************************************************/

struct ListviewEntry
{
    struct Node node;
    UBYTE   	name[30];
    UBYTE   	realname[30];
};

struct CountryEntry
{
    struct ListviewEntry lve;
    Object  	    	 *dto;
    struct BitMap   	 *flagbm;
    WORD    	    	 flagw;
    WORD    	    	 flagh;
    
};

struct LanguageEntry
{
    struct ListviewEntry lve;
};

/*********************************************************************************************/

/* main.c */

void Cleanup(CONST_STRPTR msg);
void TellGUI(LONG cmd);

/* misc.c */

void InitMenus(void);
void MakeMenus(void);
void KillMenus(void);
void SetMenuFlags(void);
struct Node *FindListNode(struct List *list, WORD which);
void SortInNode(struct List *list, struct Node *node);
STRPTR GetFile(CONST_STRPTR title, CONST_STRPTR dir, BOOL savemode);
void ScrollListview(struct Gadget *gad, WORD delta);

/* page_language.c */

LONG page_language_handler(LONG cmd, IPTR param);

/* page_country.c */

LONG page_country_handler(LONG cmd, IPTR param);

/* page_timezone.c */

LONG page_timezone_handler(LONG cmd, IPTR param);

/* locale.c */

void InitLocale(STRPTR catname, ULONG version);
void CleanupLocale(void);
CONST_STRPTR MSG(ULONG id);

/* prefs.c */

void InitPrefs(STRPTR filename, BOOL use, BOOL save);
void CleanupPrefs(void);
BOOL LoadCountry(STRPTR name, struct CountryPrefs *country);
BOOL LoadPrefs(STRPTR filename);
BOOL SavePrefs(STRPTR filename);
BOOL DefaultPrefs(void);
void RestorePrefs(void);

/*********************************************************************************************/
/*********************************************************************************************/

#endif /* GLOBAL_H */
