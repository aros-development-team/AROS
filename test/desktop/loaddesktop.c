/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

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
	struct DesktopOperationItem *doi;
	LONG i=0, j=0;
	struct NewMenu *menuDat;
	struct TagItem icTags[6];
	ULONG inputResult;

	DesktopBase=OpenLibrary("desktop.library", 0);
	MUIMasterBase=OpenLibrary("muimaster.library", 0);

	doi=GetMenuItemList(DOC_ICONOP);
	if(doi)
	{
		while(doi[i].doi_Code!=0 && doi[i].doi_Name!=NULL)
			i++;
	}

	menuDat=(struct NewMenu*)AllocVec(sizeof(struct NewMenu)*(i+3), MEMF_ANY);
	menuDat[0].nm_Type=NM_TITLE;
	menuDat[0].nm_Label="AROS";
	menuDat[0].nm_CommKey=0;
	menuDat[0].nm_Flags=0;
	menuDat[0].nm_MutualExclude=0;
	menuDat[0].nm_UserData=0;
	menuDat[1].nm_Type=NM_ITEM;
	menuDat[1].nm_Label="Quit";
	menuDat[1].nm_CommKey="Q";
	menuDat[1].nm_Flags=0;
	menuDat[1].nm_MutualExclude=0;
	menuDat[1].nm_UserData=2;
	j=2;

	if(i>0)
	{
		menuDat[2].nm_Type=NM_TITLE;
		menuDat[2].nm_Label="Icon";
		menuDat[2].nm_CommKey=0;
		menuDat[2].nm_Flags=0;
		menuDat[2].nm_MutualExclude=0;
		menuDat[2].nm_UserData=DOC_ICONOP;
		j=3;

		i=0;
		while(doi[i].doi_Code!=0 && doi[i].doi_Name!=NULL)
		{
			menuDat[j].nm_Type=NM_ITEM;
			menuDat[j].nm_Label=doi[i].doi_Name;
			menuDat[j].nm_CommKey=0;
			menuDat[j].nm_Flags=0;
			menuDat[j].nm_MutualExclude=0;
			menuDat[j].nm_UserData=doi[i].doi_Code;
			i++;
			j++;
		}
	}

	menuDat[j].nm_Type=NM_END;
	menuDat[j].nm_Label=NULL;
	menuDat[j].nm_CommKey=0;
	menuDat[j].nm_Flags=0;
	menuDat[j].nm_MutualExclude=0;
	menuDat[j].nm_UserData=0;

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

	icTags[0].ti_Tag=MUIA_InnerLeft;
	icTags[0].ti_Data=0;
	icTags[1].ti_Tag=MUIA_InnerTop;
	icTags[1].ti_Data=0;
	icTags[2].ti_Tag=MUIA_InnerBottom;
	icTags[2].ti_Data=0;
	icTags[3].ti_Tag=MUIA_InnerRight;
	icTags[3].ti_Data=0;
	icTags[4].ti_Tag=MUIA_FillArea;
	icTags[4].ti_Data=FALSE;
//	icTags[1].ti_Tag=ICOA_Directory;
//	icTags[1].ti_Data="C:";
//	icTags[2].ti_Tag=ICA_VertScroller;
//	icTags[2].ti_Data=vert;
//	icTags[3].ti_Tag=ICA_HorizScroller;
//	icTags[3].ti_Data=horiz;
	icTags[5].ti_Tag=TAG_END;
	icTags[5].ti_Data=0;

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
			inputResult=DoMethod(app, MUIM_Application_Input, &signals);
			switch(inputResult)
			{
				case MUIV_Application_ReturnID_Quit:
					running=FALSE;
					break;
				case 2:
					running=FALSE;
					break;
				default:
				{
					// a menuitem was selected...
					struct MinList *subjects=NULL;
					Object *member, *ostate;

					if(inputResult & DOC_ICONOP)
					{
						GetAttr(ICA_SelectedIcons, iconCon, &subjects);

						ostate=subjects->mlh_Head;
						while(member=NextObject(&ostate))
						{
							struct TagItem args[2];

							args[0].ti_Tag=OPA_Target;
							args[0].ti_Data=member;
							args[1].ti_Tag=TAG_END;
							args[1].ti_Data=0;

							DoDesktopOperation(inputResult, args);
						}
					}
				}
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





