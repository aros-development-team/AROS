#include <aros/asmcall.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <libraries/mui.h>

#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include <stdio.h>

#include "tooliconclass.h"
#include "wbwindowclass.h"
#include "toolicondefaultpresclass.h"
#include "wbobjectclass.h"
#include "wbapplicationclass.h"
#include "wbwindowdefaultpresclass.h"
#include "wbiconclass.h"
#include "wbdiskiconclass.h"
#include "wbdrawericonclass.h"
#include "wbtooliconclass.h"
#include "wbprojecticonclass.h"
#include "wbtrashcaniconclass.h"
#include "icondefpres.h"

struct Library *MUIMasterBase;
struct UtilityBase *UtilityBase;
struct Library *LayersBase;

struct MUI_CustomClass *WBWindowClass;
struct MUI_CustomClass *WBObjectClass;
struct MUI_CustomClass *WBApplicationClass;
struct MUI_CustomClass *WBWindowDefaultPresentationClass;
struct MUI_CustomClass *WBIconClass;
struct MUI_CustomClass *WBDiskIconClass;
struct MUI_CustomClass *WBDrawerIconClass;
struct MUI_CustomClass *WBToolIconClass;
struct MUI_CustomClass *WBProjectIconClass;
struct MUI_CustomClass *WBTrashcanIconClass;
struct MUI_CustomClass *IconDefaultPresentationClass;
struct MUI_CustomClass *WBToolIconDefaultPresentationClass;

AROS_UFH3(IPTR,wbWindowDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFH3(IPTR,wbApplicationDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFH3(IPTR,wbObjectClassDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFH3(IPTR,wbWindowDefaultPresDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFH3(IPTR,wbIconClassDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFH3(IPTR,wbDiskIconClassDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFH3(IPTR,wbDrawerIconClassDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFH3(IPTR,wbToolIconClassDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFH3(IPTR,wbProjectIconClassDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFH3(IPTR,wbTrashcanIconClassDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFH3(IPTR, iconClassDefPresDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

AROS_UFH3(IPTR,wbToolIconClassDefPresDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1));

int main(void)
{
	Object *win1, *realwin1, *icon1, *app, *strip;
	BOOL running=TRUE;
	ULONG signals=0, breakSig=0;

	UtilityBase=OpenLibrary("utility.library", 0);
	MUIMasterBase=OpenLibrary("muimaster.library", 0);
	LayersBase=OpenLibrary("layers.library", 0);

	WBObjectClass=MUI_CreateCustomClass(NULL, MUIC_Notify, NULL, sizeof(struct WBObjectClassData), wbObjectClassDispatcher);

	WBWindowClass=MUI_CreateCustomClass(NULL, NULL, WBObjectClass, sizeof(struct WBWindowClassData), wbWindowDispatcher);
	WBApplicationClass=MUI_CreateCustomClass(NULL, NULL, WBObjectClass, sizeof(struct WBApplicationClassData), wbApplicationDispatcher);
	WBWindowDefaultPresentationClass=MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct WBWinDefPresClassData), wbWindowDefaultPresDispatcher);

	WBIconClass=MUI_CreateCustomClass(NULL, NULL, WBObjectClass, sizeof(struct WBIconClassData), wbIconClassDispatcher);

	WBDiskIconClass=MUI_CreateCustomClass(NULL, NULL, WBIconClass, sizeof(struct WBDiskIconClassData), wbDiskIconClassDispatcher);
	WBDrawerIconClass=MUI_CreateCustomClass(NULL, NULL, WBIconClass, sizeof(struct WBDrawerIconClassData), wbDrawerIconClassDispatcher);
	WBToolIconClass=MUI_CreateCustomClass(NULL, NULL, WBIconClass, sizeof(struct WBToolIconClassData), wbToolIconClassDispatcher);
	WBProjectIconClass=MUI_CreateCustomClass(NULL, NULL, WBIconClass, sizeof(struct WBProjectIconClassData), wbProjectIconClassDispatcher);
	WBTrashcanIconClass=MUI_CreateCustomClass(NULL, NULL, WBIconClass, sizeof(struct WBTrashcanIconClassData), wbTrashcanIconClassDispatcher);

	IconDefaultPresentationClass=MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct IconDefPresClassData), iconClassDefPresDispatcher);
	WBToolIconDefaultPresentationClass=MUI_CreateCustomClass(NULL, NULL, IconDefaultPresentationClass, sizeof(struct WBToolIconDefPresClassData), wbToolIconClassDefPresDispatcher);

	app=(Object*)NewObject(WBApplicationClass->mcc_Class, NULL, End;
	if(app)
	{
		win1=(Object*)NewObject(WBWindowClass->mcc_Class, NULL,
			WBA_Window_Directory, "Workbench:",
			End;

		DoMethod(app, OM_ADDMEMBER, win1, TAG_END);
		SetAttrs(win1, WBA_Window_Open, TRUE, TAG_DONE);

		while(running)
		{
			switch(DoMethod(app, WBM_Application_Input, &signals, breakSig))
			{
				case 2:
					running=FALSE;
					break;
				case MUIV_Application_ReturnID_Quit:
					running=FALSE;
					break;
			}

			if(running && signals)
				breakSig=Wait(signals);

		}

		SetAttrs(win1, WBA_Window_Open, FALSE, TAG_DONE);
	}
	else
		printf("App failed\n");

	MUI_DeleteCustomClass(WBToolIconDefaultPresentationClass);
	MUI_DeleteCustomClass(IconDefaultPresentationClass);

	MUI_DeleteCustomClass(WBWindowDefaultPresentationClass);

	MUI_DeleteCustomClass(WBDiskIconClass);
	MUI_DeleteCustomClass(WBDrawerIconClass);
	MUI_DeleteCustomClass(WBProjectIconClass);
	MUI_DeleteCustomClass(WBTrashcanIconClass);
	MUI_DeleteCustomClass(WBToolIconClass);

	MUI_DeleteCustomClass(WBIconClass);

	MUI_DeleteCustomClass(WBApplicationClass);
	MUI_DeleteCustomClass(WBWindowClass);

	MUI_DeleteCustomClass(WBObjectClass);

	CloseLibrary(LayersBase);
	CloseLibrary(MUIMasterBase);
	CloseLibrary(UtilityBase);

	return 0;
}

