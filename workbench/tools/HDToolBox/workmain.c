/*
	$Id$
*/

#include <stdio.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include <devices/trackdisk.h>
#include <exec/memory.h>

#include "workmain.h"
#include "gadgets.h"
#include "listfunctions.h"
#include "pcpartitiontable.h"

extern struct List hd_list;
extern struct TagItem hdtags[], cdttags[], mbbltags[], llftags[],
                      pdtags[], vdodtags[], sctdtags[];

extern struct creategadget maingadgets[];

ULONG readDisk
	(
		struct HDUnitNode *hdunit,
		ULONG start,
		ULONG count,
		APTR mem,
		ULONG cmd
	) {
QUAD offset;

	hdunit->ioreq->iotd_Req.io_Command=cmd;
	hdunit->ioreq->iotd_Req.io_Length=count*512;
	hdunit->ioreq->iotd_Req.io_Data=mem;
	offset=start*512;
	hdunit->ioreq->iotd_Req.io_Offset=0xFFFFFFFF & offset;
	hdunit->ioreq->iotd_Req.io_Actual=offset>>32;
	return DoIO((struct IORequest *)&hdunit->ioreq->iotd_Req);
}

int getGeometry(struct HDUnitNode *hdunit) {
	hdunit->ioreq->iotd_Req.io_Command = TD_GETGEOMETRY;
	hdunit->ioreq->iotd_Req.io_Length = sizeof(struct DriveGeometry);
	hdunit->ioreq->iotd_Req.io_Data = &hdunit->geometry;
	return DoIO((struct IORequest *)&hdunit->ioreq->iotd_Req);
}


void findHDs(struct Window *mainwin, char *device, int maxUnits) {
struct HDUnitNode *node;
ULONG args[3];
struct EasyStruct es = {sizeof(struct EasyStruct), 0, "HDToolBox", 0, "Ok"};
int i,count;

	for (count=0;(device[count]!='.') && (device[count]!=0);count++);
	hdtags[0].ti_Data = (STACKIPTR)&hd_list;
	for (i=0;i<maxUnits;i++)
	{
		node = (struct HDUnitNode *)AllocMem(sizeof(struct HDUnitNode), MEMF_PUBLIC | MEMF_CLEAR);
		if (node)
		{
			node->ioport = CreateMsgPort();
			if (node->ioport)
			{
				node->ioreq = (struct IOExtTD *)CreateIORequest(node->ioport, sizeof(struct IOExtTD));
				if (node->ioreq)
				{
					node->ln.ln_Name = AllocMem(100, MEMF_PUBLIC | MEMF_CLEAR);
					if (node->ln.ln_Name)
					{
						sprintf(node->ln.ln_Name, "%.*s   %d   0", count, device, i);
						if (OpenDevice(device,i,(struct IORequest *)&node->ioreq->iotd_Req,0) == 0)
						{
							readDisk(node, 0, 1, (APTR)node->mbr, CMD_READ);
							if (getGeometry(node))
							{
								es.es_TextFormat = "Couldn't get geometry! Using standard values\nHeads = %ld Cyls = %ld Sec/Track = %ld";
								node->geometry.dg_SectorSize = 512;
								node->geometry.dg_TrackSectors = 63;
								node->geometry.dg_Heads = 255;
								node->geometry.dg_CylSectors =
									node->geometry.dg_TrackSectors *
									node->geometry.dg_Heads;
								args[0] = node->geometry.dg_Heads;
								args[1] = node->geometry.dg_Cylinders;
								args[2] = node->geometry.dg_TrackSectors;
								EasyRequestArgs(0, &es, 0, args);
							}
							if (node->geometry.dg_DeviceType != DG_CDROM)
							{
								node->partition_changed = FALSE;
								AddTail(&hd_list, &node->ln);
								SetGadgetAttrsA
									(
										maingadgets[ID_MAIN_HARDDISK-ID_MAIN_FIRST_GADGET].gadget,
										mainwin,
										0,
										hdtags
									);
								continue;
							}
							CloseDevice((struct IORequest *)&node->ioreq->iotd_Req);
						}
						FreeMem(node->ln.ln_Name, 100);
					}
					DeleteIORequest((struct IORequest *)node->ioreq);
				}
				DeleteMsgPort(node->ioport);
			}
			FreeMem(node, sizeof(struct HDUnitNode));
		}
	}
}

struct HDUnitNode *getHDUnit(struct Window *mainwin, int num) {
struct HDUnitNode *current_hd;

	current_hd=(struct HDUnitNode *)getNumNode(&hd_list, num);
	if (current_hd)
	{
/*		cdttags[0].ti_Data = FALSE;
		mbbltags[0].ti_Data = FALSE;
		llftags[0].ti_Data = FALSE;*/
		pdtags[0].ti_Data = FALSE;
//		vdodtags[0].ti_Data = FALSE;
		sctdtags[0].ti_Data = current_hd->partition_changed ? FALSE : TRUE;
/*		SetGadgetAttrsA
			(
				maingadgets[ID_MAIN_CHANGE_DRIVE_TYPE-ID_MAIN_FIRST_GADGET].gadget,
				mainwin,0,cdttags
			);
		SetGadgetAttrsA
			(
				maingadgets[ID_MAIN_MODIFY_BBL-ID_MAIN_FIRST_GADGET].gadget,
				mainwin,0,mbbltags
			);
		SetGadgetAttrsA
			(
				maingadgets[ID_MAIN_LL_FORMAT-ID_MAIN_FIRST_GADGET].gadget,
				mainwin,0,llftags
			);*/
		SetGadgetAttrsA
			(
				maingadgets[ID_MAIN_PARTITION_DRIVE-ID_MAIN_FIRST_GADGET].gadget,
				mainwin,0,pdtags
			);
/*		SetGadgetAttrsA
			(
				maingadgets[ID_MAIN_VERIFY_DD-ID_MAIN_FIRST_GADGET].gadget,
				mainwin,0,vdodtags
			);*/
		SetGadgetAttrsA
			(
				maingadgets[ID_MAIN_SAVE_CHANGES-ID_MAIN_FIRST_GADGET].gadget,
				mainwin,0,sctdtags
			);
	}
	return current_hd;
}

BOOL reallyExit(struct List *hdlist) {
struct HDUnitNode *hdunit;
struct EasyStruct es =
	{
		sizeof(struct EasyStruct), 0,
		"HDToolBox",
		"Some partitions have been changed.\n"
		"Are you sure to quit?",
		"Yes|No"
	};

	hdunit = (struct HDUnitNode *)hdlist->lh_Head;
	while (hdunit->ln.ln_Succ)
	{
		if (hdunit->partition_changed)
			return EasyRequestArgs(0, &es, 0, 0);
		hdunit = (struct HDUnitNode *)hdunit->ln.ln_Succ;
	}
	return TRUE;
}

void freeHDList(struct List *hdlist) {
struct HDUnitNode *hdunit;

	while ((hdunit = (struct HDUnitNode *)RemHead(hdlist)))
	{
		CloseDevice((struct IORequest *)hdunit->ioreq);
		FreeMem(hdunit->ln.ln_Name, 100);
		DeleteIORequest((struct IORequest *)hdunit->ioreq);
		DeleteMsgPort(hdunit->ioport);
		FreeMem(hdunit, sizeof(struct HDUnitNode));
	}
}

void saveChanges(struct Window *mainwin, struct HDUnitNode *hdunit) {
struct EasyStruct es =
	{
		sizeof(struct EasyStruct), 0,
		"HDToolBox",
		"Partition0:\n"
		"   status = %ld, type = %ld, first sector = %ld, last sector = %ld\n"
		"   (s_head=%ld, s_sector=%ld, s_cylinder=%ld)\n"
		"   (e_head=%ld, e_sector=%ld, e_cylinder=%ld)\n"
		"Partition1:\n"
		"   status = %ld, type = %ld, first sector = %ld, last sector = %ld\n"
		"   (s_head=%ld, s_sector=%ld, s_cylinder=%ld)\n"
		"   (e_head=%ld, e_sector=%ld, e_cylinder=%ld)\n"
		"Partition2:\n"
		"   status = %ld, type = %ld, first sector = %ld, last sector = %ld\n"
		"   (s_head=%ld, s_sector=%ld, s_cylinder=%ld)\n"
		"   (e_head=%ld, e_sector=%ld, e_cylinder=%ld)\n"
		"Partition3:\n"
		"   status = %ld, type = %ld, first sector = %ld, last sector = %ld\n"
		"   (s_head=%ld, s_sector=%ld, s_cylinder=%ld)\n"
		"   (e_head=%ld, e_sector=%ld, e_cylinder=%ld)\n"
		"\nAll data on modified partitions will be destroyed!\n"
		"Are you sure to write changes to disk?",
		"Yes|No"
	};
struct PCPartitionTable *pcpt=(struct PCPartitionTable *)&hdunit->mbr_copy[0x1BE];
int i;
ULONG args[40];

	for (i=0;i<4;i++)
	{
		args[i*10+0] = (ULONG)pcpt[i].status;
		args[i*10+1] = (ULONG)pcpt[i].type;
		args[i*10+2] = pcpt[i].first_sector;
		args[i*10+3] = pcpt[i].count_sector;
		args[i*10+4] = (ULONG)pcpt[i].start_head;
		args[i*10+5] = (ULONG)pcpt[i].start_sector;
		args[i*10+6] = (ULONG)pcpt[i].start_cylinder;
		args[i*10+7] = (ULONG)pcpt[i].end_head;
		args[i*10+8] = (ULONG)pcpt[i].end_sector;
		args[i*10+9] = (ULONG)pcpt[i].end_cylinder;
	}
	if (EasyRequestArgs(0, &es, 0, args))
	{
		readDisk(hdunit, 0, 1, (APTR)hdunit->mbr_copy, CMD_WRITE);
		hdunit->partition_changed = FALSE;
		sctdtags[0].ti_Data = TRUE;
		SetGadgetAttrsA
			(
				maingadgets[ID_MAIN_SAVE_CHANGES-ID_MAIN_FIRST_GADGET].gadget,
				mainwin,0,sctdtags
			);
	}
}
	
