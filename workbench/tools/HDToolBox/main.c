/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#define AROS_ALMOST_COMPATIBLE

#include <stdio.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>

#include <intuition/intuition.h>
#include <libraries/gadtools.h>

#include "error.h"
#include "gadgets.h"
#include "mainwin.h"
#include "workmain.h"
#include "workpcp.h"


struct IntuitionBase *IntuitionBase=NULL;
struct GfxBase *GfxBase=NULL;
struct Library *GadToolsBase=NULL;

APTR visual=NULL;
struct Screen *scr=NULL;
struct Window *mainwin=NULL;
struct Gadget *mainglist=NULL;
struct Gadget *pcpglist=NULL;
struct Gadget *apglist=NULL;

extern struct creategadget maingadgets[],pcpgadgets[], apgadgets[];
extern struct TagItem sctdtags[];
extern struct List hd_list, partition_list;

ULONG initEnv(char *device) {

	NEWLIST(&hd_list);
	NEWLIST(&partition_list);
	IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
	if (!IntuitionBase)
		return ERR_INTUI;
	GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
	if (!GfxBase)
		return ERR_GFX;
	GadToolsBase = OpenLibrary("gadtools.library", 0);
	if (!GadToolsBase)
		return ERR_GADTOOLS;
	scr = LockPubScreen(NULL);
	if (!scr)
		return ERR_SCREEN;
	visual = GetVisualInfoA(scr, NULL);
	if (!visual)
		return ERR_VISUAL;
	mainglist=createGadgets(maingadgets,ID_MAIN_FIRST_GADGET, ID_MAIN_LAST_GADGET,visual);
	if (!mainglist)
		return ERR_GADGETS;
	pcpglist=createGadgets(pcpgadgets,ID_PCP_FIRST_GADGET, ID_PCP_LAST_GADGET,visual);
	if (!pcpglist)
		return ERR_GADGETS;
	apglist=createGadgets(apgadgets, ID_AP_FIRST_GADGET, ID_AP_LAST_GADGET, visual);
	if (!apglist)
		return ERR_GADGETS;
	mainwin = OpenWindowTags
		(
			NULL,
			WA_PubScreen, scr,
			WA_Left, 0,
			WA_Top, 0,
			WA_Width, 640,
			WA_Height, 250,
			WA_Title, "HDToolBox",
			WA_IDCMP,
				BUTTONIDCMP | CHECKBOXIDCMP | SLIDERIDCMP | SCROLLERIDCMP | ARROWIDCMP |
				IDCMP_GADGETUP | IDCMP_REFRESHWINDOW,
			WA_Gadgets, mainglist,
			WA_DragBar, TRUE,
			WA_DepthGadget, TRUE,
			TAG_DONE
		);
	if (!mainwin)
		return ERR_WINDOW;
	if (device)
		findHDs(mainwin, device, 2);
	else
	{
		findHDs(mainwin, "ide.device", 4);
		findHDs(mainwin, "scsi.device", 6);
	}
	return ERR_NONE;
}

void uninitEnv() {
	if (mainwin)
		CloseWindow(mainwin);
	if (pcpglist)
		freeGadgets(pcpglist);
	if (mainglist)
		freeGadgets(mainglist);
	if (visual)
		FreeVisualInfo(visual);
	if (scr)
		UnlockPubScreen(NULL, scr);
	if (GadToolsBase)
		CloseLibrary(GadToolsBase);
	if (GfxBase)
		CloseLibrary((struct Library *)GfxBase);
	if (IntuitionBase)
		CloseLibrary((struct Library *)IntuitionBase);
}

void waitMessage(struct Window *win) {
struct IntuiMessage *msg;
struct HDUnitNode *current_hd=0;
struct PartitionNode *current_partition=0;
BOOL running=TRUE, changed;

	while (running)
	{
		WaitPort(win->UserPort);
		while ((msg=GT_GetIMsg(win->UserPort)))
		{
			switch (msg->Class)
			{
			case IDCMP_REFRESHWINDOW :
				GT_BeginRefresh(win);
				GT_EndRefresh(win, TRUE);
				break;
			case IDCMP_GADGETUP :
				switch (((struct Gadget *) msg->IAddress)->GadgetID)
				{
				case ID_MAIN_HARDDISK :
					current_hd = getHDUnit(mainwin, msg->Code);
					break;
				case ID_MAIN_SAVE_CHANGES :
					saveChanges(mainwin, current_hd);
					break;
				case ID_MAIN_EXIT :
					if (reallyExit(&hd_list))
					{
						freeHDList(&hd_list);
						running=FALSE;
					}
					break;
				case ID_MAIN_PARTITION_DRIVE :
					changed = FALSE;
					setPCPGadgetAttrs(0);
					findPartitions(0, current_hd);
					RemoveGList(win, mainglist, ~0);
					clearGadgets((struct ExtGadget *)mainglist, win, -1);
					AddGList(win, pcpglist, 0, ~0, NULL);
					RefreshGList(win->FirstGadget, win, NULL, -1);
					break;
				case ID_PCP_PARTITION :
					current_partition = getPartition(mainwin, current_hd, msg->Code);
					break;
				case ID_PCP_ADD_PARTITION :
					if ((current_partition = addPartition(mainwin, current_hd)))
						changed = TRUE;
					break;
				case ID_PCP_DELETE_PARTITION :
					deletePartition(mainwin, current_partition);
					changed = TRUE;
					current_partition = 0;
					break;
				case ID_PCP_EDIT_PARTITION :
					RemoveGList(win, pcpglist, ~0);
					clearGadgets((struct ExtGadget *)pcpglist, win, -1);
					AddGList(win, apglist, 0, ~0, NULL);
					RefreshGList(win->FirstGadget, win, NULL, -1);
					break;
				case ID_PCP_STARTCYL :
					if (
							changeStartCyl
								(
									mainwin,
									current_hd,
									current_partition,
									(
										(struct StringInfo *)
											(
												(struct Gadget *) msg->IAddress
											)->SpecialInfo
									)->LongInt
								)
						)
						changed = TRUE;
					break;
				case ID_PCP_ENDCYL :
					if (
							changeEndCyl
								(
									mainwin,
									current_hd,
									current_partition,
									(
										(struct StringInfo *)
											(
												(struct Gadget *) msg->IAddress
											)->SpecialInfo
									)->LongInt
								)
						)
						changed = TRUE;
					break;
				case ID_PCP_TOTALCYL :
					if (
							changeTotalCyl
								(
									mainwin,
									current_hd,
									current_partition,
									(
										(struct StringInfo *)
											(
												(struct Gadget *) msg->IAddress
											)->SpecialInfo
									)->LongInt
								)
						)
						changed = TRUE;
					break;
				case ID_PCP_TYPELV :
					if (changeType(mainwin, current_partition, msg->Code))
						changed = TRUE;
					break;
				case ID_PCP_TYPEINTEGER :
					if (
							changeType
							(
								mainwin, current_partition,
								(
									(struct StringInfo *)
										(
											(struct Gadget *) msg->IAddress
										)->SpecialInfo
								)->LongInt
							)
						)
						changed = TRUE;
					break;
				case ID_PCP_OK :
					pcp_Ok(&hd_list);
					if (changed)
					{
						sctdtags[0].ti_Data = FALSE;
						SetGadgetAttrsA
							(
								maingadgets[ID_MAIN_SAVE_CHANGES-ID_MAIN_FIRST_GADGET].gadget,
								0, 0, sctdtags
							);
						current_hd->partition_changed = TRUE;
					}
					RemoveGList(win, pcpglist, ~0);
					clearGadgets((struct ExtGadget *)pcpglist, win, -1);
					AddGList(win, mainglist, 0, ~0, NULL);
					RefreshGList(win->FirstGadget, win, NULL, -1);
					break;
				case ID_PCP_CANCEL :
					pcp_Cancel();
					RemoveGList(win, pcpglist, ~0);
					clearGadgets((struct ExtGadget *)pcpglist, win, -1);
					AddGList(win, mainglist, 0, ~0, NULL);
					RefreshGList(win->FirstGadget, win, NULL, -1);
					break;
				}
			}
			GT_ReplyIMsg(msg);
		}
	}
}

int main(int argc, char **argv) {
ULONG error;
char *device;

	device = argc > 1 ? argv[1] : 0;
	if ((error=initEnv(device))==ERR_NONE)
	{
		waitMessage(mainwin);
	}
	else
		printf("Error %ld\n", error);
	uninitEnv();
	return 0;
}

