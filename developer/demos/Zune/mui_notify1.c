/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <exec/types.h>

//#include <libraries/mui.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#ifdef __AROS__
#include <libraries/mui.h>
#endif
#include <stdio.h>

struct Library       *MUIMasterBase;

#ifndef __AROS__

#include <mui.h>
#undef SysBase

/* On AmigaOS we build a fake library base, because it's not compiled as sharedlibrary yet */
#include "muimaster_intern.h"

int openmuimaster(void)
{
    static struct MUIMasterBase_intern MUIMasterBase_instance;
    MUIMasterBase = (struct Library*)&MUIMasterBase_instance;

    MUIMasterBase_instance.sysbase = *((struct ExecBase **)4);
    MUIMasterBase_instance.dosbase = OpenLibrary("dos.library",37);
    MUIMasterBase_instance.utilitybase = OpenLibrary("utility.library",37);
    MUIMasterBase_instance.aslbase = OpenLibrary("asl.library",37);
    MUIMasterBase_instance.gfxbase = OpenLibrary("graphics.library",37);
    MUIMasterBase_instance.layersbase = OpenLibrary("layers.library",37);
    MUIMasterBase_instance.intuibase = OpenLibrary("intuition.library",37);
    MUIMasterBase_instance.cxbase = OpenLibrary("commodities.library",37);
    MUIMasterBase_instance.keymapbase = OpenLibrary("keymap.library",37);
    __zune_prefs_init(&__zprefs);

    return 1;
}

void closemuimaster(void)
{
}

#else

int openmuimaster(void)
{
    if ((MUIMasterBase = OpenLibrary("muimaster.library", 0))) return 1;
    return 0;
}

void closemuimaster(void)
{
    if (MUIMasterBase) CloseLibrary(MUIMasterBase);
}

#endif

int main (void)
{
    Object *obj;
    ULONG val = 0;

    if (!openmuimaster()) return 20;

    obj = MUI_NewObjectA(MUIC_Notify, NULL);
    printf("new Notify object = %p\n", obj);
    if (obj != NULL)
    {
        set(obj, MUIA_UserData, 21);
        get(obj, MUIA_UserData, &val);
        printf("object UserData = %ld (should be 21)\n", (long)val);

        DoMethod(obj, MUIM_Notify, MUIA_UserData, MUIV_EveryTime, MUIV_Notify_Self,
                 3, MUIM_Set, MUIA_ObjectID, 19);
	printf("MUIM_Notify OK\n");

        set(obj, MUIA_ObjectID, 12);
        get(obj, MUIA_ObjectID, &val);
        printf("id = %ld (should be 12)\n", (long)val);

        set(obj, MUIA_UserData, 22);
        get(obj, MUIA_ObjectID, &val);
        printf("id = %ld (should be 19) after notification\n", (long)val);

	set(obj, MUIA_ObjectID, 14);
	set(obj, MUIA_UserData, 22);
	get(obj, MUIA_ObjectID, &val);
        printf("id = %ld (should be 14) after notification\n", (long)val);

        MUI_DisposeObject(obj);
    }

    closemuimaster();

    return 0;
}
