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

#define DEBUG 1
#include "debug.h"

#define G(a) ((struct Gadget *)a)

struct PTableData {
	struct DrawInfo *dri;
	struct Image *frame;
	struct PartitionTableNode *table;
	struct PartitionNode *selected;
	struct DosEnvec gap;
	ULONG block;
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
struct PartitionTableNode *table;
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
	table = (struct PartitionTableNode *)GetTagData(PTCT_PartitionTable, 0, msg->ops_AttrList);
	tags[2].ti_Data = (dri->dri_Resolution.X << 16) + dri->dri_Resolution.Y;
	frame = (struct Image *) NewObjectA(NULL, FRAMEICLASS, tags);
	if (!frame)
		return NULL;

	tags[0].ti_Tag = GA_Image;
	tags[0].ti_Data = (IPTR) frame;
	tags[1].ti_Tag = TAG_MORE;
	tags[1].ti_Data = (IPTR) msg->ops_AttrList;
	obj = (Object *) DoSuperMethod(cl, obj, OM_NEW, tags, msg->ops_GInfo);
	if (!obj)
	{
		DisposeObject(frame);
		return NULL;
	}
	data = INST_DATA(cl, obj);
	data->dri = dri;
	data->frame = frame;
	data->table = table;

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
	while ((tag = NextTagItem((const struct TagItem **)&taglist)))
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
			data->table = (struct PartitionTableNode *)tag->ti_Data;
			retval = TRUE;
			break;
		case PTCT_Selected:
			data->selected = (struct PartitionNode *)tag->ti_Data;
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
IPTR retval;

	switch (msg->opg_AttrID)
	{
	default:
		retval = DoSuperMethodA(cl, obj, (Msg)msg);
		break;
	}
	return retval;
}

struct DosEnvec *findSpace
	(
		struct PartitionTableNode *table,
		struct DosEnvec *de,
		ULONG block
	)
{
struct PartitionNode *pn;
ULONG spc;
ULONG first=0;
ULONG last=0xFFFFFFFF;

	/* inherit */
	CopyMem(&table->de, de, sizeof(struct DosEnvec));
	pn = (struct PartitionNode *)table->pl.lh_Head;
	while (pn->ln.ln_Succ)
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
		pn = (struct PartitionNode *)pn->ln.ln_Succ;
	}
	spc = table->de.de_Surfaces*table->de.de_BlocksPerTrack;
	if (first == 0)
		first = table->reserved;
	first /= spc;
	last = ((last+1)/spc)-1;
	if (last>table->de.de_HighCyl)
		last=table->de.de_HighCyl;
	de->de_LowCyl = first;
	de->de_HighCyl = last;
	return de;
}

struct PartitionNode *getSelected(struct PTableData *data) {
struct PartitionNode *pn;

	pn = (struct PartitionNode *)data->table->pl.lh_Head;
	while (pn->ln.ln_Succ)
	{
	ULONG start;
	ULONG end;

		start = pn->de.de_LowCyl*pn->de.de_Surfaces*pn->de.de_BlocksPerTrack;
		end = (pn->de.de_HighCyl+1)*pn->de.de_Surfaces*pn->de.de_BlocksPerTrack;
		if ((data->block>=start) && (data->block<end))
			return pn;
		pn = (struct PartitionNode *)pn->ln.ln_Succ;
	}
	return (struct PartitionNode *)findSpace(data->table,&data->gap,data->block);
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
		struct PartitionTableNode *table,
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
		(table->de_HighCyl+1)*
		table->de_Surfaces*
		table->de_BlocksPerTrack;
#else
	blocks = table->de.de_Surfaces*table->de.de_BlocksPerTrack;
	skipcyl = table->reserved/blocks;
	cyls = table->de.de_HighCyl-skipcyl;
#endif
	spc = pn->de_Surfaces*pn->de_BlocksPerTrack;
#if 0
	start = pn->de_LowCyl*spc*gadget->Width/blocks;
	end = (((pn->de_HighCyl+1)*spc)-1)*gadget->Width/blocks;
#else
	start = pn->de_LowCyl;
	end = pn->de_HighCyl;
	if (
			(pn->de_Surfaces!=table->de.de_Surfaces) ||
			(pn->de_BlocksPerTrack!=table->de.de_BlocksPerTrack)
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
		start, gadget->TopEdge+1,
		end, gadget->TopEdge+gadget->Height-1
	);
}

void ErasePartition
	(
		struct RastPort *rport,
		struct Gadget *gadget,
		struct PartitionTableNode *table,
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
		(table->de_HighCyl+1)*
		table->de_Surfaces*
		table->de_BlocksPerTrack;
#else
	blocks = table->de.de_Surfaces*table->de.de_BlocksPerTrack;
	skipcyl = table->reserved/blocks;
	cyls = table->de.de_HighCyl-skipcyl;
#endif
	spc = pn->de_Surfaces*pn->de_BlocksPerTrack;
#if 0
	start = pn->de_LowCyl*spc*gadget->Width/blocks;
	end = (((pn->de_HighCyl+1)*spc)-1)*gadget->Width/blocks;
#else
	start = pn->de_LowCyl;
	end = pn->de_HighCyl;
	if (
			(pn->de_Surfaces!=table->de.de_Surfaces) ||
			(pn->de_BlocksPerTrack!=table->de.de_BlocksPerTrack)
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
		start, gadget->TopEdge+1,
		end, gadget->TopEdge+gadget->Height-1
	);
}
ULONG getBlock(UWORD mousex, UWORD width, struct PartitionTableNode *table) {
ULONG block;

	block = mousex*table->de.de_HighCyl/width;
	block *= table->de.de_Surfaces*table->de.de_BlocksPerTrack;
	block += table->reserved;
	return block;
}

STATIC IPTR pt_goactive(Class *cl, Object *obj, struct gpInput *msg) {
IPTR retval = GMR_MEACTIVE;
struct PTableData *data;
struct DosEnvec *de;
struct RastPort *rport;

	data = INST_DATA(cl, obj);
	data->block = getBlock(msg->gpi_Mouse.X, G(obj)->Width,data->table);
	rport = msg->gpi_GInfo->gi_RastPort;
	if (data->selected)
	{
		if ((struct DosEnvec *)data->selected == &data->gap)
		{
			SetAPen(rport, 0);
			de = &data->gap;
		}
		else
		{
			SetAPen(rport, 2);
			de = &data->selected->de;
		}
		SetPattern(rport, &pattern, 2);
		DrawPartition(rport, (struct Gadget *)obj, data->table, de);
		SetPattern(rport, 0, 0);
	}
	data->selected = getSelected(data);
	if ((struct DosEnvec *)data->selected == &data->gap)
	{
		SetAPen(rport, 1);
		de = &data->gap;
	}
	else
	{
		SetAPen(rport, 3);
		de = &data->selected->de;
	}
	DrawPartition(rport, (struct Gadget *)obj, data->table, de);
	return retval;
}

STATIC IPTR pt_goinactive(Class *cl, Object *obj, struct gpInput *msg) {
IPTR retval = TRUE;
	return retval;
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
			if ((struct DosEnvec *)data->selected != &data->gap)
			{
			ULONG block;
			LONG diff;
			ULONG start;
			ULONG end;
			ULONG spc;

				spc = data->selected->de.de_Surfaces;
				spc *= data->selected->de.de_BlocksPerTrack;
				block = getBlock(msg->gpi_Mouse.X, G(obj)->Width, data->table);
				diff = block-data->block;
				start = data->selected->de.de_LowCyl*spc;
				start += diff;
				if (validValue(data->table, data->selected, start))
				{
					end = ((data->selected->de.de_HighCyl+1)*spc)-1;
					end += diff;
					if (validValue(data->table, data->selected, end))
					{
						start = start/spc;
						end = ((end+1)/spc)-1;
						ErasePartition
						(
							msg->gpi_GInfo->gi_RastPort,
							(struct Gadget *)obj,
							data->table, &data->selected->de
						);
						data->selected->de.de_LowCyl=start;
						data->selected->de.de_HighCyl=end;
						SetAPen(msg->gpi_GInfo->gi_RastPort, 3);
						DrawPartition
						(
							msg->gpi_GInfo->gi_RastPort,
							(struct Gadget *)obj,
							data->table, &data->selected->de
						);
						data->block = block;
					}
				}
			}
			break;
		case SELECTUP:
			*msg->gpi_Termination &= 0xFFFF0000;
			G(obj)->SpecialInfo = (APTR)data->selected;
			if ((struct DosEnvec *)data->selected == &data->gap)
				*msg->gpi_Termination |= PTS_EMPTY_AREA;
			else
				*msg->gpi_Termination |= PTS_PARTITION;
			retval = GMR_NOREUSE|GMR_VERIFY;
			break;
		}
	}
	return retval;
}


STATIC IPTR pt_render(Class *cl, Object *obj, struct gpRender *msg) {
struct PTableData *data = INST_DATA(cl, obj);
struct PartitionNode *pn;
IPTR retval = 0;

	retval = DoSuperMethodA(cl, obj, (Msg)msg);
	EraseRect
	(
		msg->gpr_RPort,
		G(obj)->LeftEdge,
		G(obj)->TopEdge,
		G(obj)->LeftEdge+G(obj)->Width,
		G(obj)->TopEdge+G(obj)->Height
	);
	DrawBox
	(
		msg->gpr_RPort,
		G(obj)->LeftEdge,
		G(obj)->TopEdge,
		G(obj)->LeftEdge+G(obj)->Width,
		G(obj)->TopEdge+G(obj)->Height
	);
#if 0
	RectFill
	(
		msg->gpr_RPort,
		G(obj)->LeftEdge,
		G(obj)->TopEdge,
		G(obj)->LeftEdge+G(obj)->Width,
		G(obj)->TopEdge+G(obj)->Height
	);
#endif
	if (data->table)
	{
		SetPattern(msg->gpr_RPort, &pattern, 2);
		pn = (struct PartitionNode *)data->table->pl.lh_Head;
		while (pn->ln.ln_Succ)
		{
			if (data->selected==pn)
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
			pn = (struct PartitionNode *)pn->ln.ln_Succ;
		}
		if ((struct DosEnvec *)data->selected == &data->gap)
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
