// blah

#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <libraries/mui.h>
#include <libraries/desktop.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/desktop.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/icon.h>
#include <proto/layers.h>
#include <proto/alib.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include <workbench/workbench.h>

#include <stdio.h>
#include <stdlib.h>

struct Library *DesktopBase;
struct Library *MUIMasterBase;

int main(void)
{
	Object *app, *win, *iconCon, *strip, *vert, *horiz;
	struct MUI_CustomClass *iconContainerClass;
	BOOL running=TRUE;
	ULONG signals=0;
	struct Screen *screen;
	struct NewMenu menuDat[]=
	{
		{NM_TITLE, "AROS", 0,0,0,(APTR)1},
		{NM_ITEM, "Quit", "Q",0,0,(APTR)2},
		{NM_END, NULL, 0,0,0,(APTR)0}
	};
	struct TagItem icTags[5];

	DesktopBase=OpenLibrary("desktop.library", 0);
	MUIMasterBase=OpenLibrary("muimaster.library", 0);

	screen=LockPubScreen(NULL);

	horiz=PropObject,
		MUIA_Prop_Horiz, TRUE,
		MUIA_Prop_Entries, 0,
		MUIA_Prop_UseWinBorder, MUIV_Prop_UseWinBorder_Bottom,
		End;
	vert=PropObject,
		MUIA_Prop_Horiz, FALSE,
		MUIA_Prop_UseWinBorder, MUIV_Prop_UseWinBorder_Right,
		End;

	icTags[0].ti_Tag=MUIA_FillArea;
	icTags[0].ti_Data=FALSE;
//	icTags[1].ti_Tag=ICOA_Directory;
//	icTags[1].ti_Data="C:";
//	icTags[2].ti_Tag=ICA_VertScroller;
//	icTags[2].ti_Data=vert;
//	icTags[3].ti_Tag=ICA_HorizScroller;
//	icTags[3].ti_Data=horiz;
	icTags[1].ti_Tag=TAG_END;
	icTags[1].ti_Data=0;

	app=ApplicationObject,
		SubWindow, win=WindowObject,
			MUIA_Window_Backdrop, TRUE,
			MUIA_Window_Borderless, TRUE,
			MUIA_Window_CloseGadget, FALSE,
			MUIA_Window_DepthGadget, FALSE,
			MUIA_Window_SizeGadget, FALSE,
			MUIA_Window_DragBar, FALSE,
			MUIA_Window_LeftEdge, 0,
			MUIA_Window_TopEdge, screen->BarHeight+1,
			MUIA_Window_Width, screen->Width,
			MUIA_Window_Height, screen->Height-screen->BarHeight-1,
			MUIA_Window_Menustrip, strip=MUI_MakeObject(MUIO_MenustripNM, menuDat, 0),
//			MUIA_Window_UseBottomBorderScroller, TRUE,
//			MUIA_Window_UseRightBorderScroller, TRUE,
			MUIA_Window_EraseArea, FALSE,
			WindowContents, iconCon=CreateDesktopObjectA(CDO_Desktop, icTags),
		End,
	End;

	if(app)
	{
		DoMethod(win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

		// these are here temporarily..
		DoMethod(vert, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, iconCon, 3, MUIM_Set, ICA_ScrollToVert, MUIV_TriggerValue);
		DoMethod(horiz, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, iconCon, 3, MUIM_Set, ICA_ScrollToHoriz, MUIV_TriggerValue);

		SetAttrs(win, MUIA_Window_Open, TRUE, TAG_DONE);

		while(running)
		{
			switch(DoMethod(app, MUIM_Application_Input, &signals))
			{
				case MUIV_Application_ReturnID_Quit:
					running=FALSE;
					break;
				case 2:
					running=FALSE;
					break;
			}

			if(running && signals)
				Wait(signals);
		}

		SetAttrs(win, MUIA_Window_Open, FALSE, TAG_DONE);

		DisposeObject(app);
	}


	CloseLibrary(MUIMasterBase);
	CloseLibrary(DesktopBase);

	return 0;
}





