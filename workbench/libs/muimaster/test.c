#include <stdio.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>

/* the following should go in a single include file which then only
** constits of the public constants and members. Actually this is easiey
*/

#include "mui.h"
#include "classes/notify.h" 

/* muimaster.library is not yet a library */
#include "muimaster_intern.h"

struct Library *MUIMasterBase;
struct MUIMasterBase_intern MUIMasterBase_instance;

__asm __saveds void hook_function(register __a1 int *pval)
{
    printf("get notification of the userdata: %ld\n",*pval);
}

void main(void)
{
    struct Hook hook;
    Object *notify;

    MUIMasterBase = (struct Library*)&MUIMasterBase_instance;

    MUIMasterBase_instance.sysbase = *((struct ExecBase **)4);
    MUIMasterBase_instance.utilitybase = OpenLibrary("utility.library",37);

    printf("Creating object\n");
    notify = MUI_NewObjectA(MUIC_Notify, NULL);

    if (notify)
    {
	LONG ud;
	printf("Object created at 0x%lx. Now setting MUIA_UserData to 10\n",notify);
	set(notify, MUIA_UserData, 10);
	get(notify, MUIA_UserData, &ud);
	printf("UserData is now %ld\n",ud);

	printf("Create a notify for the MUIA_UserData tag\n");
	hook.h_Entry = (HOOKFUNC)hook_function;
	DoMethod(notify, MUIM_Notify, MUIA_UserData, MUIV_EveryTime, notify, 3, MUIM_CallHook, &hook, MUIV_TriggerValue);
	printf("Setting User Data to 20\n");
	set(notify, MUIA_UserData, 20);
	printf("Done. Now disposing object\n");

	MUI_DisposeObject(notify);
    }
}
