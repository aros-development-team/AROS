/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#warning "TODO: fs support"
//#error "Continue: clean (disable other windows) + check mount"

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <exec/memory.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <intuition/intuition.h>
#include <intuition/sghooks.h>
#include <libraries/locale.h>
#include <libraries/mui.h>
#ifdef HAVE_COOLIMAGES
#include <libraries/coolimages.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#define DEBUG 1
#include "debug.h"

#include "gui.h"
#include "devices.h"
#include "error.h"
#include "harddisks.h"
#include "hdtoolbox_support.h"
#include "locale.h"
#include "platform.h"
#include "ptclass.h"
#include "prefs.h"

#undef CATCOMP_STRINGS
#undef CATCOMP_NUMBERS
#define CATCOMP_NUMBERS
#include "strings.h"

#define SIMPLEBUTTON(text) SimpleButton(MSG(text))

#ifdef HAVE_COOLIMAGES
#define IMAGEBUTTON(text,imgid) CoolImageIDButton(MSG(text),imgid)
#else
#define IMAGEBUTTON(text,imgid) SIMPLEBUTTON(text)
#endif

struct Library *MUIMasterBase=NULL;

Object *app;
Object *mainwin;
Object *about_item;
Object *quit_item;
struct GUIGadgets gadgets;

struct Hook hook_display;
struct Hook hook_buttons;
struct Hook hook_lv_doubleclick;
struct Hook hook_lv_click;

struct AddDeviceGadgets {
	Object *win;
	Object *disk;
	Object *file;
	Object *ok;
	Object *cancel;
} adddevicegadgets;

Class *ptclass;

struct AddPartitionGadgets {
	Object *win;
	Object *pt;
	Object *ok;
	Object *cancel;
} addpartitiongadgets;

struct PartitionTypeGadgets {
	Object *win;
	struct TableTypeNode *ttn;
	Object *lv;
	Object *base;
	struct Hook hook_hexidedit;
	Object *hexid;
	struct PartitionType type;
	struct ListNode *iln;
	Object *ok;
	Object *cancel;
} partitiontypegadgets;

struct PartitionTableTypeGadgets {
	Object *win;
	Object *lv;
	struct ListNode *iln;
	Object *ok;
	Object *cancel;
} partitiontabletypegadgets;

struct ResizeMoveGadgets {
	Object *win;
	Object *pt;
	Object *lowcyl;
	Object *highcyl;
	Object *totalcyl;
	Object *size;
	Object *ok;
	Object *cancel;
} resizemovegadgets;

struct RenameGadgets {
	Object *win;
	Object *name;
	struct ListNode *iln;
	Object *ok;
	Object *cancel;
} renamegadgets;

struct DosEnvecGadgets {
	Object *win;
	Object *mask;
	Object *maxtransfer;
	Object *custboot;
	Object *numcustboot;
	Object *reservedblocksstart;
	Object *reservedblocksend;
	Object *blocksize;
	Object *buffers;
	struct ListNode *iln;
	Object *ok;
	Object *cancel;
} dosenvecgadgets;

struct MountBootGadgets {
	Object *win;
	Object *active;
	Object *automount;
	Object *bootable;
	Object *bootpri;
	struct ListNode *iln;
	Object *ok;
	Object *cancel;
} mountbootgadgets;

char *editcycleentries[]={"0x", NULL};
char *blocksizecycleentries[]={"512","1024","2048","4096", NULL};

void setChanged(struct ListNode *iln) {
struct ListNode *parent = iln;

	while (parent)
	{
		parent->change_count++;
		parent = parent->parent;
	}
	DoMethod(gadgets.leftlv, MUIM_List_Redraw, MUIV_List_Redraw_All);
	set(gadgets.leftlv, MUIA_Listview_SelectChange, TRUE);
}

void unsetChanged(struct ListNode *iln) {
struct ListNode *sub = iln;
struct ListNode *parent;

	/* remove changed from all first generation child partitions */
	sub = (struct ListNode *)iln->list.lh_Head;
	while (sub->ln.ln_Succ)
	{
		if (sub->change_count)
		{
			parent = sub->parent;
			while (parent)
			{
				parent->change_count -= sub->change_count;
				parent = parent->parent;
			}
			sub->change_count = 0;
		}
		sub = (struct ListNode *)sub->ln.ln_Succ;
	}
	if (iln->flags & LNF_ToSave)
	{
		iln->flags &= ~LNF_ToSave;
		/* we saved it so there can't be a change_count */
		if (iln->change_count)
		{
			parent = iln->parent;
			while (parent)
			{
				parent->change_count -= iln->change_count;
				parent = parent->parent;
			}
			iln->change_count = 0;
		}
	}
	DoMethod(gadgets.leftlv, MUIM_List_Redraw, MUIV_List_Redraw_Active);
}

AROS_UFH3(void, hexidedit_function,
	AROS_UFHA(struct Hook *, h, A0),
	AROS_UFHA(struct SGWork *, sgwork, A2),
	AROS_UFHA(ULONG *, msg, A1))
{
    AROS_USERFUNC_INIT

kprintf("key press\n");
	if (*msg == SGH_KEY)
	{
kprintf("key press\n");
	}

    AROS_USERFUNC_EXIT
}

AROS_UFH3(void, display_function,
	AROS_UFHA(struct Hook *, h, A0),
	AROS_UFHA(char **, strings, A2),
	AROS_UFHA(struct ListNode *, entry, A1))
{
    AROS_USERFUNC_INIT

static char buf[16];
static char bu2[64];

	if (entry)
	{
		int i=0;
		if (entry->change_count)
			buf[i++] = '*';
		if (entry->flags & LNF_ToSave)
			buf[i++] = 'S';
		buf[i] = 0;
		strings[0] = buf;
		if ((entry->flags & LNF_Listable) || (entry->ln.ln_Type == LNT_Parent))
		{
			sprintf(bu2,"\0333%s",entry->ln.ln_Name);
      	strings[1] = bu2;
		}
		else
			strings[1] = entry->ln.ln_Name;
	}
	else
   {
		strings[0] = MSG(WORD_Changed);
		strings[1] = MSG(WORD_Name);
	}

    AROS_USERFUNC_EXIT
}

void setTypeString(struct PartitionType *type, Object *strgad) {
char str[256];
char *cur=str;
LONG len=0;

	while (len!=type->id_len)
	{
		sprintf(cur, "%02x ", type->id[len]);
		cur += 3;
		len++;
	}
	cur[-1]=0;
	set(strgad,  MUIA_String_Contents, str);
}

LONG askSave(STRPTR name) {
struct EasyStruct es =
{
	sizeof(struct EasyStruct), 0,
	"HDToolBox",
	MSG(MSG_ASK_SAVE),
	NULL
};
char yesno[32];

	sprintf(yesno, "%s|%s|%s", MSG_STD(YESSTR), MSG(WORD_All), MSG_STD(NOSTR));
	es.es_GadgetFormat = yesno;
	return EasyRequestArgs(0, &es, 0, &name);
}


LONG saveChanges(struct ListNode *iln, LONG asksave) {
struct ListNode *sub;

	if (iln->change_count)
	{
		sub = (struct ListNode *)iln->list.lh_Head;
		while (sub->ln.ln_Succ)
		{
			asksave = saveChanges(sub, asksave);
			sub = (struct ListNode *)sub->ln.ln_Succ;
		}
		if (iln->flags & LNF_ToSave)
		{
			struct HDTBPartition *partition;
			partition = (struct HDTBPartition *)iln;
			if (partition->table)
			{
				LONG result=1;
				if (asksave)
					result = askSave(iln->ln.ln_Name);
				if (result == 2)
					asksave = FALSE;
				if (result)
				{
					if (WritePartitionTable(partition->ph) == 0)
						unsetChanged(iln);
					else
						set(gadgets.text, MUIA_Text_Contents, MSG(MSG_WriteTableError));
				}
			}
		}
	}
	return asksave;
}

AROS_UFH3(void, buttons_function,
	AROS_UFHA(struct Hook *, h, A0),
	AROS_UFHA(Object *, object, A2),
	AROS_UFHA(ULONG *, arg, A1))
{
    AROS_USERFUNC_INIT

LONG active;
struct ListNode *iln;
int i;

	for (i=GB_FIRST;i<=GB_LAST;i++)
		if (object == gadgets.buttons[i])
			break;
	if (i > GB_LAST)
	{
		if (object == gadgets.leftlv)
		{
		}
		else if (object == adddevicegadgets.ok)
		{
			char *str;
			struct HDTBDevice *dn;
			get(adddevicegadgets.file, MUIA_String_Contents, &str);
			if (str[0])
				dn = addDeviceName(str);
		}
		else if (object == addpartitiongadgets.ok)
		{
			struct DosEnvec *de;
			get(addpartitiongadgets.pt, PTCT_ActivePartition, &de);
			if (de)
			{
				struct HDTBPartition *table;
				struct HDTBPartition *partition;
				get(addpartitiongadgets.pt, PTCT_PartitionTable, &iln);
				table = (struct HDTBPartition *)iln;
				partition = addPartition(table, de);
				if (partition)
				{
					setChanged(&partition->listnode);
					partition->listnode.parent->flags |= LNF_ToSave;
					InsertList(gadgets.leftlv, &partition->listnode);
				}
				else
					set(gadgets.text, MUIA_Text_Contents, MSG(MSG_NO_MEMORY));
			}
		}
		else if (object == addpartitiongadgets.pt);
		else if (object == partitiontypegadgets.hexid)
		{
			char *str;
			struct HDTBPartition *partition;
			partition = (struct HDTBPartition *)partitiontypegadgets.iln;
			DoMethod(gadgets.leftlv, MUIM_List_GetEntry, active, (IPTR)&iln);
			get(object, MUIA_String_Contents, &str);
			partitiontypegadgets.type.id_len=strcpyESC(partitiontypegadgets.type.id, str);
		}
		else if (object == partitiontypegadgets.lv)
		{
			get(partitiontypegadgets.lv, MUIA_List_Active, &active);
			if (active != MUIV_List_Active_Off)
			{
				struct TypeNode *tn;
				STRPTR name;
				DoMethod(partitiontypegadgets.lv, MUIM_List_GetEntry, active, (IPTR)&name);
				tn = (struct TypeNode *)FindName(&partitiontypegadgets.ttn->typelist, name);
				setTypeString(&tn->type, partitiontypegadgets.hexid);
				CopyMem(&tn->type, &partitiontypegadgets.type, sizeof(struct PartitionType));
			}
		}
		else if (object == partitiontypegadgets.ok)
		{
			struct HDTBPartition *partition;
			partition = (struct HDTBPartition *)partitiontypegadgets.iln;
			CopyMem(&partitiontypegadgets.type,  &partition->type, sizeof(struct PartitionType));
			SetPartitionAttrsA(partition->ph, PT_TYPE, &partition->type, TAG_DONE);
			partitiontypegadgets.iln->parent->flags |= LNF_ToSave;
			setChanged(partitiontypegadgets.iln);
		}
		else if (object == partitiontabletypegadgets.ok)
		{
			get(partitiontabletypegadgets.lv, MUIA_List_Active, &active);
			if (active != MUIV_List_Active_Off)
			{
				struct TableTypeNode *ttn;
				struct HDTBPartition *table;
				STRPTR name;
				DoMethod(partitiontabletypegadgets.lv, MUIM_List_GetEntry, active, (IPTR)&name);
				ttn = findTableTypeNodeName(name);
				table = (struct HDTBPartition *)partitiontabletypegadgets.iln;
				if (
						(table->table == NULL) ||
						(ttn->pti->pti_Type != table->table->type)
					)
				{
					if (makePartitionTable(table, ttn->pti->pti_Type))
					{
						table->listnode.flags |= LNF_Listable;
						table->listnode.flags |= LNF_ToSave;
						setChanged(partitiontabletypegadgets.iln); 
					}
					else
						set(gadgets.text, MUIA_Text_Contents, MSG(MSG_CreateTableError));
				}
			}
		}
		else if (object == resizemovegadgets.ok)
		{
			struct HDTBPartition *table;
			struct HDTBPartition *partition;
			get(resizemovegadgets.pt, PTCT_PartitionTable, &table);
			partition = (struct HDTBPartition *)table->listnode.list.lh_Head;
			table->listnode.flags |= LNF_ToSave;
			while (partition->listnode.ln.ln_Succ)
			{
				if (partition->listnode.flags & LNF_IntermedChange)
				{
					partition->listnode.flags &= ~LNF_IntermedChange;
					setChanged(&partition->listnode);
					SetPartitionAttrsA(partition->ph, PT_DOSENVEC, &partition->de, TAG_DONE);
				}
				partition = (struct HDTBPartition *)partition->listnode.ln.ln_Succ;
			}
		}
		else if (object == resizemovegadgets.cancel)
		{
			struct HDTBPartition *table;
			struct HDTBPartition *partition;
			get(resizemovegadgets.pt, PTCT_PartitionTable, &table);
			partition = (struct HDTBPartition *)table->listnode.list.lh_Head;
			while (partition->listnode.ln.ln_Succ)
			{
				if (partition->listnode.flags & LNF_IntermedChange)
				{
					partition->listnode.flags &= ~LNF_IntermedChange;
					GetPartitionAttrsA(partition->ph, PT_DOSENVEC, &partition->de, TAG_DONE);
				}
				partition = (struct HDTBPartition *)partition->listnode.ln.ln_Succ;
			}
		}
		else if (object == resizemovegadgets.pt)
		{
			LONG type;
			struct DosEnvec *de;
			char str[32];
			get(resizemovegadgets.pt, PTCT_ActiveType, &type);
			if (type == PTS_EMPTY_AREA)
			{
				get(resizemovegadgets.pt, PTCT_ActivePartition, &de);
			}
			else
			{
				struct HDTBPartition *partition;
				get(resizemovegadgets.pt, PTCT_ActivePartition, &partition);
				de = &partition->de;
				if (*arg == PTCT_PartitionMove)
				{
					partition->listnode.flags |= LNF_IntermedChange;
				}
			}
			set(resizemovegadgets.lowcyl, MUIA_String_Integer, de->de_LowCyl);
			set(resizemovegadgets.highcyl, MUIA_String_Integer, de->de_HighCyl);
			set(resizemovegadgets.totalcyl, MUIA_String_Integer, de->de_HighCyl-de->de_LowCyl+1);
			getSizeStr
			(
				str,
				(
					(
						(de->de_HighCyl-de->de_LowCyl+1)*
						de->de_Surfaces*de->de_BlocksPerTrack
					)-1
				)/2
			);
			set(resizemovegadgets.size, MUIA_String_Contents, str);
		}
		else if (object == resizemovegadgets.lowcyl)
		{
			LONG type;
			ULONG value;
			get(object, MUIA_String_Integer, &value);
			get(resizemovegadgets.pt, PTCT_ActiveType, &type);
			if (type == PTS_PARTITION)
			{
				struct HDTBPartition *table;
				struct HDTBPartition *partition;
				ULONG block;
				get(resizemovegadgets.pt, PTCT_PartitionTable, &table);
				get(resizemovegadgets.pt, PTCT_ActivePartition, &partition);
				if (value != partition->de.de_LowCyl)
				{
					block =
						value*partition->de.de_Surfaces*partition->de.de_BlocksPerTrack;
					if (validValue(table, partition, block))
					{
						char str[32];
						partition->listnode.flags |= LNF_IntermedChange;
						partition->de.de_LowCyl = value;
						set(resizemovegadgets.totalcyl, MUIA_String_Integer, partition->de.de_HighCyl-partition->de.de_LowCyl+1);
						set(resizemovegadgets.pt, PTCT_PartitionTable, table);
						getSizeStr
						(
							str,
							(
								(
									(partition->de.de_HighCyl-partition->de.de_LowCyl+1)*
									partition->de.de_Surfaces*partition->de.de_BlocksPerTrack
								)-1
							)/2
						);
						set(resizemovegadgets.size, MUIA_String_Contents, str);
					}
					else
						set(object, MUIA_String_Integer, partition->de.de_LowCyl);
				}
			}
			else if (type == PTS_EMPTY_AREA)
			{
				struct DosEnvec *de;
				get(resizemovegadgets.pt, PTCT_ActivePartition, &de);
				if (value != de->de_LowCyl)
					set(object, MUIA_String_Integer, de->de_LowCyl);
			}
			else
				set(object, MUIA_String_Integer, 0);
		}
		else if (object == resizemovegadgets.highcyl)
		{
			LONG type;
			ULONG value;
			get(object, MUIA_String_Integer, &value);
			get(resizemovegadgets.pt, PTCT_ActiveType, &type);
			if (type == PTS_PARTITION)
			{
				struct HDTBPartition *table;
				struct HDTBPartition *partition;
				ULONG block;
				get(resizemovegadgets.pt, PTCT_PartitionTable, &table);
				get(resizemovegadgets.pt, PTCT_ActivePartition, &partition);
				if (value != partition->de.de_HighCyl)
				{
					block =
						(
							(value+1)*
							partition->de.de_Surfaces*partition->de.de_BlocksPerTrack
						)-1;
					if (validValue(table, partition, block))
					{
						char str[32];
						partition->listnode.flags |= LNF_IntermedChange;
						partition->de.de_HighCyl = value;
						set(resizemovegadgets.totalcyl, MUIA_String_Integer, partition->de.de_HighCyl-partition->de.de_LowCyl+1);
						set(resizemovegadgets.pt, PTCT_PartitionTable, table);
						getSizeStr
						(
							str,
							(
								(
									(partition->de.de_HighCyl-partition->de.de_LowCyl+1)*
									partition->de.de_Surfaces*partition->de.de_BlocksPerTrack
								)-1
							)/2
						);
						set(resizemovegadgets.size, MUIA_String_Contents, str);
					}
					else
						set(object, MUIA_String_Integer, partition->de.de_HighCyl);
				}
			}
			else if (type == PTS_EMPTY_AREA)
			{
				struct DosEnvec *de;
				get(resizemovegadgets.pt, PTCT_ActivePartition, &de);
				if (value != de->de_HighCyl)
					set(object, MUIA_String_Integer, de->de_HighCyl);
			}
			else
				set(object, MUIA_String_Integer, 0);
		}
		else if (object == resizemovegadgets.totalcyl)
		{
			LONG type;
			ULONG value;
			get(object, MUIA_String_Integer, &value);
			get(resizemovegadgets.pt, PTCT_ActiveType, &type);
			if (type == PTS_PARTITION)
			{
				struct HDTBPartition *table;
				struct HDTBPartition *partition;
				ULONG block;
				get(resizemovegadgets.pt, PTCT_PartitionTable, &table);
				get(resizemovegadgets.pt, PTCT_ActivePartition, &partition);
				if (value != (partition->de.de_HighCyl-partition->de.de_LowCyl+1))
				{
					block =
						(
							(partition->de.de_LowCyl+value)*
							partition->de.de_Surfaces*partition->de.de_BlocksPerTrack
						)-1;
					if (validValue(table, partition, block))
					{
						char str[32];
						partition->listnode.flags |= LNF_IntermedChange;
						partition->de.de_HighCyl = partition->de.de_LowCyl+value-1;
						set(resizemovegadgets.highcyl, MUIA_String_Integer, partition->de.de_HighCyl);
						set(resizemovegadgets.pt, PTCT_PartitionTable, table);
						getSizeStr
						(
							str,
							(
								(
									(partition->de.de_HighCyl-partition->de.de_LowCyl+1)*
									partition->de.de_Surfaces*partition->de.de_BlocksPerTrack
								)-1
							)/2
						);
						set(resizemovegadgets.size, MUIA_String_Contents, str);
					}
					else
						set(object, MUIA_String_Integer, partition->de.de_HighCyl-partition->de.de_LowCyl+1);
				}
			}
			else if (type == PTS_EMPTY_AREA)
			{
				struct DosEnvec *de;
				get(resizemovegadgets.pt, PTCT_ActivePartition, &de);
				if (value != (de->de_HighCyl-de->de_LowCyl+1))
					set(object, MUIA_String_Integer, de->de_HighCyl-de->de_LowCyl+1);
			}
			else
				set(object, MUIA_String_Integer, 0);
		}
		else if (object == resizemovegadgets.size)
		{
			char val[32];
			struct DosEnvec *de;
			STRPTR *str;
			ULONG size = 0;
			ULONG type;
			get(resizemovegadgets.pt, PTCT_ActiveType, &type);
			if (type == PTS_PARTITION)
			{
				struct HDTBPartition *partition;
				get(resizemovegadgets.pt, PTCT_ActivePartition, &partition);
				de = &partition->de;
				get(object, MUIA_String_Contents, &str);
				size = sizeStrToUL((STRPTR)str);
				size = (size*2+1)/partition->de.de_BlocksPerTrack/partition->de.de_Surfaces+1;
			}
			else if (type == PTS_EMPTY_AREA)
			{
				get(resizemovegadgets.pt, PTCT_ActivePartition, &de);
			}
			set(resizemovegadgets.totalcyl, MUIA_String_Integer, size);
			set(resizemovegadgets.totalcyl, MUIA_String_Acknowledge, TRUE);
			getSizeStr
			(
				val,
				(
					(
						(de->de_HighCyl-de->de_LowCyl+1)*
						de->de_Surfaces*de->de_BlocksPerTrack
					)-1
				)/2
			);
			set(resizemovegadgets.size, MUIA_String_Contents, val);
		}
		else if (object == renamegadgets.ok)
		{
			struct HDTBPartition *partition;
			STRPTR name;
			partition = (struct HDTBPartition *)renamegadgets.iln;
			get(renamegadgets.name, MUIA_String_Contents, &name);
			if (strcmp(name, partition->listnode.ln.ln_Name) != 0)
			{
				SetPartitionAttrsA(partition->ph, PT_NAME, name, TAG_DONE);
				strcpy(partition->listnode.ln.ln_Name, name);
				renamegadgets.iln->parent->flags |= LNF_ToSave;
				setChanged(renamegadgets.iln);
			}
		}
		else if (object == dosenvecgadgets.ok)
		{
			BOOL changed = FALSE;
			LONG check;
			ULONG value;
			STRPTR str;
			char *end;
			struct HDTBPartition *partition;
			partition = (struct HDTBPartition *)dosenvecgadgets.iln;
			get(dosenvecgadgets.mask, MUIA_String_Integer, &value);
			if (value != partition->de.de_Mask)
			{
				partition->de.de_Mask = value;
				changed = TRUE;
			}
			get(dosenvecgadgets.maxtransfer, MUIA_String_Contents, &str);
			value = strtoul(str, &end, NULL);
			if (*end == 0)
			{
				if (value != partition->de.de_MaxTransfer)
				{
					partition->de.de_MaxTransfer = value;
					changed = TRUE;
				}
			}
			get(dosenvecgadgets.custboot, MUIA_Selected, &check);
			if (check)
			{ 
				if (partition->de.de_TableSize<(DE_BOOTBLOCKS+1))
				{
					partition->de.de_TableSize = DE_BOOTBLOCKS+1;
					changed = TRUE;
				}
				get(dosenvecgadgets.numcustboot, MUIA_String_Integer, &value);
				if (value != partition->de.de_BootBlocks)
				{
					partition->de.de_BootBlocks = value;
					changed = TRUE;
				}
			}
			else
			{
				if (partition->de.de_TableSize>=(DE_BOOTBLOCKS+1))
				{
					partition->de.de_TableSize = DE_BOOTBLOCKS;
					partition->de.de_BootBlocks = 0;
					changed = TRUE;
				}
			}
			get(dosenvecgadgets.reservedblocksstart, MUIA_String_Integer, &value);
			if (value != partition->de.de_Reserved)
			{
				partition->de.de_Reserved = value;
				changed = TRUE;
			}
			get(dosenvecgadgets.reservedblocksend, MUIA_String_Integer, &value);
			if (value != partition->de.de_PreAlloc)
			{
				partition->de.de_PreAlloc = value;
				changed = TRUE;
			}
			get(dosenvecgadgets.blocksize, MUIA_Cycle_Active, &value);
			value = 1<<(value+7);
			if (value != partition->de.de_SizeBlock)
			{
				partition->de.de_SizeBlock = value;
				changed = TRUE;
			}
			get(dosenvecgadgets.buffers, MUIA_String_Integer, &value);
			if (value != partition->de.de_NumBuffers)
			{
				partition->de.de_NumBuffers = value;
				changed = TRUE;
			}
			if (changed)
			{
				SetPartitionAttrsA(partition->ph, PT_DOSENVEC, &partition->de, TAG_DONE);
				dosenvecgadgets.iln->parent->flags |= LNF_ToSave;
				setChanged(dosenvecgadgets.iln);
			}
		}
		else if (object == mountbootgadgets.ok)
		{
			struct HDTBPartition *partition;
			BOOL changeda = FALSE;
			BOOL changed = FALSE;
			LONG check;
			ULONG value;
			partition = (struct HDTBPartition *)mountbootgadgets.iln;
			get(mountbootgadgets.active, MUIA_Selected, &check);
			if (check)
			{
				if (!(partition->flags & PNF_ACTIVE))
				{
					partition->flags |= PNF_ACTIVE;
					changeda = TRUE;
				}
			}
			else
			{
				if (partition->flags & PNF_ACTIVE)
				{
					partition->flags &= ~PNF_ACTIVE;
					changeda = TRUE;
				}
			}
			if (changeda)
			{
				if (partition->flags & PNF_ACTIVE)
				{
					struct HDTBPartition *other;
					other = (struct HDTBPartition *)partition->listnode.ln.ln_Pred;
					while (other->listnode.ln.ln_Pred)
					{
						if (other->listnode.ln.ln_Type == LNT_Partition)
						{
							if (other->flags & PNF_ACTIVE)
							{
								other->flags &= ~PNF_ACTIVE;
								SetPartitionAttrsA(other->ph, PT_ACTIVE, FALSE, TAG_DONE);
								setChanged(&other->listnode);
							}
						}
						other = (struct HDTBPartition *)other->listnode.ln.ln_Pred;
					}
					other = (struct HDTBPartition *)partition->listnode.ln.ln_Succ;
					while (other->listnode.ln.ln_Succ)
					{
						if (other->listnode.ln.ln_Type == LNT_Partition)
						{
							if (other->flags & PNF_ACTIVE)
							{
								other->flags &= ~PNF_ACTIVE;
								SetPartitionAttrsA(other->ph, PT_ACTIVE, FALSE, TAG_DONE);
								setChanged(&other->listnode);
							}
						}
						other = (struct HDTBPartition *)other->listnode.ln.ln_Succ;
					}
				}
				SetPartitionAttrsA(partition->ph, PT_ACTIVE, check, TAG_DONE);
				changed = TRUE;
			}
			changeda = FALSE;
			get(mountbootgadgets.automount, MUIA_Selected, &check);
			if (check)
			{
				if (!(partition->flags & PNF_AUTOMOUNT))
				{
					partition->flags |= PNF_AUTOMOUNT;
					changeda = TRUE;
				}
			}
			else
			{
				if (partition->flags & PNF_AUTOMOUNT)
				{
					partition->flags &= ~PNF_AUTOMOUNT;
					changeda = TRUE;
				}
			}
			if (changeda)
			{
				SetPartitionAttrsA(partition->ph, PT_AUTOMOUNT, check, TAG_DONE);
				changed = TRUE;
			}
			changeda = FALSE;
			get(mountbootgadgets.bootable, MUIA_Selected, &check);
			if (check)
			{
				if (!(partition->flags & PNF_BOOTABLE))
				{
					partition->flags |= PNF_BOOTABLE;
					changeda = TRUE;
				}
				get(mountbootgadgets.bootpri, MUIA_String_Integer, &value);
				if (value != partition->de.de_BootPri)
				{
					partition->de.de_BootPri = value;
					changeda = TRUE;
				}
			}
			else
			{
				if (partition->flags & PNF_BOOTABLE)
				{
					partition->flags &= ~PNF_BOOTABLE;
					partition->de.de_BootPri = 0;
					changeda = TRUE;
				}
			}
			if (changeda)
			{
				SetPartitionAttrsA(partition->ph, PT_BOOTABLE, check, PT_DOSENVEC, &partition->de, TAG_DONE);
				changed = TRUE;
			}
			if (changed)
			{
				mountbootgadgets.iln->parent->flags |= LNF_ToSave;
				setChanged(mountbootgadgets.iln);
			}
		}
	}
	else
	{
		switch (i)
		{
		case GB_ADD_ENTRY:
			DoMethod(gadgets.leftlv, MUIM_List_GetEntry, 0, (IPTR)&iln);
			if (
					(iln == NULL) ||
					((iln) && (iln->parent->parent == NULL))
				)
			{
				set(adddevicegadgets.win, MUIA_Window_Open, TRUE);
			}
			else if ((iln->parent->ln.ln_Type == LNT_Harddisk) || (iln->parent->ln.ln_Type == LNT_Partition))
			{
				struct HDTBPartition *table;
				table = (struct HDTBPartition *)iln->parent;
				if (
						(
							(table->table->max_partitions) &&
							(countNodes(&table->listnode.list, LNT_Partition)<table->table->max_partitions)
						) ||
						(table->table->max_partitions == 0)
					)
				{	
					DoMethod
					(
						addpartitiongadgets.win,
						MUIM_Notify, MUIA_Window_Open, TRUE, (IPTR)addpartitiongadgets.pt, 3,
						MUIM_Set, PTCT_PartitionTable, (IPTR)table
					);
					set(addpartitiongadgets.win, MUIA_Window_Open, TRUE);
				}
				else
					set(gadgets.text, MUIA_Text_Contents, MSG(MSG_PARTITION_TABLE_FULL));
			}
			break;
		case GB_REMOVE_ENTRY:
			get(gadgets.leftlv, MUIA_List_Active, &active);
			if (active != MUIV_List_Active_Off)
			{
				DoMethod(gadgets.leftlv, MUIM_List_GetEntry, active, (IPTR)&iln);
				switch (iln->ln.ln_Type)
				{
			 	case LNT_Device:
					DoMethod(gadgets.leftlv, MUIM_List_Remove, active);
					Remove(&iln->ln);
					freeDeviceNode((struct HDTBDevice *)iln);
					break;
				case LNT_Partition:
					{
						struct HDTBPartition *partition;
						partition = (struct HDTBPartition *)iln;
						iln->parent->flags |= LNF_ToSave;
						setChanged(iln);
						DoMethod(gadgets.leftlv, MUIM_List_Remove, active);
						Remove(&iln->ln);
						DeletePartition(partition->ph);
						freePartitionNode(partition);
					}
				}
			}
			break;
		case GB_CREATE_TABLE:
		case GB_CHANGE_TYPE:
			get(gadgets.leftlv, MUIA_List_Active, &active);
			if (active != MUIV_List_Active_Off)
			{
				DoMethod(gadgets.leftlv, MUIM_List_GetEntry, active, (IPTR)&iln);
				if (
						(iln->ln.ln_Type == LNT_Harddisk) ||
						((i==2) && (iln->ln.ln_Type == LNT_Partition))
					)
				{
					struct HDTBPartition *table;
					ULONG type = 0;
					partitiontabletypegadgets.iln = iln;
					table = (struct HDTBPartition *)iln;
					if (table->table)
						type = table->table->type;
					DoMethod(partitiontabletypegadgets.lv, MUIM_List_Clear);
					for (i=0;PartitionBase->tables[i] != NULL;i++)
					{
						DoMethod
						(
							partitiontabletypegadgets.lv,
							MUIM_List_InsertSingle, (IPTR)PartitionBase->tables[i]->pti_Name, MUIV_List_Insert_Bottom
						);
						if (type == PartitionBase->tables[i]->pti_Type)
							set(partitiontabletypegadgets.lv, MUIA_List_Active, i);
					}
					set(partitiontabletypegadgets.win, MUIA_Window_Open, TRUE);
				}
				else if ((i==3) && (iln->ln.ln_Type == LNT_Partition))
				{
					struct TypeNode *tn;
					struct HDTBPartition *partition;
					partitiontypegadgets.iln = iln;
					i = 0;
					DoMethod(partitiontypegadgets.lv, MUIM_List_Clear);
					partition = (struct HDTBPartition *)iln;
					partitiontypegadgets.ttn = findTableTypeNode(partition->root->table->type);
					if (partitiontypegadgets.ttn)
					{
						tn = (struct TypeNode *)partitiontypegadgets.ttn->typelist.lh_Head;
						while (tn->ln.ln_Succ)
						{
							DoMethod
							(
								partitiontypegadgets.lv,
								MUIM_List_InsertSingle, (IPTR)tn->ln.ln_Name, MUIV_List_Insert_Bottom
							);
							if (tn->type.id_len == partition->type.id_len)
								if (memcmp(tn->type.id, partition->type.id, tn->type.id_len) == 0)
									set(partitiontypegadgets.lv, MUIA_List_Active, i);
							tn = (struct TypeNode *)tn->ln.ln_Succ;
							i++;
						}
					}
					setTypeString(&partition->type, partitiontypegadgets.hexid);
					set(partitiontypegadgets.win, MUIA_Window_Open, TRUE);
				}
			}
			break;
		case GB_RESIZE_MOVE:
			break;
		case GB_PARENT:
			DoMethod(gadgets.leftlv, MUIM_List_GetEntry, 0, (IPTR)&iln);
			if (iln->ln.ln_Type == LNT_Parent)
			{
				set(gadgets.leftlv, MUIA_List_Active, 0);
				set(gadgets.leftlv, MUIA_Listview_DoubleClick, TRUE);
			}
			break;
		case GB_RENAME:
			get(gadgets.leftlv, MUIA_List_Active, &active);
			if (active != MUIV_List_Active_Off)
			{
				DoMethod(gadgets.leftlv, MUIM_List_GetEntry, active, (IPTR)&iln);
				if (iln->ln.ln_Type == LNT_Partition)
				{
					struct HDTBPartition *partition;
					partition = (struct HDTBPartition *)iln;
					if (getAttrInfo(partition->root->table->pattrlist, PTA_NAME) & PLAM_READ)
					{
						renamegadgets.iln = iln;
						set(renamegadgets.name, MUIA_String_Contents, iln->ln.ln_Name);
						set(renamegadgets.win, MUIA_Window_Open, TRUE);
					}
				}
			}
			break;
		case GB_DOSENVEC:
			get(gadgets.leftlv, MUIA_List_Active, &active);
			if (active != MUIV_List_Active_Off)
			{
				DoMethod(gadgets.leftlv, MUIM_List_GetEntry, active, (IPTR)&iln);
				if (iln->ln.ln_Type == LNT_Partition)
				{
					struct HDTBPartition *partition;
					partition = (struct HDTBPartition *)iln;
					if (getAttrInfo(partition->root->table->pattrlist, PTA_DOSENVEC) & PLAM_READ)
					{
						char str[32];
						dosenvecgadgets.iln = iln;
						set(dosenvecgadgets.mask, MUIA_String_Integer, partition->de.de_Mask);
						sprintf(str, "0x%08lx", partition->de.de_MaxTransfer);
						set(dosenvecgadgets.maxtransfer, MUIA_String_Contents, str);
						if (partition->de.de_TableSize>=(DE_BOOTBLOCKS+1))
						{
							set(dosenvecgadgets.custboot, MUIA_Selected, TRUE);
							set(dosenvecgadgets.numcustboot, MUIA_String_Integer, partition->de.de_BootBlocks);
						}
						else
							set(dosenvecgadgets.custboot, MUIA_Selected, FALSE);
						set(dosenvecgadgets.reservedblocksstart, MUIA_String_Integer, partition->de.de_Reserved);
						set(dosenvecgadgets.reservedblocksend, MUIA_String_Integer, partition->de.de_PreAlloc);
						set(dosenvecgadgets.blocksize, MUIA_Cycle_Active, getBitNum(partition->de.de_SizeBlock>>7));
						set(dosenvecgadgets.buffers, MUIA_String_Integer, partition->de.de_NumBuffers);
						set(dosenvecgadgets.win, MUIA_Window_Open, TRUE);
					}
				}
			}
			break;
		case GB_SWITCHES:
			get(gadgets.leftlv, MUIA_List_Active, &active);
			if (active != MUIV_List_Active_Off)
			{
				DoMethod(gadgets.leftlv, MUIM_List_GetEntry, active, (IPTR)&iln);
				if (iln->ln.ln_Type == LNT_Partition)
				{
					struct HDTBPartition *partition;
					BOOL active;
					BOOL automount;
					BOOL bootable;
					partition = (struct HDTBPartition *)iln;
					active = getAttrInfo(partition->root->table->pattrlist, PTA_ACTIVE);
					automount = getAttrInfo(partition->root->table->pattrlist, PTA_AUTOMOUNT);
					bootable = getAttrInfo(partition->root->table->pattrlist, PTA_BOOTABLE);
					if ((active | automount | bootable) & PLAM_READ)
					{
						mountbootgadgets.iln = iln;
						set(mountbootgadgets.active, MUIA_Disabled, !(active & PLAM_READ));
						set(mountbootgadgets.automount, MUIA_Disabled, !(automount & PLAM_READ));
						set(mountbootgadgets.bootable, MUIA_Disabled, !(bootable & PLAM_READ));
						set(mountbootgadgets.bootpri, MUIA_Disabled, !(bootable & PLAM_READ));
						set(mountbootgadgets.active, MUIA_Selected, (partition->flags & PNF_ACTIVE) ? TRUE : FALSE);
						set(mountbootgadgets.automount, MUIA_Selected, (partition->flags & PNF_AUTOMOUNT) ? TRUE : FALSE);
						if (partition->flags & PNF_BOOTABLE)
						{
							set(mountbootgadgets.bootable, MUIA_Selected, TRUE);
							set(mountbootgadgets.bootpri, MUIA_String_Integer, partition->de.de_BootPri);
						}
						else
							set(mountbootgadgets.bootable, MUIA_Selected, FALSE);
						set(mountbootgadgets.win, MUIA_Window_Open, TRUE);
					}
				}
			}
			break;
		case GB_SAVE_CHANGES:
			get(gadgets.leftlv, MUIA_List_Active, &active);
			if (active != MUIV_List_Active_Off)
			{
				DoMethod(gadgets.leftlv, MUIM_List_GetEntry, active, (IPTR)&iln);
				saveChanges(iln, TRUE);
			}
			break;
		}
	}

    AROS_USERFUNC_EXIT
}

/************************* general List functions ***************************/

LONG InitListNode(struct ListNode *node, struct ListNode *parent) {
struct ListNode *new;

	NEWLIST(&node->list);
	new = AllocMem(sizeof(struct ListNode), MEMF_PUBLIC | MEMF_CLEAR);
	if (new)
	{
		node->parent = parent;
		new->ln.ln_Name = "..";
		new->ln.ln_Type = LNT_Parent;
		new->ln.ln_Pri = 127;
		new->parent = node;
		AddTail(&node->list, &new->ln);
		return TRUE;
	}
	return FALSE;
}

void UninitListNode(struct ListNode *node) {
struct ListNode *parent;

	/* free parent entry */
	parent = (struct ListNode *)node->list.lh_Head;
	while (parent->ln.ln_Succ)
	{
		if (parent->ln.ln_Type == LNT_Parent)
		{
			Remove(&parent->ln);
			FreeMem(parent, sizeof(struct ListNode));
			return;
		}
		parent = (struct ListNode *)parent->ln.ln_Succ;
	}
}

void InsertList(Object *list, struct ListNode *node) {
	DoMethod(list, MUIM_List_InsertSingle, (IPTR)node, MUIV_List_Insert_Bottom);
}

void ShowList(Object *list, struct List *lh) {
struct ListNode *lnode;

	DoMethod(list, MUIM_List_Clear);
	lnode = (struct ListNode *)lh->lh_Head;
	while (lnode->ln.ln_Succ)
	{
		if (
				(
					((lnode->ln.ln_Type == LNT_Parent) && (lnode->parent->parent != NULL)) ||
					(lnode->ln.ln_Type != LNT_Parent)
				) &&
				(!(lnode->flags & LNF_Invalid))
			)
		{
			DoMethod(list, MUIM_List_InsertSingle, (IPTR)lnode, MUIV_List_Insert_Bottom);
		}
		lnode = (struct ListNode *)lnode->ln.ln_Succ;
	}
}

void disableObject(Object *object) {
LONG disabled;

	get(object, MUIA_Disabled, &disabled);
	if (disabled == FALSE)
	{
		set(object, MUIA_Disabled, TRUE);
	}
}

void enableObject(Object *object) {
LONG disabled;

	get(object, MUIA_Disabled, &disabled);
	if (disabled == TRUE)
	{
		set(object, MUIA_Disabled, FALSE);
	}
}


/********************************** Left  Listview ***************************/

AROS_UFH3(void, lv_doubleclick,
	AROS_UFHA(struct Hook *, h, A0),
	AROS_UFHA(Object *, object, A2),
	AROS_UFHA(void *, arg, A1))
{
    AROS_USERFUNC_INIT

LONG active;
LONG type=-1;
struct ListNode *iln;

	get(object, MUIA_List_Active, &active);
	if (active != MUIV_List_Active_Off)
	{
		DoMethod(object,MUIM_List_GetEntry,active, (IPTR)&iln);
		if (iln->flags & LNF_Listable)
		{
			ShowList(object, &iln->list);
			type = iln->ln.ln_Type;
		}
		else if (iln->ln.ln_Type == LNT_Parent)
		{
			if (iln->parent->parent)
			{
				ShowList(object, &iln->parent->parent->list);
				type = iln->parent->parent->ln.ln_Type;
				iln = iln->parent;
			}
		}
		switch (type)
		{
		case LNT_Root:
			enableObject(gadgets.buttons[GB_ADD_ENTRY]);
			disableObject(gadgets.buttons[GB_REMOVE_ENTRY]);
			disableObject(gadgets.buttons[GB_CREATE_TABLE]);
			disableObject(gadgets.buttons[GB_CHANGE_TYPE]);
			disableObject(gadgets.buttons[GB_RESIZE_MOVE]);
			disableObject(gadgets.buttons[GB_PARENT]);
			disableObject(gadgets.buttons[GB_RENAME]);
			disableObject(gadgets.buttons[GB_DOSENVEC]);
			disableObject(gadgets.buttons[GB_SWITCHES]);
			disableObject(gadgets.buttons[GB_SAVE_CHANGES]);
			break;
		case LNT_Device:
			disableObject(gadgets.buttons[GB_ADD_ENTRY]);
			disableObject(gadgets.buttons[GB_REMOVE_ENTRY]);
			disableObject(gadgets.buttons[GB_CREATE_TABLE]);
			disableObject(gadgets.buttons[GB_CHANGE_TYPE]);
			disableObject(gadgets.buttons[GB_RESIZE_MOVE]);
			enableObject(gadgets.buttons[GB_PARENT]);
			disableObject(gadgets.buttons[GB_RENAME]);
			disableObject(gadgets.buttons[GB_DOSENVEC]);
			disableObject(gadgets.buttons[GB_SWITCHES]);
			disableObject(gadgets.buttons[GB_SAVE_CHANGES]);
			break;
		case LNT_Harddisk:
		case LNT_Partition:
			enableObject(gadgets.buttons[GB_ADD_ENTRY]);
			disableObject(gadgets.buttons[GB_REMOVE_ENTRY]);
			disableObject(gadgets.buttons[GB_CREATE_TABLE]);
			disableObject(gadgets.buttons[GB_CHANGE_TYPE]);
			enableObject(gadgets.buttons[GB_RESIZE_MOVE]);
			enableObject(gadgets.buttons[GB_PARENT]);
			disableObject(gadgets.buttons[GB_RENAME]);
			disableObject(gadgets.buttons[GB_DOSENVEC]);
			disableObject(gadgets.buttons[GB_SWITCHES]);
			disableObject(gadgets.buttons[GB_SAVE_CHANGES]);
			DoMethod(gadgets.buttons[GB_RESIZE_MOVE], MUIM_KillNotify, MUIA_Pressed);
			DoMethod
			(
				gadgets.buttons[GB_RESIZE_MOVE],
				MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)resizemovegadgets.win, 3,
				MUIM_Set, MUIA_Window_Open, TRUE
			);
			DoMethod
			(
				resizemovegadgets.win,
				MUIM_Notify, MUIA_Window_Open, TRUE, (IPTR)resizemovegadgets.pt, 3,
				MUIM_Set, PTCT_PartitionTable, (IPTR)iln
			);
			break;
		}
	}

    AROS_USERFUNC_EXIT
}

AROS_UFH3(void, lv_click,
	AROS_UFHA(struct Hook *, h, A0),
	AROS_UFHA(Object *, object, A2),
	AROS_UFHA(void *, arg, A1))
{
    AROS_USERFUNC_INIT

LONG active;
struct ListNode *iln;
char str[64];
char sizestr[16];

	get(object, MUIA_List_Active, &active);
	if (active != MUIV_List_Active_Off)
	{
		DoMethod(object,MUIM_List_GetEntry,active, (IPTR)&iln);
		DoMethod(gadgets.rightlv, MUIM_List_Clear);
		switch (iln->ln.ln_Type)
		{
		case LNT_Device:
			sprintf(str, "%s: %ld", MSG(WORD_Units),countNodes(&iln->list, LNT_Harddisk));
			DoMethod(gadgets.rightlv, MUIM_List_InsertSingle, (IPTR)str, MUIV_List_Insert_Bottom);
			enableObject(gadgets.buttons[GB_REMOVE_ENTRY]);
			if (iln->change_count > 0)
				enableObject(gadgets.buttons[GB_SAVE_CHANGES]);
			else
				disableObject(gadgets.buttons[GB_SAVE_CHANGES]);
			break;
		case LNT_Harddisk:
		case LNT_Partition:
			{
				struct HDTBPartition *partition;
				partition = (struct HDTBPartition *)iln;
				getSizeStr
				(
					sizestr,
					(
						(
							(partition->de.de_HighCyl-partition->de.de_LowCyl+1)*
							partition->de.de_Surfaces*partition->de.de_BlocksPerTrack
						)-1
					)/2
				);
				sprintf(str, "%s: %s", MSG(WORD_Size), sizestr);
				DoMethod(gadgets.rightlv, MUIM_List_InsertSingle, (IPTR)str, MUIV_List_Insert_Bottom);
				sprintf(str, "%s: ", MSG(WORD_Partition_Table));
				if (partition->table)
				{
					struct TableTypeNode *ttn;
					ttn = findTableTypeNode(partition->table->type);
					strcat(str, ttn->pti->pti_Name);
					DoMethod(gadgets.rightlv, MUIM_List_InsertSingle, (IPTR)str, MUIV_List_Insert_Bottom);
					sprintf(str, "%s: %ld", MSG(WORD_Partitions), countNodes(&iln->list, LNT_Partition));
					DoMethod(gadgets.rightlv, MUIM_List_InsertSingle, (IPTR)str, MUIV_List_Insert_Bottom);
				}
				else
				{
					strcat(str, MSG(WORD_Unknown));
					DoMethod(gadgets.rightlv, MUIM_List_InsertSingle, (IPTR)str, MUIV_List_Insert_Bottom);
				}
				if (iln->ln.ln_Type == LNT_Partition)
				{
					struct TypeNode *type;
					type = findPartitionType(&partition->type, partition->root->table->type);
					sprintf(str, "%s: ", MSG(WORD_Partition_Type));
					if (type)
						strcat(str, type->ln.ln_Name);
					else
						strcat(str, MSG(WORD_Unknown));
					DoMethod(gadgets.rightlv, MUIM_List_InsertSingle, (IPTR)str, MUIV_List_Insert_Bottom);
					sprintf(str, "%s: ", MSG(WORD_Active));
					if (partition->flags & PNF_ACTIVE)
						strcat(str, MSG_STD(YESSTR));
					else
						strcat(str, MSG_STD(NOSTR));
					DoMethod(gadgets.rightlv, MUIM_List_InsertSingle, (IPTR)str, MUIV_List_Insert_Bottom);
					sprintf(str, "%s: ", MSG(WORD_Bootable));
					if (partition->flags & PNF_BOOTABLE)
						strcat(str, MSG_STD(YESSTR));
					else
						strcat(str, MSG_STD(NOSTR));
					DoMethod(gadgets.rightlv, MUIM_List_InsertSingle, (IPTR)str, MUIV_List_Insert_Bottom);
					sprintf(str, "%s: ", MSG(WORD_Automount));
					if (partition->flags & PNF_AUTOMOUNT)
						strcat(str, MSG_STD(YESSTR));
					else
						strcat(str, MSG_STD(NOSTR));
					DoMethod(gadgets.rightlv, MUIM_List_InsertSingle, (IPTR)str, MUIV_List_Insert_Bottom);
					enableObject(gadgets.buttons[GB_REMOVE_ENTRY]);
					enableObject(gadgets.buttons[GB_RENAME]);
					enableObject(gadgets.buttons[GB_DOSENVEC]);
					enableObject(gadgets.buttons[GB_SWITCHES]);
				}
				else if (iln->ln.ln_Type == LNT_Harddisk)
				{
					if (iln->change_count > 0)
						enableObject(gadgets.buttons[GB_SAVE_CHANGES]);
					else
						disableObject(gadgets.buttons[GB_SAVE_CHANGES]);
				}
				enableObject(gadgets.buttons[GB_CREATE_TABLE]);
				enableObject(gadgets.buttons[GB_CHANGE_TYPE]);
			}
			break;
		case LNT_Parent:
			disableObject(gadgets.buttons[GB_REMOVE_ENTRY]);
			disableObject(gadgets.buttons[GB_RENAME]);
			disableObject(gadgets.buttons[GB_DOSENVEC]);
			disableObject(gadgets.buttons[GB_SWITCHES]);
			disableObject(gadgets.buttons[GB_CREATE_TABLE]);
			disableObject(gadgets.buttons[GB_CHANGE_TYPE]);
			disableObject(gadgets.buttons[GB_SAVE_CHANGES]);
			break;
		}
	}

    AROS_USERFUNC_EXIT
}

/**************************************** Main ******************************/

LONG initGUI(void) {
int i;

	MUIMasterBase = OpenLibrary("muimaster.library", 0);
	if (!MUIMasterBase)
		return ERR_MUIMASTER;

	ptclass = makePTClass();
	if (ptclass == NULL)
		return ERR_GADGETS;
	hook_display.h_Entry = (HOOKFUNC)display_function;
	hook_buttons.h_Entry = (HOOKFUNC)buttons_function;
	hook_lv_doubleclick.h_Entry = (HOOKFUNC)lv_doubleclick;
	hook_lv_click.h_Entry = (HOOKFUNC)lv_click;
	partitiontypegadgets.hook_hexidedit.h_Entry = (HOOKFUNC)hexidedit_function;

	app = ApplicationObject,
                MUIA_Application_Title      , "HDToolBox",
                MUIA_Application_Version    , "$VER: HDToolbox 0.1 (09-Apr-2003)",
                MUIA_Application_Copyright  , "(c) 1995-2003 by the AROS Development Team",
                MUIA_Application_Author     , "Bearly, Ogun, Fats and others at AROS",
                MUIA_Application_Description, "Partition your disks.",
                MUIA_Application_Base       , "HDTOOLBOX",
		MUIA_Application_Menustrip, MenuitemObject,
			MUIA_Family_Child, MenuitemObject,
				MUIA_Menuitem_Title, MSG(WORD_MENU_Project),
				MUIA_Family_Child,
					about_item = MenuitemObject, MUIA_Menuitem_Title, MSG(WORD_MENU_About),
				End,
				MUIA_Family_Child,
					quit_item  = MenuitemObject, MUIA_Menuitem_Title, MSG(WORD_MENU_Quit),
				End,
			End,
		End,
		SubWindow, mainwin = WindowObject,
			MUIA_Window_Title, "HDToolBox",
			MUIA_Window_Activate, TRUE,
			MUIA_Window_Height, MUIV_Window_Height_Visible(50),
			MUIA_Window_Width, MUIV_Window_Width_Visible(60),
			WindowContents, VGroup,
				Child, VGroup,
					GroupFrame,
					Child, gadgets.text = TextObject,
						TextFrame,
						MUIA_Text_Contents, MSG(MSG_Welcome),
					End,
					Child, HGroup,
						Child, ListviewObject,
							MUIA_Listview_List, gadgets.leftlv=ListObject,
								InputListFrame,
								MUIA_List_DisplayHook, &hook_display,
								MUIA_List_Format, ",",
								MUIA_List_Title, TRUE,
							End,
						End,
						Child, ListviewObject,
							MUIA_Listview_List, gadgets.rightlv=ListObject,
								ReadListFrame,
								MUIA_Listview_Input, FALSE,
								MUIA_List_Title, TRUE,
								MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
								MUIA_List_DestructHook, MUIV_List_DestructHook_String,
							End,
						End,
					End,
				End,
				Child, VGroup,
					GroupFrame,
					Child, HGroup,
						Child, VGroup,
							Child, gadgets.buttons[GB_ADD_ENTRY] = SIMPLEBUTTON(WORD_AddEntry),
							Child, gadgets.buttons[GB_REMOVE_ENTRY] = SIMPLEBUTTON(WORD_RemoveEntry),
						End,
						Child, VGroup,
							Child, gadgets.buttons[GB_CREATE_TABLE] = SIMPLEBUTTON(WORD_Create_Table),
							Child, gadgets.buttons[GB_CHANGE_TYPE] = SIMPLEBUTTON(WORD_Change_Type),
						End,
						Child, VGroup,
							Child, gadgets.buttons[GB_RESIZE_MOVE] = SIMPLEBUTTON(WORD_Resize_Move),
							Child, gadgets.buttons[GB_PARENT] = SIMPLEBUTTON(WORD_Parent),
						End,
						Child, VGroup,
							Child, gadgets.buttons[GB_RENAME] = SIMPLEBUTTON(WORD_Rename),
							Child, gadgets.buttons[GB_DOSENVEC] = SIMPLEBUTTON(WORD_DosEnvec),
						End,
						Child, VGroup,
							Child, gadgets.buttons[GB_SWITCHES] = SIMPLEBUTTON(WORD_Switches),
							Child, HVSpace,
						End,
					End,
					Child, HGroup,
					    	MUIA_Group_SameWidth, TRUE,
						MUIA_FixHeight, 1,
					    	Child, gadgets.buttons[GB_SAVE_CHANGES] = IMAGEBUTTON(WORD_Save_Changes, COOL_SAVEIMAGE_ID),
					    	Child, gadgets.buttons[GB_EXIT] = IMAGEBUTTON(WORD_Exit, COOL_CANCELIMAGE_ID),
					End,
				End,
			End,
		End,
		SubWindow, adddevicegadgets.win = WindowObject,
			MUIA_Window_Title, MSG(WORD_Devices),
			MUIA_Window_Activate, TRUE,
			WindowContents, VGroup,
				Child, PopaslObject,
					MUIA_Popstring_String, adddevicegadgets.file = StringObject,
						StringFrame,
					End,
					MUIA_Popstring_Button, PopButton(MUII_PopFile),
				End,
				Child, HGroup,
				    	MUIA_Group_SameWidth, TRUE,
					MUIA_FixHeight, 1,
					Child, adddevicegadgets.ok = IMAGEBUTTON(WORD_Ok, COOL_USEIMAGE_ID),
					Child, adddevicegadgets.cancel = IMAGEBUTTON(WORD_Cancel, COOL_CANCELIMAGE_ID),
				End,
			End,
		End,
		SubWindow, addpartitiongadgets.win = WindowObject,
			MUIA_Window_Title, MSG(MSG_Select_Empty_Area),
			MUIA_Window_Activate, TRUE,
			WindowContents, VGroup,
				Child, addpartitiongadgets.pt = BoopsiObject,
					GroupFrame,
					MUIA_Boopsi_Class, ptclass,
					MUIA_Boopsi_MinWidth, 600,
					MUIA_Boopsi_MinHeight, 100,
					MUIA_Boopsi_TagDrawInfo, GA_DrawInfo,
					MUIA_Boopsi_Remember, PTCT_PartitionTable,
					MUIA_Boopsi_Remember, PTCT_Flags,
					PTCT_Flags, PTCTF_NoPartitionMove | PTCTF_EmptySelectOnly,
					GA_Left, 0,
					GA_Top, 0,
					GA_Width, 0,
					GA_Height, 0,
					GA_DrawInfo, 0,
					MUIA_VertWeight, 10,
					ICA_TARGET, ICTARGET_IDCMP,
				End,
				Child, HGroup,
				    	MUIA_Group_SameWidth, TRUE,
				    	MUIA_FixHeight, 1,
					Child, addpartitiongadgets.ok = IMAGEBUTTON(WORD_Ok, COOL_USEIMAGE_ID),
					Child, addpartitiongadgets.cancel = IMAGEBUTTON(WORD_Cancel, COOL_CANCELIMAGE_ID),
				End,
			End,
		End,
		SubWindow, partitiontypegadgets.win = WindowObject,
			MUIA_Window_Title, MSG(WORD_Partition_Type),
			MUIA_Window_Activate, TRUE,
			WindowContents, VGroup,
				Child, ListviewObject,
					MUIA_Listview_List, partitiontypegadgets.lv=ListObject,
						InputListFrame,
						MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
						MUIA_List_DestructHook, MUIV_List_DestructHook_String,
						MUIA_List_Title, TRUE,
					End,
				End,
				Child, HGroup,
					Child, partitiontypegadgets.base = CycleObject,
						ButtonFrame,
						MUIA_Cycle_Entries, editcycleentries,
						MUIA_HorizWeight, 1,
					End,
					Child, partitiontypegadgets.hexid = StringObject,
						StringFrame,
						MUIA_String_EditHook, &partitiontypegadgets.hook_hexidedit,
					End,
				End,
				Child, HGroup,
				    	MUIA_Group_SameWidth, TRUE,
					MUIA_FixHeight, 1,
					Child, partitiontypegadgets.ok = IMAGEBUTTON(WORD_Ok, COOL_USEIMAGE_ID),
					Child, partitiontypegadgets.cancel = IMAGEBUTTON(WORD_Cancel, COOL_CANCELIMAGE_ID),
				End,
			End,
		End,
		SubWindow, partitiontabletypegadgets.win = WindowObject,
			MUIA_Window_Title, MSG(WORD_Partition_Table_Type),
			MUIA_Window_Activate, TRUE,
			WindowContents, VGroup,
				Child, ListviewObject,
					MUIA_Listview_List, partitiontabletypegadgets.lv=ListObject,
						InputListFrame,
						MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
						MUIA_List_DestructHook, MUIV_List_DestructHook_String,
						MUIA_List_Title, TRUE,
					End,
				End,
				Child, HGroup,
				    	MUIA_Group_SameWidth, TRUE,
					MUIA_FixHeight, 1,
					Child, partitiontabletypegadgets.ok = IMAGEBUTTON(WORD_Ok, COOL_USEIMAGE_ID),
					Child, partitiontabletypegadgets.cancel = IMAGEBUTTON(WORD_Cancel, COOL_CANCELIMAGE_ID),
				End,
			End,
		End,
		SubWindow, resizemovegadgets.win = WindowObject,
			MUIA_Window_Title, MSG(MSG_Select_Partition_Resize_Move),
			MUIA_Window_Activate, TRUE,
			WindowContents, VGroup,
				Child, resizemovegadgets.pt = BoopsiObject,
					GroupFrame,
					MUIA_Boopsi_Class, ptclass,
					MUIA_Boopsi_MinWidth, 600,
					MUIA_Boopsi_MinHeight, 100,
					MUIA_Boopsi_TagDrawInfo, GA_DrawInfo,
					MUIA_Boopsi_Remember, PTCT_PartitionTable,
					MUIA_Boopsi_Remember, PTCT_Flags,
					PTCT_Flags, 0,
					GA_Left, 0,
					GA_Top, 0,
					GA_Width, 0,
					GA_Height, 0,
					GA_DrawInfo, 0,
					MUIA_VertWeight, 10,
					ICA_TARGET, ICTARGET_IDCMP,
				End,
				Child, VGroup,
					GroupFrame,
					MUIA_FrameTitle, MSG(WORD_Geometry),
					Child, HGroup,
						Child, VGroup,
							Child, HGroup,
								Child, Label2("Low Cyl"),
								Child, resizemovegadgets.lowcyl=StringObject,
									StringFrame,
									MUIA_String_Integer, 0,
									MUIA_String_Accept, "0123456789",
								End,
							End,
							Child, HGroup,
								Child, Label2("High Cyl"),
								Child, resizemovegadgets.highcyl=StringObject,
									StringFrame,
									MUIA_String_Integer, 0,
									MUIA_String_Accept, "0123456789",
								End,
							End,
							Child, HGroup,
								Child, Label2("Total Cyl"),
								Child, resizemovegadgets.totalcyl=StringObject,
									StringFrame,
									MUIA_String_Integer, 0,
									MUIA_String_Accept, "0123456789",
								End,
							End,
							MUIA_HorizWeight, 10,
						End,
						Child, Label2("Size"),
						Child, resizemovegadgets.size=StringObject,
							ButtonFrame,
							MUIA_String_Accept, "0123456789GM.",
							MUIA_HorizWeight, 3,
						End,
					End,
				End,
				Child, HGroup,
				    	MUIA_Group_SameWidth, TRUE,
					MUIA_FixHeight, 1,
					Child, resizemovegadgets.ok = IMAGEBUTTON(WORD_Ok, COOL_USEIMAGE_ID),
					Child, resizemovegadgets.cancel = IMAGEBUTTON(WORD_Cancel, COOL_CANCELIMAGE_ID),
				End,
			End,
		End,
		SubWindow, renamegadgets.win = WindowObject,
			MUIA_Window_Title, MSG(WORD_Rename),
			MUIA_Window_Activate, TRUE,
			WindowContents, VGroup,
				Child, renamegadgets.name = StringObject,
					StringFrame,
				End,
				Child, HGroup,
				    	MUIA_Group_SameWidth, TRUE,
					MUIA_FixHeight, 1,
					Child, renamegadgets.ok = IMAGEBUTTON(WORD_Ok, COOL_USEIMAGE_ID),
					Child, renamegadgets.cancel = IMAGEBUTTON(WORD_Cancel, COOL_CANCELIMAGE_ID),
				End,
			End,
		End,
		SubWindow, dosenvecgadgets.win = WindowObject,
			MUIA_Window_Title, MSG(WORD_DosEnvec),
			MUIA_Window_Activate, TRUE,
			WindowContents, VGroup,
				Child, VGroup,
					GroupFrame,
					Child, HGroup,
						Child, Label2("Mask"),
						Child, dosenvecgadgets.mask=StringObject,
							StringFrame,
							MUIA_String_Integer, -2,
							MUIA_String_Accept, "0123456789",
						End,
					End,
					Child, HGroup,
						Child, Label2("MaxTransfer"),
						Child, dosenvecgadgets.maxtransfer=StringObject,
							StringFrame,
							MUIA_String_Contents, "0x7FFFFF",
							MUIA_String_Accept, "0123456789x",
						End,
					End,
				End,
				Child, VGroup,
					GroupFrame,
					Child, HGroup,
						Child, Label("_Custom Bootcode"),
						Child, HVSpace,
						Child, dosenvecgadgets.custboot=MUI_MakeObject
							(
								MUIO_Checkmark,
								(IPTR)"_Custom Bootcode"
							),
					End,
					Child, HGroup,
						Child, Label2("Nr. of Custom Bootblocks"),
						Child, dosenvecgadgets.numcustboot=StringObject,
							StringFrame,
							MUIA_String_Integer, 0,
							MUIA_String_Accept, "0123456789",
						End,
					End,
				End,
				Child, VGroup,
					GroupFrame,
					MUIA_FrameTitle, "Reserved Blocks at",
					Child, HGroup,
						Child, Label2("Beginning"),
						Child, dosenvecgadgets.reservedblocksstart=StringObject,
							StringFrame,
							MUIA_String_Integer, 0,
							MUIA_String_Accept, "0123456789",
						End,
					End,
					Child, HGroup,
						Child, Label2("End"),
						Child, dosenvecgadgets.reservedblocksend=StringObject,
							StringFrame,
							MUIA_String_Integer, 0,
							MUIA_String_Accept, "0123456789",
						End,
					End,
				End,
				Child, VGroup,
					GroupFrame,
					Child, HGroup,
						Child, Label("BlockSize"),
						Child, dosenvecgadgets.blocksize=CycleObject,
							ButtonFrame,
							MUIA_Cycle_Entries, blocksizecycleentries,
						End,
					End,
					Child, HGroup,
						Child, Label2("Buffers"),
						Child, dosenvecgadgets.buffers=StringObject,
							StringFrame,
							MUIA_String_Integer, 20,
							MUIA_String_Accept, "0123456789",
						End,
					End,
				End,
				Child, HGroup,
				    	MUIA_Group_SameWidth, TRUE,
					MUIA_FixHeight, 1,
					Child, dosenvecgadgets.ok = IMAGEBUTTON(WORD_Ok, COOL_USEIMAGE_ID),
					Child, dosenvecgadgets.cancel = IMAGEBUTTON(WORD_Cancel, COOL_CANCELIMAGE_ID),
				End,
			End,
		End,
		SubWindow, mountbootgadgets.win = WindowObject,
			MUIA_Window_Title, MSG(WORD_Switches),
			MUIA_Window_Activate, TRUE,
			WindowContents, VGroup,
				Child, HGroup,
					Child, Label(MSG(WORD_Active)),
					Child, HVSpace,
					Child, mountbootgadgets.active=MUI_MakeObject
						(
							MUIO_Checkmark,
							MSG(WORD_Active)
						),
				End,
				Child, HGroup,
					Child, Label(MSG(WORD_Automount)),
					Child, HVSpace,
					Child, mountbootgadgets.automount=MUI_MakeObject
						(
							MUIO_Checkmark,
							MSG(WORD_Automount)
						),
				End,
				Child, HGroup,
					Child, Label(MSG(WORD_Bootable)),
					Child, HVSpace,
					Child, mountbootgadgets.bootable=MUI_MakeObject
						(
							MUIO_Checkmark,
							MSG(WORD_Bootable)
						),
				End,
				Child, HGroup,
					Child, Label(MSG(WORD_BootPri)),
					Child, mountbootgadgets.bootpri=StringObject,
						StringFrame,
						MUIA_String_Integer, 0,
						MUIA_String_Accept, "0123456789",
					End,
				End,
				Child, HGroup,
				    	MUIA_Group_SameWidth, TRUE,
					MUIA_FixHeight, 1,
					Child, mountbootgadgets.ok = IMAGEBUTTON(WORD_Ok, COOL_USEIMAGE_ID),
					Child, mountbootgadgets.cancel = IMAGEBUTTON(WORD_Cancel, COOL_CANCELIMAGE_ID),
				End,
			End,
		End,
	End;
	if (!app)
		return ERR_GADGETS;
	/* Main Window */
	DoMethod
	(
		mainwin, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)app, 2,
		MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
	);
	DoMethod
	(
		gadgets.buttons[GB_EXIT], MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2,
		MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
	);
	DoMethod
	(
		gadgets.leftlv,
			MUIM_Notify, MUIA_Listview_DoubleClick, TRUE, (IPTR)gadgets.leftlv, 2,
			MUIM_CallHook, (IPTR)&hook_lv_doubleclick
	);
	DoMethod
	(
		gadgets.leftlv,
			MUIM_Notify, MUIA_Listview_SelectChange, TRUE, (IPTR)gadgets.leftlv, 2,
			MUIM_CallHook, (IPTR)&hook_lv_click
	);
	for (i=GB_FIRST;i<GB_EXIT;i++)
	{
		DoMethod
		(
			gadgets.buttons[i],
				MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)gadgets.buttons[i], 2,
				MUIM_CallHook, (IPTR)&hook_buttons
		);
		/* MUIM_CallHook, hook */
	}
	disableObject(gadgets.buttons[GB_REMOVE_ENTRY]);
	disableObject(gadgets.buttons[GB_CREATE_TABLE]);
	disableObject(gadgets.buttons[GB_CHANGE_TYPE]);
	disableObject(gadgets.buttons[GB_RESIZE_MOVE]);
	disableObject(gadgets.buttons[GB_PARENT]);
	disableObject(gadgets.buttons[GB_RENAME]);
	disableObject(gadgets.buttons[GB_DOSENVEC]);
	disableObject(gadgets.buttons[GB_SWITCHES]);
	disableObject(gadgets.buttons[GB_SAVE_CHANGES]);
	DoMethod
	(
		quit_item,
		MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, (IPTR)app, 2,
		MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
	);
	/* add device window */
	DoMethod
	(
		adddevicegadgets.win,
		MUIM_Notify, MUIA_Window_Open, TRUE, (IPTR)mainwin, 3,
		MUIM_Set, MUIA_Window_Sleep, TRUE
	);
	DoMethod
	(
		adddevicegadgets.win,
		MUIM_Notify, MUIA_Window_Open, FALSE, (IPTR)mainwin, 3,
		MUIM_Set, MUIA_Window_Sleep, FALSE
	);
	DoMethod
	(
		adddevicegadgets.win,
		MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)adddevicegadgets.win, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);
	DoMethod
	(
		adddevicegadgets.ok,
		MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)adddevicegadgets.ok, 2,
		MUIM_CallHook, (IPTR)&hook_buttons
	);
	DoMethod
	(
		adddevicegadgets.ok,
		MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)adddevicegadgets.win, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);
	DoMethod
	(
		adddevicegadgets.cancel,
		MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)adddevicegadgets.win, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);
	/* add partition window */
	DoMethod
	(
		addpartitiongadgets.win,
		MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)addpartitiongadgets.win, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);
	DoMethod
	(
		addpartitiongadgets.ok,
		MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)addpartitiongadgets.ok, 2,
		MUIM_CallHook, (IPTR)&hook_buttons
	);
	DoMethod
	(
		addpartitiongadgets.ok,
		MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)addpartitiongadgets.win, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);
	DoMethod
	(
		addpartitiongadgets.cancel,
		MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)addpartitiongadgets.win, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);
	DoMethod
	(
		addpartitiongadgets.pt,
		MUIM_Notify, PTCT_Selected, TRUE, (IPTR)addpartitiongadgets.pt, 2,
		MUIM_CallHook, (IPTR)&hook_buttons
	);
	/* partition type window */
	DoMethod
	(
		partitiontypegadgets.win,
		MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)partitiontypegadgets.win, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);
	DoMethod
	(
		partitiontypegadgets.hexid,
		MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
		(IPTR)partitiontypegadgets.hexid, 2,
		MUIM_CallHook, (IPTR)&hook_buttons
	);
#warning "FIXME: notify doesn't work"
	DoMethod
	(
		partitiontypegadgets.ok,
		MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)partitiontypegadgets.ok, 2,
		MUIM_CallHook, (IPTR)&hook_buttons
	);
	DoMethod
	(
		partitiontypegadgets.ok,
		MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)partitiontypegadgets.win, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);
	DoMethod
	(
		partitiontypegadgets.cancel,
		MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)partitiontypegadgets.win, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);
	DoMethod
	(
		partitiontypegadgets.lv,
		MUIM_Notify, MUIA_Listview_SelectChange, TRUE, (IPTR)partitiontypegadgets.lv, 2,
		MUIM_CallHook, (IPTR)&hook_buttons
	);
	/* partition table type window */
	DoMethod
	(
		partitiontabletypegadgets.win,
		MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)partitiontabletypegadgets.win, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);
	DoMethod
	(
		partitiontabletypegadgets.ok,
		MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)partitiontabletypegadgets.ok, 2,
		MUIM_CallHook, (IPTR)&hook_buttons
	);
	DoMethod
	(
		partitiontabletypegadgets.ok,
		MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)partitiontabletypegadgets.win, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);
	DoMethod
	(
		partitiontabletypegadgets.cancel,
		MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)partitiontabletypegadgets.win, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);
	/* resize/move window */
	DoMethod
	(
		resizemovegadgets.win,
		MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)resizemovegadgets.win, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);
	DoMethod
	(
		resizemovegadgets.ok,
		MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)resizemovegadgets.ok, 2,
		MUIM_CallHook, (IPTR)&hook_buttons
	);
	DoMethod
	(
		resizemovegadgets.ok,
		MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)resizemovegadgets.win, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);
	DoMethod
	(
		resizemovegadgets.size,
		MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
		(IPTR)resizemovegadgets.size, 2,
		MUIM_CallHook, (IPTR)&hook_buttons
	);
	DoMethod
	(
		resizemovegadgets.cancel,
		MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)resizemovegadgets.cancel, 2,
		MUIM_CallHook, (IPTR)&hook_buttons
	);
	DoMethod
	(
		resizemovegadgets.cancel,
		MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)resizemovegadgets.win, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);
	DoMethod
	(
		resizemovegadgets.pt,
		MUIM_Notify, PTCT_Selected, TRUE, (IPTR)resizemovegadgets.pt, 3,
		MUIM_CallHook, (IPTR)&hook_buttons, PTCT_Selected
	);
	DoMethod
	(
		resizemovegadgets.pt,
		MUIM_Notify, PTCT_PartitionMove, TRUE, (IPTR)resizemovegadgets.pt, 3,
		MUIM_CallHook, (IPTR)&hook_buttons, PTCT_PartitionMove
	);
	DoMethod
	(
		resizemovegadgets.highcyl,
		MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, (IPTR)resizemovegadgets.highcyl, 2,
		MUIM_CallHook, (IPTR)&hook_buttons
	);
	DoMethod
	(
		resizemovegadgets.lowcyl,
		MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, (IPTR)resizemovegadgets.lowcyl, 2,
		MUIM_CallHook, (IPTR)&hook_buttons
	);
	DoMethod
	(
		resizemovegadgets.totalcyl,
		MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, (IPTR)resizemovegadgets.totalcyl, 2,
		MUIM_CallHook, (IPTR)&hook_buttons
	);
	/* rename window */
	DoMethod
	(
		renamegadgets.win,
		MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)renamegadgets.win, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);
	DoMethod
	(
		renamegadgets.ok,
		MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)renamegadgets.ok, 2,
		MUIM_CallHook, (IPTR)&hook_buttons
	);
	DoMethod
	(
		renamegadgets.ok,
		MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)renamegadgets.win, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);
	DoMethod
	(
		renamegadgets.cancel,
		MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)renamegadgets.win, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);
	/* dosenvec window */
	DoMethod
	(
		dosenvecgadgets.win,
		MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)dosenvecgadgets.win, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);
	DoMethod
	(
		dosenvecgadgets.ok,
		MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)dosenvecgadgets.ok, 2,
		MUIM_CallHook, (IPTR)&hook_buttons
	);
	DoMethod
	(
		dosenvecgadgets.ok,
		MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)dosenvecgadgets.win, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);
	DoMethod
	(
		dosenvecgadgets.cancel,
		MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)dosenvecgadgets.win, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);
	/* automount/boot window */
	DoMethod
	(
		mountbootgadgets.win,
		MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)mountbootgadgets.win, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);
	DoMethod
	(
		mountbootgadgets.ok,
		MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)mountbootgadgets.ok, 2,
		MUIM_CallHook, (IPTR)&hook_buttons
	);
	DoMethod
	(
		mountbootgadgets.ok,
		MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)mountbootgadgets.win, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);
	DoMethod
	(
		mountbootgadgets.cancel,
		MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)mountbootgadgets.win, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);
	set(mainwin, MUIA_Window_Open, TRUE);
	return 0;
}

void deinitGUI() {
	if (app)
		MUI_DisposeObject(app);
	if (ptclass != NULL)
		FreeClass(ptclass);
	if (MUIMasterBase)
		CloseLibrary(MUIMasterBase);
}

BOOL QuitGUI(ULONG *sigs) {
	if ((LONG)DoMethod(app, MUIM_Application_NewInput, (IPTR)sigs) == MUIV_Application_ReturnID_Quit)
		return TRUE;
	return FALSE;
}


