/*
    Copyright © 2003, The AROS Development Team. 
    All rights reserved.

    $Id$
*/

#ifndef _ZUNE_MUISUPPORT_H
#define _ZUNE_MUISUPPORT_H

#include <string.h>

#include <exec/memory.h>

#include <libraries/asl.h>
#include <libraries/mui.h>
#include <prefs/prefhdr.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/iffparse.h>

#ifdef __AROS__
#include <proto/muimaster.h>
#endif

Object *MakeLabel(STRPTR str);
LONG xget(Object * obj, ULONG attr);

#define getstring(obj) (char *) xget(obj, MUIA_String_Contents)

#define SimpleText(text) TextObject, MUIA_Text_Contents, (IPTR) text, End


struct Library *MUIMasterBase;


#ifndef __AROS__

/* On AmigaOS we build a fake library base, because it's not compiled as sharedlibrary yet */
#include "muimaster_intern.h"

int open_muimaster(void)
{
    static struct MUIMasterBase_intern MUIMasterBase_instance;
    MUIMasterBase = (struct Library*)&MUIMasterBase_instance;

    MUIMasterBase_instance.sysbase      = *((struct ExecBase **)4);
    MUIMasterBase_instance.dosbase      = (void *)OpenLibrary("dos.library",        37);
    MUIMasterBase_instance.utilitybase  = (void *)OpenLibrary("utility.library",    37);
    MUIMasterBase_instance.aslbase      =         OpenLibrary("asl.library",        37);
    MUIMasterBase_instance.gfxbase      = (void *)OpenLibrary("graphics.library",   37);
    MUIMasterBase_instance.layersbase   =         OpenLibrary("layers.library",     37);
    MUIMasterBase_instance.intuibase    = (void *)OpenLibrary("intuition.library",  37);
    MUIMasterBase_instance.cxbase       =         OpenLibrary("commodities.library",37);
    MUIMasterBase_instance.keymapbase   =         OpenLibrary("keymap.library",     37);
    MUIMasterBase_instance.gadtoolsbase =         OpenLibrary("gadtools.library",   37);
    MUIMasterBase_instance.iffparsebase =         OpenLibrary("iffparse.library",   37);
    MUIMasterBase_instance.diskfontbase =         OpenLibrary("diskfont.library",   37);
    __zune_prefs_init(&__zprefs);
    InitSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
    return 1;
}

void close_muimaster(void)
{
}

#else

int open_muimaster(void)
{
    if ((MUIMasterBase = OpenLibrary("muimaster.library", 0))) return 1;
    return 0;
}

void close_muimaster(void)
{
    if (MUIMasterBase) CloseLibrary(MUIMasterBase);
}

#endif


/****************************************************************
 Open needed libraries
*****************************************************************/
int open_libs(void)
{
    if (open_muimaster())
    {
        return 1;
    }

    return 0;
}

/****************************************************************
 Close opened libraries
*****************************************************************/
void close_libs(void)
{
    close_muimaster();
}


/****************************************************************
 Create a simple label
*****************************************************************/
Object *MakeLabel(STRPTR str)
{
    return (MUI_MakeObject(MUIO_Label, str, 0));
}


/****************************************************************
 Easy getting an attributes value
*****************************************************************/
LONG xget(Object * obj, ULONG attr)
{
    LONG x = 0;
    get(obj, attr, &x);
    return x;
}


#endif /* _ZUNE_MUISUPPORT_H */
