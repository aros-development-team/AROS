#include <stdio.h>
#include <strings.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/intuition.h>

#include "details.h"
#include "gadgets.h"
#include "hdtoolbox_support.h"
#include "partitiontypes.h"
#include "platform.h"

#define DEBUG 1
#include "debug.h"

extern struct TagItem dettypelvtags[], dettypestringtags[],
	detpartitiontabletags[], detbufferstags[], detmasktags[],
	detmaxtransfertags[], detautomounttags[], detcustboottags[],
	detbegintags[], detendtags[], detblocksizetags[], detcustbbtags[];
extern struct List pctypelist;
extern struct List rdbtypelist;
extern struct creategadget detailsgadgets[];

struct PartitionType backup_type;
struct DosEnvec backup_de;
ULONG backup_flags;

void setTypeGadgets(struct Window *win, struct PartitionNode *pn) {
char str[32];
struct PartitionTypeNode *ptypenode;

	dettypelvtags[1].ti_Data = (STACKIPTR)pn->root->typelist;
	if (pn->root->typelist)
	{
		dettypelvtags[0].ti_Data = FALSE;
		ptypenode = getPartitionTypeNode(pn->root, &pn->type);
		if (ptypenode->ln.ln_Succ)
		{
			dettypelvtags[2].ti_Data = getNodeNum(&ptypenode->ln);
			dettypelvtags[3].ti_Data = dettypelvtags[2].ti_Data;
		}
		else
		{
			dettypelvtags[3].ti_Data = ~0;
		}
		if (pn->type.id_len)
			typestrncpy(str, pn->type.id, pn->type.id_len);
		else
			*str = 0;
		dettypestringtags[1].ti_Data = (STACKIPTR)str;
	}
	else
	{
		*str = 0;
		dettypelvtags[0].ti_Data = TRUE;
		dettypestringtags[1].ti_Data = (STACKIPTR)str;
	}
	GT_SetGadgetAttrsA
	(
		detailsgadgets[ID_DET_TYPELV-ID_DET_FIRST_GADGET].gadget,
		win,0,dettypelvtags
	);
	GT_SetGadgetAttrsA
	(
		detailsgadgets[ID_DET_TYPESTRING-ID_DET_FIRST_GADGET].gadget,
		win,0,dettypestringtags
	);
}

void setDetGadgets(struct Window *win, struct PartitionNode *pn) {
char mask[16];
char maxt[16];

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
	if (existsAttr(pn->root->pattrlist, PTA_DOSENVEC))
	{

		detblocksizetags[0].ti_Data = FALSE;
		detblocksizetags[1].ti_Data = getBitNum(pn->de.de_SizeBlock>>7);
		detbufferstags[0].ti_Data = FALSE;
		detbufferstags[1].ti_Data = pn->de.de_NumBuffers;
		detmasktags[0].ti_Data = FALSE;
		sprintf(mask, "0x%08lx", pn->de.de_Mask);
		detmasktags[1].ti_Data = (STACKIPTR)mask;
		detmaxtransfertags[0].ti_Data = FALSE;
		sprintf(maxt, "0x%08lx", pn->de.de_MaxTransfer);
		detmaxtransfertags[1].ti_Data = (STACKIPTR)maxt;
		detautomounttags[0].ti_Data = FALSE;
		detautomounttags[1].ti_Data = pn->flags & PNF_AUTOMOUNT ? TRUE : FALSE;
		detcustboottags[0].ti_Data = FALSE;
		detcustboottags[1].ti_Data = pn->de.de_TableSize>=(DE_BOOTBLOCKS+1) ? TRUE : FALSE;
		detcustbbtags[0].ti_Data = !detcustboottags[1].ti_Data;
		detcustbbtags[1].ti_Data = detcustboottags[1].ti_Data ? pn->de.de_BootBlocks : 0;
		detbegintags[0].ti_Data = FALSE;
		detbegintags[1].ti_Data = pn->de.de_Reserved;
		detendtags[0].ti_Data = FALSE;
		detendtags[1].ti_Data = pn->de.de_PreAlloc;
	}
	else
	{
		detblocksizetags[0].ti_Data = TRUE;
		detbufferstags[0].ti_Data = TRUE;
		detmasktags[0].ti_Data = TRUE;
		detmaxtransfertags[0].ti_Data = TRUE;
		detautomounttags[0].ti_Data = TRUE;
		detcustboottags[0].ti_Data = TRUE;
		detcustbbtags[0].ti_Data = TRUE;
		detbegintags[0].ti_Data = TRUE;
		detendtags[0].ti_Data = TRUE;
	}
	setTypeGadgets(win, pn);
	GT_SetGadgetAttrsA
	(
		detailsgadgets[ID_DET_PARTITION_TABLE-ID_DET_FIRST_GADGET].gadget,
		win,0,detpartitiontabletags
	);
	GT_SetGadgetAttrsA
	(
		detailsgadgets[ID_DET_BLOCKSIZE-ID_DET_FIRST_GADGET].gadget,
		win,0,detblocksizetags
	);
	GT_SetGadgetAttrsA
	(
		detailsgadgets[ID_DET_BUFFERS-ID_DET_FIRST_GADGET].gadget,
		win,0,detbufferstags
	);
	GT_SetGadgetAttrsA
	(
		detailsgadgets[ID_DET_MASK-ID_DET_FIRST_GADGET].gadget,
		win,0,detmasktags
	);
	GT_SetGadgetAttrsA
	(
		detailsgadgets[ID_DET_MAX_TRANSFER-ID_DET_FIRST_GADGET].gadget,
		win,0,detmaxtransfertags
	);
	GT_SetGadgetAttrsA
	(
		detailsgadgets[ID_DET_AUTOMOUNT-ID_DET_FIRST_GADGET].gadget,
		win,0,detautomounttags
	);
	GT_SetGadgetAttrsA
	(
		detailsgadgets[ID_DET_CUSTBOOT-ID_DET_FIRST_GADGET].gadget,
		win,0,detcustboottags
	);
	GT_SetGadgetAttrsA
	(
		detailsgadgets[ID_DET_CUSTBB-ID_DET_FIRST_GADGET].gadget,
		win,0,detcustbbtags
	);
	GT_SetGadgetAttrsA
	(
		detailsgadgets[ID_DET_BEGINING-ID_DET_FIRST_GADGET].gadget,
		win,0,detbegintags
	);
	GT_SetGadgetAttrsA
	(
		detailsgadgets[ID_DET_END-ID_DET_FIRST_GADGET].gadget,
		win,0,detendtags
	);
}

void det_Init(struct Window *win, struct PartitionNode *pn) {
	CopyMem(&pn->type, &backup_type, sizeof(struct PartitionType));
	CopyMem(&pn->de, &backup_de, sizeof(struct DosEnvec));
	backup_flags = pn->flags & (PNF_AUTOMOUNT);
	setDetGadgets(win, pn);
}

void changeType
	(
		struct Window *win,
		struct PartitionNode *pn,
		UBYTE *id,
		UWORD id_len
	)
{
	strncpy(pn->type.id, id, id_len);
	pn->type.id_len = id_len;
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
			(pn->type.id_len != backup_type.id_len) ||
			(strncmp(pn->type.id, backup_type.id, backup_type.id_len)) ||
			(pn->flags &
				(
					PNF_NEW_TABLE |
					PNF_DEL_TABLE |
					PNF_DE_CHANGED |
					PNF_FLAGS_CHANGED
				)
			)
		)
		pn->flags |= PNF_CHANGED;
}

void det_Cancel(struct PartitionNode *pn) {

	CopyMem(&backup_type, &pn->type, sizeof(struct PartitionType));
	CopyMem(&backup_de, &pn->de, sizeof(struct DosEnvec));
	if (pn->flags & PNF_FLAGS_CHANGED)
	{
		if (backup_flags & PNF_AUTOMOUNT)
			pn->flags |= PNF_AUTOMOUNT;
		else
			pn->flags &= ~PNF_AUTOMOUNT;
	}
	pn->flags &=
		~(
			PNF_NEW_TABLE |
			PNF_DEL_TABLE |
			PNF_DE_CHANGED |
			PNF_FLAGS_CHANGED
		);
}

