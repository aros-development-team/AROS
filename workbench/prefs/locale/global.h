/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
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

#ifndef DATATYPES_DATATYPES_H
#include <datatypes/datatypes.h>
#endif

#ifndef DATATYPES_DATATYPESCLASS_H
#include <datatypes/datatypesclass.h>
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

/*********************************************************************************************/

#include "vars.h"

#undef CATCOMP_STRINGS
#undef CATCOMP_NUMBERS

#define CATCOMP_NUMBERS

#include "localeprefs_strings.h"

/*********************************************************************************************/

#define PAGECMD_INIT         1
#define PAGECMD_LAYOUT       2
#define PAGECMD_GETMINWIDTH  3
#define PAGECMD_GETMINHEIGHT 4
#define PAGECMD_SETDOMLEFT   5
#define PAGECMD_SETDOMTOP    6
#define PAGECMD_SETDOMWIDTH  7
#define PAGECMD_SETDOMHEIGHT 8
#define PAGECMD_MAKEGADGETS  9
#define PAGECMD_ADDGADGETS   10
#define PAGECMD_REMGADGETS   11
#define PAGECMD_HANDLEINPUT  12
#define PAGECMD_CLEANUP      13

#define BORDER_X    4
#define BORDER_Y    4
#define TABBORDER_X 4
#define TABBORDER_Y 4
#define SPACE_X     4
#define SPACE_Y     4

/*********************************************************************************************/

/* main.c */

void Cleanup(STRPTR msg);

/* misc.c */

void InitMenus(void);
void MakeMenus(void);
void KillMenus(void);
void SetMenuFlags(void);

STRPTR GetFile(void);

/* page_language.c */

LONG page_language_handler(LONG cmd, LONG param);

/* page_country.c */

LONG page_country_handler(LONG cmd, LONG param);

/* page_timezone.c */

LONG page_timezone_handler(LONG cmd, LONG param);

/* locale.c */

void InitLocale(STRPTR catname, ULONG version);
void CleanupLocale(void);
STRPTR MSG(ULONG id);

/*********************************************************************************************/
/*********************************************************************************************/

#endif /* GLOBAL_H */
