/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include <intuition/cghooks.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/intuition.h>
#include <intuition/imageclass.h>

#include "ptclass.h"
#include "partitions.h"
#include "partitiontables.h"
#include "platform.h"

#define DEBUG 0
#include "debug.h"

#define G(a) ((struct Gadget *)a)

struct PTableData {
	struct DrawInfo *dri;
	struct Image *frame;
	struct HDTBPartition *table;
	struct HDTBPartition *active; /* active partition */
	struct DosEnvec gap;
	ULONG block;
	ULONG flags;
	BOOL move;
	BOOL selected;
};

STATIC UWORD pattern[]=
{
	0xAAAA, 0xAAAA,
	0x5555, 0x5555
};

#define SetPattern(r,p,s) {r->AreaPtrn = (UWORD *)p; r->AreaPtSz = s;}

Class *ptcl=0;


STATIC IPTR pt_new(Class *cl, Object *obj, struct opSet *msg) {
struct PTableData *data;
struct DrawInfo *dri;
struct Image *frame;
struct HDTBPartition *table;
ULONG flags;
struct TagItem tags[]=
{
	{IA_Width	, 0UL		},
	{IA_Height	, 0UL		},
	{IA_Resolution	, 0UL		},
	{IA_FrameType	, FRAME_BUTTON	},
	{TAG_DONE			}
};

	dri = (struct DrawInfo *) GetTagData(GA_DrawInfo, NULL, msg->ops_AttrList);
	if (!dri)
		return NULL;
	tags[0].ti_Data = GetTagData(GA_Width, 0, msg->ops_AttrList);
	tags[1].ti_Data = GetTagData(GA_Height, 0, msg->ops_AttrList);
	table = (struct HDTBPartition *)GetTagData(PTCT_PartitionTable, 0, msg->ops_AttrList);
	flags = GetTagData(PTCT_Flags, 0, msg->ops_AttrList);
	tags[2].ti_Data = (dri->dri_Resolution.X << 16) + dri->dri_Resolution.Y;
	frame = (struct Image *) NewObjectA(NULL, FRAMEICLASS, tags);
	if (!frame)
		return NULL;

	tags[0].ti_Tag = GA_Image;
	tags[0].ti_Data = (IPTR) frame;
	tags[1].ti_Tag = TAG_MORE;
	tags[1].ti_Data = (IPTR) msg->ops_AttrList;
	obj = (Object *) DoSuperMethodA(cl, obj, (Msg)msg);
	if (!obj)
	{
		DisposeObject(frame);
		return NULL;
	}
	data = INST_DATA(cl, obj);
	data->dri = dri;
	data->frame = frame;
	data->table = table;
	data->flags = flags;
	data->active = 0;
	data->move = FALSE;
	data->selected = FALSE;
	return (IPTR)obj;
}

STATIC IPTR pt_set(Class *cl, Object *obj, struct opSet *msg) {
IPTR retval = 0UL;
struct PTableData *data;
struct TagItem *tag;
struct TagItem *taglist;
struct RastPort *rport;

	if (msg->MethodID != OM_NEW)
		retval = DoSuperMethodA(cl, obj, (Msg)msg);

	data = INST_DATA(cl, obj);
	taglist = (struct TagItem *)msg->ops_AttrList;
	while ((tag = NextTagItem(&taglist)))
	{
		switch (tag->ti_Tag)
		{
		case GA_Disabled:
			retval = TRUE;
			break;
		case GA_DrawInfo:
			if (msg->MethodID == OM_NEW)
				data->dri = (struct DrawInfo *)tag->ti_Data;
			break;
		case PTCT_PartitionTable:
			data->table = (struct HDTBPartition *)tag->ti_Data;
			retval = TRUE;
			break;
		case PTCT_ActivePartition:
			data->active = (struct HDTBPartition *)tag->ti_Data;
			retval = TRUE;
			break;
		}
	}

	/* redraw if one attribute has changed something */
	if ((retval) && (OCLASS(obj) == cl))
	{
		rport = ObtainGIRPort(msg->ops_GInfo);
		if (rport)
		{
			DoMethod(obj, GM_RENDER, msg->ops_GInfo, rport, GREDRAW_UPDATE);
			ReleaseGIRPort(rport);
			retval = FALSE;
		}
	}
	return retval;
}

STATIC IPTR pt_get(Class *cl, Object *obj, struct opGet *msg) {
struct PTableData *data=INST_DATA(cl, obj);
IPTR retval;

	switch (msg->opg_AttrID)
	{
	case PTCT_PartitionTable:
		*msg->opg_Storage = (IPTR)data->table;
		break;
	case PTCT_ActivePartition:
		*msg->opg_Storage = (IPTR)data->active;
		break;
	case PTCT_ActiveType:
		if ((struct DosEnvec *)data->active == &data->gap)
			*msg->opg_Storage = (IPTR)PTS_EMPTY_AREA;
		else if (data->active == NULL)
			*msg->opg_Storage = (IPTR)PTS_NOTHING;
		else
			*msg->opg_Storage = (IPTR)PTS_PARTITION;
	case PTCT_Flags:
		break;
	default:
		retval = DoSuperMethodA(cl, obj, (Msg)msg);
	}
	return retval;
}

struct DosEnvec *findSpace
	(
		struct HDTBPartition *table,
		struct DosEnvec *de,
		ULONG block
	)
{
struct HDTBPartition *pn;
ULONG spc;
ULONG first=0;
ULONG last=0xFFFFFFFF;

	/* inherit */
	CopyMem(&table->table->de, de, sizeof(struct DosEnvec));
	pn = (struct HDTBPartition *)table->listnode.list.lh_Head;
	while (pn->listnode.ln.ln_Succ)
	{
	ULONG start;
	ULONG end;

		spc = pn->de.de_Surfaces*pn->de.de_BlocksPerTrack;
		start = pn->de.de_LowCyl*spc;
		end = ((pn->de.de_HighCyl+1)*spc)-1;
		if (block<start)
		{
			if (start<last)
				last=start-1;
		}
		else if (block>end)
		{
			if (end>first)
				first = end+1;
		}
		pn = (struct HDTBPartition *)pn->listnode.ln.ln_Succ;
	}
	spc = table->table->de.de_Surfaces*table->table->de.de_BlocksPerTrack;
	if (first == 0)
		first = table->table->reserved;
	first /= spc;
	last = ((last+1)/spc)-1;
	if (last>table->table->de.de_HighCyl)
		last=table->table->de.de_HighCyl;
	de->de_LowCyl = first;
	de->de_HighCyl = last;
	return de;
}

struct HDTBPartition *getActive(struct PTableData *data) {
struct HDTBPartition *pn;

	pn = (struct HDTBPartition *)data->table->listnode.list.lh_Head;
	while (pn->listnode.ln.ln_Succ)
	{
	ULONG start;
	ULONG end;

		start = pn->de.de_LowCyl*pn->de.de_Surfaces*pn->de.de_BlocksPerTrack;
		end = (pn->de.de_HighCyl+1)*pn->de.de_Surfaces*pn->de.de_BlocksPerTrack;
		if ((data->block>=start) && (data->block<end))
			return pn;
		pn = (struct HDTBPartition *)pn->listnode.ln.ln_Succ;
	}
	return (struct HDTBPartition *)findSpace(data->table,&data->gap,data->block);
}

void DrawBox(struct RastPort *rport, UWORD sx, UWORD sy, UWORD ex, UWORD ey) {

	SetAPen(rport, 1);
	Move(rport, ex, sy);
	Draw(rport, ex, ey);
	Draw(rport, sx, ey);
	SetAPen(rport, 2);
	Draw(rport, sx, sy);
	Draw(rport, ex, sy);
}

void DrawFilledBox
	(
		struct RastPort *rport,
		UWORD sx, UWORD sy, UWORD ex, UWORD ey
	)
{
	RectFill(rport, sx+1, sy+1, ex-1, ey-1);
	DrawBox(rport, sx, sy, ex, ey);
}

void DrawPartition
	(
		struct RastPort *rport,
		struct Gadget *gadget,
		struct HDTBPartition *table,
		struct DosEnvec *pn
	)
{
ULONG start;
ULONG end;
ULONG blocks;
ULONG spc;
ULONG cyls;
ULONG skipcyl;

#if 0
	blocks =
		(table->table->de_HighCyl+1)*
		table->table->de_Surfaces*
		table->table->de_BlocksPerTrack;
#else
	blocks = table->table->de.de_Surfaces*table->table->de.de_BlocksPerTrack;
	skipcyl = table->table->reserved/blocks;
	cyls = table->table->de.de_HighCyl-skipcyl;
#endif
	spc = pn->de_Surfaces*pn->de_BlocksPerTrack;
#if 0
	start = pn->de_LowCyl*spc*gadget->Width/blocks;
	end = (((pn->de_HighCyl+1)*spc)-1)*gadget->Width/blocks;
#else
	start = pn->de_LowCyl;
	end = pn->de_HighCyl;
	if (
			(pn->de_Surfaces!=table->table->de.de_Surfaces) ||
			(pn->de_BlocksPerTrack!=table->table->de.de_BlocksPerTrack)
		)
	{
		start = start*spc/blocks;
		end   = end*spc/blocks;
	}
	if (start!=0)
		start -= skipcyl;
	if (end !=0)
		end   -= skipcyl;
	start = (start*(gadget->Width)/cyls)+1;
	end  = (end*(gadget->Width)/cyls);
	start += gadget->LeftEdge;
	end   += gadget->LeftEdge;
#endif
	DrawFilledBox
	(
		rport,
		start, gadget->TopEdge+1+(gadget->Height/2),
		end, gadget->TopEdge+(gadget->Height)-1
	);
}

void ErasePartition
	(
		struct RastPort *rport,
		struct Gadget *gadget,
		struct HDTBPartition *table,
		struct DosEnvec *pn
	)
{
ULONG start;
ULONG end;
ULONG blocks;
ULONG spc;
ULONG cyls;
ULONG skipcyl;

#if 0
	blocks =
		(table->table->de_HighCyl+1)*
		table->table->de_Surfaces*
		table->table->de_BlocksPerTrack;
#else
	blocks = table->table->de.de_Surfaces*table->table->de.de_BlocksPerTrack;
	skipcyl = table->table->reserved/blocks;
	cyls = table->table->de.de_HighCyl-skipcyl;
#endif
	spc = pn->de_Surfaces*pn->de_BlocksPerTrack;
#if 0
	start = pn->de_LowCyl*spc*gadget->Width/blocks;
	end = (((pn->de_HighCyl+1)*spc)-1)*gadget->Width/blocks;
#else
	start = pn->de_LowCyl;
	end = pn->de_HighCyl;
	if (
			(pn->de_Surfaces!=table->table->de.de_Surfaces) ||
			(pn->de_BlocksPerTrack!=table->table->de.de_BlocksPerTrack)
		)
	{
		start *= spc/blocks;
		end   *= spc/blocks;
	}
	if (start!=0)
		start -= skipcyl;
	if (end !=0)
		end   -= skipcyl;
	start = (start*(gadget->Width)/cyls)+1;
	end  = (end*(gadget->Width)/cyls);
	start += gadget->LeftEdge;
	end   += gadget->LeftEdge;
#endif
	EraseRect	
	(
		rport,
		start, gadget->TopEdge+1+(gadget->Height/2),
		end, gadget->TopEdge+(gadget->Height)-1
	);
}
ULONG getBlock(UWORD mousex, UWORD width, struct HDTBPartition *table) {
ULONG block;

	block = mousex*table->table->de.de_HighCyl/width;
	block *= table->table->de.de_Surfaces*table->table->de.de_BlocksPerTrack;
	block += table->table->reserved;
	return block;
}

STATIC VOID notify_all(Class *cl, Object *obj, struct GadgetInfo *gi, BOOL interim, BOOL userinput) {
struct PTableData *data=INST_DATA(cl, obj);
struct opUpdate opu;
struct TagItem tags[]=
{
	{GA_ID,               G(obj)->GadgetID},
#ifndef __AMIGAOS__
	{GA_UserInput,        userinput},
#endif
	{PTCT_ActivePartition,(IPTR)data->active},
	{PTCT_PartitionTable, (IPTR)data->table},
	{PTCT_PartitionMove,  (IPTR)data->move},
	{PTCT_Selected,       (IPTR)data->selected},
	{TAG_DONE}
};

	opu.MethodID = OM_NOTIFY;
	opu.opu_AttrList = tags;
	opu.opu_GInfo = gi;
	opu.opu_Flags = interim ? OPUF_INTERIM : 0;
	DoMethodA(obj, (Msg)&opu);
}

STATIC IPTR pt_goactive(Class *cl, Object *obj, struct gpInput *msg) {
IPTR retval = GMR_MEACTIVE;
struct PTableData *data;
struct DosEnvec *de;
struct RastPort *rport;

	data = INST_DATA(cl, obj);
	data->block = getBlock(msg->gpi_Mouse.X, G(obj)->Width,data->table);
	rport = msg->gpi_GInfo->gi_RastPort;
	if (data->active != NULL)
	{
		/* draw previous active as unselected */
		if ((struct DosEnvec *)data->active == &data->gap)
		{
			SetAPen(rport, 0);
			de = &data->gap;
		}
		else
		{
			SetAPen(rport, 2);
			de = &data->active->de;
		}
		SetPattern(rport, &pattern, 2);
		DrawPartition(rport, (struct Gadget *)obj, data->table, de);
		SetPattern(rport, 0, 0);
	}
	data->active = getActive(data);
	if ((struct DosEnvec *)data->active == &data->gap)
	{
		SetAPen(rport, 1);
		de = &data->gap;
	}
	else
	{
		SetAPen(rport, 3);
		de = &data->active->de;
		if (data->flags & PTCTF_EmptySelectOnly)
			data->active=0;
	}
	data->selected = TRUE;
	notify_all(cl, obj, msg->gpi_GInfo, TRUE, TRUE);
	data->selected = FALSE;
	if (data->active)
	{
		DrawPartition(rport, (struct Gadget *)obj, data->table, de);
	}
	return retval;
}

STATIC IPTR pt_goinactive(Class *cl, Object *obj, struct gpInput *msg) {
IPTR retval = TRUE;
struct PTableData *data;
struct DosEnvec *de;
struct RastPort *rport;

	data = INST_DATA(cl, obj);
	if (data->active)
	{
//		data->block = getBlock(msg->gpi_Mouse.X, G(obj)->Width, data->table);
		if (getActive(data) == data->active)
		{
			rport = msg->gpi_GInfo->gi_RastPort;
			if ((struct DosEnvec *)data->active == &data->gap)
			{
				SetAPen(rport, 1);
				de = &data->gap;
			}
			else
			{
				SetAPen(rport, 3);
				de = &data->active->de;
			}
			SetPattern(rport, &pattern, 2);
			DrawPartition(rport, (struct Gadget *)obj, data->table, de);
			SetPattern(rport, 0, 0);
		}
	}
	return retval;
}

ULONG overflow_add(ULONG a, LONG b) {
ULONG result;

	result = a + b;
	if (a > result)
		result = (ULONG)-1;
	return result;
}

ULONG underflow_add(ULONG a, LONG b) {
ULONG result;

	if (a<-b)
		result = 0;
	else
		result = a+b;
	return result;
}

BOOL overlap(ULONG a, ULONG b, ULONG c, ULONG d) {

	if (a>b)
	{
		ULONG e;
		e = b;
		b = a;
		a = e;
	}
	return
		(
			(((a >= c) && (a < d)) || ((b >= c) && (b < d))) ||
			(((c >= a) && (c < b)) || ((d >= a) && (d < b)))
		);
}

LONG getBetterDiff
	(
		struct HDTBPartition *table,
		struct HDTBPartition *current,
		LONG diff
	)
{
struct HDTBPartition *pn;
ULONG spc;
ULONG oldblock;
ULONG block;
ULONG other;
ULONG start;
ULONG end;
ULONG size;

	spc = current->de.de_Surfaces*current->de.de_BlocksPerTrack;
	start = current->de.de_LowCyl*spc;
	end = ((current->de.de_HighCyl+1)*spc)-1;
	size = end-start;
	if (diff>0)
	{
		oldblock = end;
		block = overflow_add(end, diff);
		other = block-size;
	}
	else
	{
		oldblock = start;
		block = underflow_add(start, diff);
		other = block+size; /* use correct partition size */
	}
	if (block<table->table->reserved)
	{
		diff = table->table->reserved-oldblock;
		if (diff == 0)
			return 0;
		return getBetterDiff(table, current, diff);
	}
	spc = table->table->de.de_Surfaces*table->table->de.de_BlocksPerTrack;
	if (block>=((table->table->de.de_HighCyl+1)*spc))
	{
		diff = (((table->table->de.de_HighCyl+1)*spc)-1)-oldblock;
		if (diff == 0)
			return 0;
		return getBetterDiff(table, current, diff);
	}
	pn = (struct HDTBPartition *)table->listnode.list.lh_Head;
	while (pn->listnode.ln.ln_Succ)
	{
		/* dont't check currently processed partition */
		if (current != pn)
		{
			spc = pn->de.de_Surfaces*pn->de.de_BlocksPerTrack;
			if (
					overlap
					(
						block, other,
						pn->de.de_LowCyl*spc, ((pn->de.de_HighCyl+1)*spc)-1
					)
				)
			{
				if (diff>0)
					diff = ((pn->de.de_LowCyl*spc)-1)-oldblock;
				else
					diff = ((pn->de.de_HighCyl+1)*spc)-oldblock;
				return getBetterDiff(table, current, diff);
			}
		}
		pn = (struct HDTBPartition *)pn->listnode.ln.ln_Succ;
	}
	return diff;
}

STATIC IPTR pt_handleinput(Class *cl, Object *obj, struct gpInput *msg) {
IPTR retval = GMR_MEACTIVE;
struct InputEvent *ie;
struct PTableData *data;

	data = INST_DATA(cl, obj);
	ie = msg->gpi_IEvent;
	if (ie->ie_Class == IECLASS_RAWMOUSE)
	{
		switch (ie->ie_Code)
		{
		case IECODE_NOBUTTON:
			if (
					((struct DosEnvec *)data->active != &data->gap) &&
					(!(data->flags & PTCTF_NoPartitionMove))
				)
			{
			ULONG block;
			LONG diff;
			ULONG start;
			ULONG end;
			ULONG spc;
			ULONG tocheck;

				block = getBlock(msg->gpi_Mouse.X, G(obj)->Width, data->table);
				diff = block-data->block;
				if (diff)
				{
					diff = getBetterDiff(data->table, data->active, diff);
					if (diff)
					{
						spc=data->active->de.de_Surfaces*data->active->de.de_BlocksPerTrack;
						start = data->active->de.de_LowCyl*spc;
						start += diff;
						end = ((data->active->de.de_HighCyl+1)*spc)-1;
						end += diff;
						tocheck = (diff>0) ? end : start;
#ifdef DEBUG
#if DEBUG>0
						if (validValue(data->table, data->active, tocheck))
						{
#endif
#endif
							start = start/spc;
							end = ((end+1)/spc)-1;
							ErasePartition
							(
								msg->gpi_GInfo->gi_RastPort,
								(struct Gadget *)obj,
								data->table, &data->active->de
							);
							data->active->de.de_LowCyl=start;
							data->active->de.de_HighCyl=end;
							SetAPen(msg->gpi_GInfo->gi_RastPort, 3);
							DrawPartition
							(
								msg->gpi_GInfo->gi_RastPort,
								(struct Gadget *)obj,
								data->table, &data->active->de
							);
							data->block = block;
							data->move = TRUE;
							notify_all(cl, obj, msg->gpi_GInfo, TRUE, TRUE);
#ifdef DEBUG
#if DEBUG>0
						}
						else
							kprintf("!!!!!!!!!!!!!!!!!!!not valid\n");
#endif
#endif
					}
				}
			}
			break;
		case SELECTUP:
			data->move = FALSE;
			notify_all(cl, obj, msg->gpi_GInfo, FALSE, TRUE);
			data->selected = FALSE;
			retval = GMR_NOREUSE|GMR_VERIFY;
			break;
		}
	}
	return retval;
}

void DrawLegend(struct RastPort *rport, LONG sx, LONG sy, LONG ex, LONG ey, char *text) {
struct TextAttr tattr;

	RectFill(rport, sx+1, sy+1, ex-1, ey-1);
	DrawBox(rport, sx, sy, ex, ey);
	SetAPen(rport, 1);
	AskFont(rport, &tattr);
	Move(rport, sx, ey+tattr.ta_YSize+1);
	Text(rport, text, strlen(text));
}

STATIC IPTR pt_render(Class *cl, Object *obj, struct gpRender *msg) {
struct PTableData *data = INST_DATA(cl, obj);
struct HDTBPartition *pn;
IPTR retval = 0;

	retval = DoSuperMethodA(cl, obj, (Msg)msg);
	EraseRect
	(
		msg->gpr_RPort,
		G(obj)->LeftEdge,
		G(obj)->TopEdge,
		G(obj)->LeftEdge+G(obj)->Width,
		G(obj)->TopEdge+(G(obj)->Height)
	);
	DrawBox
	(
		msg->gpr_RPort,
		G(obj)->LeftEdge,
		G(obj)->TopEdge+(G(obj)->Height/2),
		G(obj)->LeftEdge+G(obj)->Width,
		G(obj)->TopEdge+(G(obj)->Height)
	);
	SetPattern(msg->gpr_RPort, &pattern, 2);
	SetAPen(msg->gpr_RPort, 0);
	DrawLegend
	(
		msg->gpr_RPort,
		G(obj)->LeftEdge,
		G(obj)->TopEdge,
		G(obj)->LeftEdge+(G(obj)->Width/5),
		G(obj)->TopEdge+(G(obj)->Height/2)-(G(obj)->Height/4),
		"Unselected Empty"
	);
	SetAPen(msg->gpr_RPort, 1);
	DrawLegend
	(
		msg->gpr_RPort,
		G(obj)->LeftEdge+(G(obj)->Width/5)+(G(obj)->Width/15),
		G(obj)->TopEdge,
		G(obj)->LeftEdge+(G(obj)->Width/5*2)+(G(obj)->Width/15),
		G(obj)->TopEdge+(G(obj)->Height/2)-(G(obj)->Height/4),
		"Selected Empty"
	);
	SetAPen(msg->gpr_RPort, 2);
	DrawLegend
	(
		msg->gpr_RPort,
		G(obj)->LeftEdge+(G(obj)->Width/5*2)+(G(obj)->Width/15*2),
		G(obj)->TopEdge,
		G(obj)->LeftEdge+(G(obj)->Width/5*3)+(G(obj)->Width/15*2),
		G(obj)->TopEdge+(G(obj)->Height/2)-(G(obj)->Height/4),
		"Unselected Used"
	);
	SetAPen(msg->gpr_RPort, 3);
	DrawLegend
	(
		msg->gpr_RPort,
		G(obj)->LeftEdge+(G(obj)->Width/5*3)+(G(obj)->Width/15*3),
		G(obj)->TopEdge,
		G(obj)->LeftEdge+(G(obj)->Width/5*4)+(G(obj)->Width/15*3),
		G(obj)->TopEdge+(G(obj)->Height/2)-(G(obj)->Height/4),
		"Selected Used"
	);
#if 0
	RectFill
	(
		msg->gpr_RPort,
		G(obj)->LeftEdge,
		G(obj)->TopEdge,
		G(obj)->LeftEdge+G(obj)->Width+(G(obj)->Height/2),
		G(obj)->TopEdge+(G(obj)->Height)
	);
#endif
	if (data->table)
	{
		SetPattern(msg->gpr_RPort, &pattern, 2);
		pn = (struct HDTBPartition *)data->table->listnode.list.lh_Head;
		while (pn->listnode.ln.ln_Succ)
		{
			if (pn->listnode.ln.ln_Type == LNT_Partition)
			{
				if (data->active==pn)
					SetAPen(msg->gpr_RPort, 3);
				else
					SetAPen(msg->gpr_RPort, 2);
				DrawPartition
				(
					msg->gpr_RPort,
					(struct Gadget *)obj,
					data->table,
					&pn->de
				);
			}
			pn = (struct HDTBPartition *)pn->listnode.ln.ln_Succ;
		}
		if ((struct DosEnvec *)data->active == &data->gap)
		{
			SetAPen(msg->gpr_RPort, 1);
			DrawPartition
			(
				msg->gpr_RPort,
				(struct Gadget *)obj,
				data->table,
				&data->gap
			);
		}
		SetPattern(msg->gpr_RPort, 0, 0);
	}
	return retval;
}

STATIC IPTR pt_dispose(Class *cl, Object *obj, Msg msg) {
struct PTableData *data = INST_DATA(cl, obj);

	return DoSuperMethodA(cl, obj, msg);
}

STATIC IPTR pt_hittest(Class *cl, Object *obj, struct gpHitTest *msg) {

	return GMR_GADGETHIT;
}

AROS_UFH3S(IPTR, dispatch_ptclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, obj, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
	AROS_USERFUNC_INIT
	IPTR retval;
	switch (msg->MethodID)
	{
	case OM_NEW:
		retval = pt_new(cl, obj, (struct opSet *)msg);
		break;
	case OM_DISPOSE:
		retval = pt_dispose(cl, obj, msg);
		break;
	case OM_SET:
	case OM_UPDATE:
		retval = pt_set(cl, obj, (struct opSet *)msg);
		break;
	case OM_GET:
		retval = pt_get(cl, obj, (struct opGet *)msg);
		break;
	case GM_HITTEST:
		retval = pt_hittest(cl, obj, (struct gpHitTest *)msg);
		break;
	case GM_RENDER:
		retval = pt_render(cl, obj, (struct gpRender *)msg);
		break;
	case GM_GOACTIVE:
		retval = pt_goactive(cl, obj, (struct gpInput *)msg);
		break;
	case GM_GOINACTIVE:
		retval = pt_goinactive(cl, obj, (struct gpInput *)msg);
		break;
	case GM_HANDLEINPUT:
		retval = pt_handleinput(cl, obj, (struct gpInput *)msg);
		break;
	default:
kprintf("default %ld\n", msg->MethodID);
		retval = DoSuperMethodA(cl, obj, msg);
		break;
	}
	return retval;

	AROS_USERFUNC_EXIT
}

Class *makePTClass(void) {

	if (!ptcl)
	{
		ptcl = MakeClass(NULL, GADGETCLASS, NULL, sizeof(struct PTableData), 0);
		if (ptcl)
		{
			ptcl->cl_Dispatcher.h_Entry = AROS_ASMSYMNAME(dispatch_ptclass);
			ptcl->cl_Dispatcher.h_SubEntry = NULL;
			ptcl->cl_UserData = NULL;
		}
	}
	return ptcl;
}
