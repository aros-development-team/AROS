#include <stdio.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include "details.h"
#include "gadgets.h"
#include "hdtoolbox_support.h"

#define DEBUG 1
#include <aros/debug.h>

extern struct TagItem dettypelvtags[], dettypestringtags[],
	detpartitiontabletags[];
extern struct List pctypelist;
extern struct List rdbtypelist;
extern struct creategadget detailsgadgets[];

ULONG backup_type;

void setTypeGadgets(struct Window *win, struct PartitionNode *pn) {
char str[8];
struct Node *node;

	if (existsAttr(pn->root->pattrlist, PTA_TYPE))
	{
		switch (pn->root->type)
		{
		case PHPTT_MBR:
			if (pn->type > 255)
				pn->type = dettypelvtags[2].ti_Data;
			dettypelvtags[1].ti_Data = (ULONG)&pctypelist;
			dettypelvtags[2].ti_Data = pn->type;
			dettypelvtags[3].ti_Data = pn->type;
			sprintf(str,"\\%ld",pn->type);
			dettypestringtags[1].ti_Data = (ULONG)str;
			break;
		case PHPTT_RDB:
			dettypelvtags[1].ti_Data = (ULONG)&rdbtypelist;
			CopyMem(&pn->type, str, 4);
			str[4] = 0;
			node = FindName(&rdbtypelist, str);
			if (node)
			{
				dettypelvtags[2].ti_Data = getNodeNum(node);
				dettypelvtags[3].ti_Data = dettypelvtags[2].ti_Data;
			}
			else
			{
				dettypelvtags[0].ti_Data = TRUE;
			}
			typestrncpy(str, (STRPTR)&pn->type,4);
			dettypestringtags[1].ti_Data = (ULONG)str;
			break;
		default:
			dettypelvtags[0].ti_Data = TRUE;
			dettypestringtags[0].ti_Data = TRUE;
			*str = 0;
			dettypestringtags[1].ti_Data = (ULONG)str;
		}
	}
	else
	{
		*str = 0;
		dettypelvtags[0].ti_Data = TRUE;
		dettypestringtags[0].ti_Data = TRUE;
		dettypestringtags[1].ti_Data = (ULONG)str;
	}
	SetGadgetAttrsA
	(
		detailsgadgets[ID_DET_TYPELV-ID_DET_FIRST_GADGET].gadget,
		win,0,dettypelvtags
	);
	SetGadgetAttrsA
	(
		detailsgadgets[ID_DET_TYPESTRING-ID_DET_FIRST_GADGET].gadget,
		win,0,dettypestringtags
	);
}

void setDetGadgets(struct Window *win, struct PartitionNode *pn) {

	if (pn->flags & PNF_TABLE)
	{
		detpartitiontabletags[1].ti_Data=TRUE;
		dettypelvtags[0].ti_Data = TRUE;
		dettypestringtags[0].ti_Data = TRUE;
	}
	else
	{
		detpartitiontabletags[1].ti_Data=FALSE;
		dettypelvtags[0].ti_Data = FALSE;
		dettypestringtags[0].ti_Data = FALSE;
	}
	setTypeGadgets(win, pn);
	SetGadgetAttrsA
	(
		detailsgadgets[ID_DET_PARTITION_TABLE-ID_DET_FIRST_GADGET].gadget,
		win,0,detpartitiontabletags
	);
}

void det_Init(struct Window *win, struct PartitionNode *pn) {
	backup_type = pn->type;
	setDetGadgets(win, pn);
}

void changeType
	(
		struct Window *win,
		struct PartitionNode *pn,
		ULONG value
	)
{
	pn->type = value;
	setTypeGadgets(win, pn);
}

void setPartitionTable(struct Window *win, struct PartitionNode *pn, BOOL f) {

	if (f)
	{
		if (pn->flags & PNF_DEL_TABLE)
			pn->flags &= ~PNF_DEL_TABLE;
		else
			pn->flags |= PNF_NEW_TABLE;
		pn->flags |= PNF_TABLE;
	}
	else
	{
		if (pn->flags & PNF_NEW_TABLE)
			pn->flags &= ~PNF_NEW_TABLE;
		else
		{
			pn->flags |= PNF_DEL_TABLE;
		}
		pn->flags &= ~PNF_TABLE;
	}
	setDetGadgets(win, pn);
}

void det_Ok(struct PartitionNode *pn) {

	if (
			(pn->type != backup_type) ||
			(pn->flags & (PNF_NEW_TABLE | PNF_DEL_TABLE))
		)
		pn->flags |= PNF_CHANGED;
}

void det_Cancel(struct PartitionNode *pn) {

	pn->type = backup_type;
	pn->flags &= ~(PNF_NEW_TABLE | PNF_DEL_TABLE);
}

