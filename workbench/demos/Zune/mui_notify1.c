#include <exec/types.h>

//#include <libraries/mui.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#ifdef _AROS
#include <proto/muimaster.h>
#endif
#include <stdio.h>

#include <mui.h>

struct Library       *MUIMasterBase;

#ifndef _AROS

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
    ULONG val;

    if (!openmuimaster()) return 20;

    obj = MUI_NewObjectA(MUIC_Notify, NULL);
    printf("new Notify object = %p\n", obj);
    if (obj != NULL)
    {
        set(obj, MUIA_UserData, 21);
        get(obj, MUIA_UserData, &val);
        printf("object UserData = %ld (should be 21)\n", val);

        DoMethod(obj, MUIM_Notify, MUIA_UserData, MUIV_EveryTime, MUIV_Notify_Self,
                 3, MUIM_Set, MUIA_ObjectID, 19);
	printf("MUIM_Notify OK\n");

        set(obj, MUIA_ObjectID, 12);
        get(obj, MUIA_ObjectID, &val);
        printf("id = %ld (should be 12)\n", val);

        set(obj, MUIA_UserData, 22);
        get(obj, MUIA_ObjectID, &val);
        printf("id = %ld (should be 19) after notification\n", val);

        MUI_DisposeObject(obj);
    }

    closemuimaster();

    return 0;
}
