/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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

#ifndef DEVICES_TIMER_H
#include <devices/timer.h>
#endif

#ifndef LIBRARIES_GADTOOLS_H
#include <libraries/gadtools.h>
#endif

#ifndef UTILITY_UTILITY_H
#include <utility/utility.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#ifndef UTILITY_DATE_H
#include <utility/date.h>
#endif

#ifndef LIBRARIES_LOCALE_H
#include <libraries/locale.h>
#endif

#ifndef LIBRARIES_MUI_H
#include <libraries/mui.h>
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

#ifndef PROTO_TIMER_H
#include <proto/timer.h>
#endif

#ifndef PROTO_BATTCLOCK_H
#include <proto/battclock.h>
#endif

#ifndef PROTO_MUIMASTER_H
#include <proto/muimaster.h>
#endif

/*********************************************************************************************/

#include "vars.h"

#undef CATCOMP_STRINGS
#undef CATCOMP_NUMBERS

#define CATCOMP_NUMBERS

#include "strings.h"

/*********************************************************************************************/

/* main.c */

void Cleanup(STRPTR msg);

/* misc.c */

void InitMenus(void);
void my_sprintf(UBYTE *buffer, UBYTE *format, ...);

/* locale.c */

void InitLocale(STRPTR catname, ULONG version);
void CleanupLocale(void);
STRPTR MSG(ULONG id);

/* prefs.c */

void InitPrefs(BOOL use, BOOL save);
BOOL UsePrefs(void);
BOOL SavePrefs(void);
void RestorePrefs(void);


/*********************************************************************************************/

#endif /* GLOBAL_H */
