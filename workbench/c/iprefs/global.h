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

#ifndef DOS_NOTIFY_H
#include <dos/notify.h>
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

#ifndef PROTO_ASL_H
#include <proto/asl.h>
#endif

#ifndef PROTO_DISKFONT_H
#include <proto/diskfont.h>
#endif

/*********************************************************************************************/

#define IPREFS_SEM_NAME "� IPrefs �"

struct IPrefsSem
{
    struct SignalSemaphore sem;
    UBYTE   	    	   semname[12];
};

/*********************************************************************************************/

#include "vars.h"

/*********************************************************************************************/

/* main.c */

void Cleanup(STRPTR msg);

/* patches.c */

void InstallPatches(void);

/*********************************************************************************************/

#endif /* GLOBAL_H */
