/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/


#include <stdio.h>
#include <stdlib.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>

#include <intuition/intuition.h>
#include <libraries/gadtools.h>

#include "details.h"
#include "error.h"
#include "gadgets.h"
#include "hdtoolbox_support.h"
#include "mainwin.h"
#include "partitions.h"
#include "partitiontables.h"
#include "partitiontypes.h"
#include "platform.h"
#include "ptclass.h"

#define DEBUG 1
#include "debug.h"

struct IntuitionBase *IntuitionBase=NULL;
struct GfxBase *GfxBase=NULL;
struct Library *GadToolsBase=NULL;
struct PartitionBase *PartitionBase=NULL;

APTR visual=NULL;
struct Screen *scr=NULL;
struct Window *mainwin=NULL;
struct Gadget *mainglist=NULL;
struct Gadget *pcpglist=NULL;
struct Gadget *dglist=NULL;

extern struct creategadget maingadgets[],pcpgadgets[], detailsgadgets[];
extern struct TagItem sctdtags[];
extern struct List hd_list, pt_list;

ULONG initEnv(char *device) {

	NEWLIST(&hd_list);
	NEWLIST(&pt_list);
	IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
	if (!IntuitionBase)
		return ERR_INTUI;
	GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
	if (!GfxBase)
		return ERR_GFX;
	GadToolsBase = OpenLibrary("gadtools.library", 0);
	if (!GadToolsBase)
		return ERR_GADTOOLS;
	PartitionBase = (struct PartitionBase *)OpenLibrary("partition.library", 1);
	if (!PartitionBase)
		return ERR_PARTITION;
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
	dglist=createGadgets(detailsgadgets, ID_DET_FIRST_GADGET, ID_DET_LAST_GADGET, visual);
	if (!dglist)
		return ERR_GADGETS;
	allocPTGadget(scr, pcpglist);
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
				BUTTONIDCMP |
				CHECKBOXIDCMP |
				SLIDERIDCMP |
				SCROLLERIDCMP |
				ARROWIDCMP |
				IDCMP_GADGETUP |
				IDCMP_REFRESHWINDOW,
			WA_Gadgets, mainglist,
			WA_DragBar, TRUE,
			WA_DepthGadget, TRUE,
			TAG_DONE
		);
	if (!mainwin)
		return ERR_WINDOW;
	GT_RefreshWindow(mainwin, NULL);
	SetWindowPointer(mainwin, WA_BusyPointer, TRUE, TAG_DONE);
	if (device)
		findHDs(device, 2);
	else
	{
		findHDs("ide.device", 4);
		findHDs("scsi.device", 6);
	}
	findPartitionTables(mainwin, &hd_list);
	SetWindowPointer(mainwin, WA_BusyPointer, FALSE, TAG_DONE);
	return ERR_NONE;
}

void uninitEnv() {
	if (mainwin)
		CloseWindow(mainwin);
	if (dglist)
		freeGadgets(dglist);
	if (pcpglist)
		freeGadgets(pcpglist);
	if (mainglist)
		freeGadgets(mainglist);
	freePTGadget(scr);
	if (visual)
		FreeVisualInfo(visual);
	if (scr)
		UnlockPubScreen(NULL, scr);
	if (PartitionBase)
		CloseLibrary((struct Library *)PartitionBase);
	if (GadToolsBase)
		CloseLibrary(GadToolsBase);
	if (GfxBase)
		CloseLibrary((struct Library *)GfxBase);
	if (IntuitionBase)
		CloseLibrary((struct Library *)IntuitionBase);
}

void waitMessage(struct Window *win) {
struct IntuiMessage *msg;
struct PartitionTableNode *current_pt=0;
struct PartitionNode *current_partition=0;
BOOL running=TRUE;

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
					current_pt = getPartitionTable(mainwin, msg->Code);
					break;
				case ID_MAIN_SAVE_CHANGES :
					saveChanges(mainwin, current_pt);
					break;
				case ID_MAIN_EXIT :
					if (reallyExit(&pt_list))
					{
						mountPartitions(&pt_list);
						freePartitionTableList(&pt_list);
						freeHDList(&hd_list);
						running=FALSE;
					}
					break;
				case ID_MAIN_PARTITION_DRIVE :
					par_Init(0, current_pt);
					RemoveGList(win, mainglist, ~0);
					clearGadgets((struct ExtGadget *)mainglist, win, -1);
					AddGList(win, pcpglist, ~0, ~0, NULL);
					RefreshGList(win->FirstGadget, win, NULL, -1);
					GT_RefreshWindow(mainwin, NULL);
					break;
				case ID_PCP_PARTITION :
					current_partition =(struct PartitionNode *)getNumNode(&current_pt->pl, msg->Code);
					viewPartitionData(mainwin, current_pt, current_partition);
					break;
				case ID_PCP_ADD_PARTITION :
					current_partition = addPartition
						(
							mainwin,
							current_pt,
							(struct DosEnvec *)current_partition
						);
					break;
				case ID_PCP_DELETE_PARTITION :
					deletePartition(mainwin, current_partition);
					current_partition = 0;
					break;
				case ID_PCP_EDIT_PARTITION :
					RemoveGList(win, pcpglist, ~0);
					clearGadgets((struct ExtGadget *)pcpglist, win, -1);
					AddGList(win, dglist, ~0, ~0, NULL);
					RefreshGList(win->FirstGadget, win, NULL, -1);
					GT_RefreshWindow(mainwin, NULL);
					det_Init(win, current_partition);
					break;
				case ID_PCP_STARTCYL :
						changeStartCyl
						(
							mainwin,
							current_pt,
							current_partition,
							(
								(struct StringInfo *)
									(
										(struct Gadget *) msg->IAddress
									)->SpecialInfo
							)->LongInt
						);
					break;
				case ID_PCP_ENDCYL :
						changeEndCyl
						(
							mainwin,
							current_pt,
							current_partition,
							(
								(struct StringInfo *)
									(
										(struct Gadget *) msg->IAddress
									)->SpecialInfo
							)->LongInt
						);
					break;
				case ID_PCP_TOTALCYL :
					changeTotalCyl
					(
						mainwin,
						current_pt,
						current_partition,
						(
							(struct StringInfo *)
								(
									(struct Gadget *) msg->IAddress
								)->SpecialInfo
						)->LongInt
					);
					break;
				case ID_PCP_NAME:
					changeName
					(
						mainwin,
						current_partition,
						(
							(struct StringInfo *)
								(
									(struct Gadget *) msg->IAddress
								)->SpecialInfo
						)->Buffer
					);
					break;
				case ID_PCP_BOOTABLE:
					if (msg->Code)
						current_partition->flags |= PNF_BOOTABLE;
					else
						current_partition->flags &= ~PNF_BOOTABLE;
					current_partition->flags |= PNF_FLAGS_CHANGED;
					viewPartitionData(mainwin, current_pt, current_partition);
					break;
				case ID_PCP_BOOTPRI:
					changeBootPri
					(
						mainwin,
						current_partition,
						(
							(struct StringInfo *)
								(
									(struct Gadget *) msg->IAddress
								)->SpecialInfo
						)->LongInt
					);
					break;
				case ID_PCP_OK :
					if (pcp_Ok(current_pt))
					{
						sctdtags[0].ti_Data = FALSE;
						GT_SetGadgetAttrsA
						(
							maingadgets[ID_MAIN_SAVE_CHANGES-ID_MAIN_FIRST_GADGET].gadget,
							0, 0, sctdtags
						);
						current_pt->flags |= PNF_TABLE_CHANGED;
					}
					RemoveGList(win, pcpglist, ~0);
					clearGadgets((struct ExtGadget *)pcpglist, win, -1);
					AddGList(win, mainglist, ~0, ~0, NULL);
					RefreshGList(win->FirstGadget, win, NULL, -1);
					GT_RefreshWindow(mainwin, NULL);
					break;
				case ID_PCP_CANCEL :
					pcp_Cancel(current_pt);
					RemoveGList(win, pcpglist, ~0);
					clearGadgets((struct ExtGadget *)pcpglist, win, -1);
					AddGList(win, mainglist, ~0, ~0, NULL);
					RefreshGList(win->FirstGadget, win, NULL, -1);
					GT_RefreshWindow(mainwin, NULL);
					break;
				case ID_DET_TYPELV :
					{
					struct PartitionTypeNode *ptypenode;

						ptypenode = (struct PartitionTypeNode *)getNumNode(current_pt->typelist, msg->Code);
						changeType
						(
							mainwin,
							current_partition,
							ptypenode->id,
							ptypenode->id_len
						);
					}
					break;
				case ID_DET_TYPESTRING :
					{
					UBYTE id[32];
					UWORD id_len;

						id_len = ownsprintf
						(
							id,
							(
								(struct StringInfo *)
									((struct Gadget *) msg->IAddress)->SpecialInfo
							)->Buffer
						);
						changeType
						(
							mainwin, current_partition, id, id_len
						);
					}
					break;
				case ID_DET_PARTITION_TABLE:
					setPartitionTable(mainwin, current_partition, msg->Code);
					break;
				case ID_DET_BLOCKSIZE:
					current_partition->de.de_SizeBlock = 1<<(msg->Code+7);
					current_partition->flags |= PNF_DE_CHANGED;
					break;
				case ID_DET_BUFFERS:
					current_partition->de.de_NumBuffers =
						(
							(struct StringInfo *)
								((struct Gadget *) msg->IAddress)->SpecialInfo
						)->LongInt;
					current_partition->flags |= PNF_DE_CHANGED;
					break;
				case ID_DET_MASK:
					current_partition->de.de_Mask = strtoul
						(
							(
								(struct StringInfo *)
									((struct Gadget *) msg->IAddress)->SpecialInfo
							)->Buffer,
							NULL,
							0
						);
					current_partition->flags |= PNF_DE_CHANGED;
					setDetGadgets(mainwin, current_partition);
					break;
				case ID_DET_MAX_TRANSFER:
					current_partition->de.de_MaxTransfer = strtol
						(
							(
								(struct StringInfo *)
									((struct Gadget *) msg->IAddress)->SpecialInfo
							)->Buffer,
							NULL,
							0
						);
					current_partition->flags |= PNF_DE_CHANGED;
					setDetGadgets(mainwin, current_partition);
					break;
				case ID_DET_AUTOMOUNT:
					if (msg->Code)
						current_partition->flags |= PNF_AUTOMOUNT;
					else
						current_partition->flags &= ~PNF_AUTOMOUNT;
					current_partition->flags |= PNF_FLAGS_CHANGED;
					break;
				case ID_DET_CUSTBOOT:
					if (msg->Code)
						current_partition->de.de_TableSize = DE_BOOTBLOCKS+1;
					else
						current_partition->de.de_TableSize = DE_DOSTYPE+1;
					current_partition->flags |= PNF_DE_CHANGED;
					setDetGadgets(mainwin, current_partition);
					break;
				case ID_DET_CUSTBB:
					current_partition->de.de_BootBlocks =
						(
							(struct StringInfo *)
								((struct Gadget *) msg->IAddress)->SpecialInfo
						)->LongInt;
					current_partition->flags |= PNF_DE_CHANGED;
					break;
				case ID_DET_BEGINING:
					current_partition->de.de_Reserved =
						(
							(struct StringInfo *)
								((struct Gadget *) msg->IAddress)->SpecialInfo
						)->LongInt;
					current_partition->flags |= PNF_DE_CHANGED;
					break;
				case ID_DET_END:
					current_partition->de.de_PreAlloc =
						(
							(struct StringInfo *)
								((struct Gadget *) msg->IAddress)->SpecialInfo
						)->LongInt;
					current_partition->flags |= PNF_DE_CHANGED;
					break;
				case ID_DET_OK:
					det_Ok(current_partition);
					RemoveGList(win, dglist, ~0);
					clearGadgets((struct ExtGadget *)dglist, win, -1);
					AddGList(win, pcpglist, ~0, ~0, NULL);
					RefreshGList(win->FirstGadget, win, NULL, -1);
					GT_RefreshWindow(mainwin, NULL);
					viewPartitionData(mainwin, current_pt, current_partition);
					break;
				case ID_DET_CANCEL:
					det_Cancel(current_partition);
					RemoveGList(win, dglist, ~0);
					clearGadgets((struct ExtGadget *)dglist, win, -1);
					AddGList(win, pcpglist, ~0, ~0, NULL);
					RefreshGList(win->FirstGadget, win, NULL, -1);
					GT_RefreshWindow(mainwin, NULL);
					viewPartitionData(mainwin, current_pt, current_partition);
					break;
				case ID_PCP_PARTITION_GUI:
					current_partition =
						((struct Gadget *) msg->IAddress)->SpecialInfo;
					if (msg->Code == PTS_PARTITION)
					{
						viewPartitionData(mainwin, current_pt, current_partition);
					}
					else if (msg->Code == PTS_EMPTY_AREA)
					{
						viewDosEnvecData
						(
							mainwin,
							current_pt,
							(struct DosEnvec *)current_partition
						);
					}
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

