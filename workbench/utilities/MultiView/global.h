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

#ifndef DEVICES_KEYMAP_H
#include <devices/keymap.h>
#endif

#ifndef DEVICES_INPUTEVENT_H
#include <devices/inputevent.h>
#endif

#ifdef __AROS__
#ifndef DEVICES_RAWKEYCODES_H
#include <devices/rawkeycodes.h>
#endif
#endif

#ifndef LIBRARIES_GADTOOLS_H
#include <libraries/gadtools.h>
#endif

#ifndef LIBRARIES_ASL_H
#include <libraries/asl.h>
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

#ifndef DATATYPES_PICTURECLASS_H
#include <datatypes/pictureclass.h>
#endif

#ifndef DATATYPES_TEXTCLASS_H
#include <datatypes/textclass.h>
#endif

#ifndef WORKBENCH_STARTUP_H
#include <workbench/startup.h>
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

#ifndef PROTO_KEYMAP_H
#include <proto/keymap.h>
#endif

#ifndef PROTO_LOCALE_H
#include <proto/locale.h>
#endif

#ifndef PROTO_LAYERS_H
#include <proto/layers.h>
#endif

#ifndef PROTO_DATATYPES_H
#include <proto/datatypes.h>
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

#ifndef PROTO_DISKFONT_H
#include <proto/diskfont.h>
#endif

/*********************************************************************************************/

enum
{
    GAD_UPARROW,
    GAD_DOWNARROW,
    GAD_LEFTARROW,
    GAD_RIGHTARROW,
    GAD_VERTSCROLL,
    GAD_HORIZSCROLL,
    NUM_GADGETS
};

enum
{
    IMG_UPARROW,
    IMG_DOWNARROW,
    IMG_LEFTARROW,
    IMG_RIGHTARROW,
    IMG_SIZE,
    NUM_IMAGES
};

/*********************************************************************************************/

#include "vars.h"

#undef CATCOMP_STRINGS
#undef CATCOMP_NUMBERS

#define CATCOMP_NUMBERS

#include "strings.h"

/*********************************************************************************************/

/* main.c */

void OutputMessage(CONST_STRPTR msg);
void Cleanup(CONST_STRPTR msg);
void AddDTOToWin(void);

/* misc.c */

void InitMenus(struct NewMenu *newm);
struct Menu * MakeMenus(struct NewMenu *newm);
void KillMenus(void);
void SetMenuFlags(void);

STRPTR GetFileName(ULONG msgtextid);
void About(void);

ULONG DoTrigger(ULONG what);
ULONG DoWriteMethod(STRPTR name, ULONG mode);
ULONG DoLayout(ULONG initial);
ULONG DoScaleMethod(ULONG xsize, ULONG ysize, BOOL aspect);
void DoZoom(WORD zoomer);

/*********************************************************************************************/

/* locale.c */

void InitLocale(STRPTR catname, ULONG version);
void CleanupLocale(void);
CONST_STRPTR MSG(ULONG id);

/*********************************************************************************************/
/*********************************************************************************************/

#endif /* GLOBAL_H */
