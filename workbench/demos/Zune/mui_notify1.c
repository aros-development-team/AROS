#include <exec/types.h>
#ifdef _AROS
#define AMIGA
#endif

#ifdef AMIGA
#include <libraries/mui.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/alib.h>
#include <stdio.h>
#else
#include <zune/zune.h>
#endif

#ifdef AMIGA
struct IntuitionBase *IntuitionBase;
struct Library       *MUIMasterBase;
#endif

int main (void)
{
    Object *obj;
    ULONG val;

#ifdef AMIGA
    MUIMasterBase = OpenLibrary("muimaster.library", 0);
    if (MUIMasterBase == NULL) return 20;
    IntuitionBase = OpenLibrary("intuition.library", 36);
#endif

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

#ifdef AMIGA
    CloseLibrary(IntuitionBase);
    CloseLibrary(MUIMasterBase);
#endif

    return 0;
}
