/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Boot AROS
    Lang: english
*/

#define DEBUG 1

#include <exec/alerts.h>
#include <aros/asmcall.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/types.h>
#include <libraries/configvars.h>
#include <libraries/expansionbase.h>
#include <libraries/partition.h>
#include <utility/tagitem.h>
#include <devices/bootblock.h>

#include <proto/exec.h>
#include <proto/expansion.h>
#include <proto/partition.h>

#ifdef DEBUG
#include <aros/debug.h>
#endif
#include <aros/macros.h>

#include <string.h>

#define BOOT_CHECK 0

int boot_entry()
{
	return -1;
}

static const char boot_end;
int AROS_SLIB_ENTRY(init,boot)();

const struct Resident boot_resident =
{
	RTC_MATCHWORD,
	(struct Resident *)&boot_resident,
	(APTR)&boot_end,
	RTF_COLDSTART,
	41,
	NT_PROCESS,
	-50,
	"Boot Strap",
	"AROS Boot Strap 41.0\r\n",
	(APTR)&boot_init
};

static const struct _dt {
    IPTR    mask,type,fs;
} DosTypes[] = {
    { 0xffffff00, AROS_MAKE_ID('D','O','S','\0'), (IPTR)"afs.handler"	    },
    { 0xffffffff, AROS_MAKE_ID('E','X','T','\2'), (IPTR)"ext2.handler"    },
    { 0xffffff00, AROS_MAKE_ID('F','A','T',' ' ), (IPTR)"fat.handler"     },
    { 0,0,0, }
};

IPTR MatchHandler(IPTR DosType)
{
    int i;
    IPTR fs=0;

    for (i=0; i < (sizeof(DosTypes) / sizeof(struct _dt)); i++)
    {
	if ((DosType & DosTypes[i].mask) == DosTypes[i].type)
	{
	    fs = DosTypes[i].fs;
	    break;
	}
    }
    return fs;
}

ULONG getOffset(struct PartitionBase *PartitionBase, struct PartitionHandle *ph) {
STACKIPTR tags[3];
struct DosEnvec de;
ULONG offset=0;

    tags[0] = PT_DOSENVEC;
    tags[1] = (STACKIPTR)&de;
    tags[2] = TAG_DONE;
    ph = ph->root;
    while (ph->root)
    {
	GetPartitionAttrs(ph, (struct TagItem *)tags);
	offset += de.de_LowCyl;
    	ph = ph->root;
    }
    return offset;
}

void addPartitionVolume
    (
	struct ExpansionBase *ExpansionBase,
	struct PartitionBase *PartitionBase,
	struct FileSysStartupMsg *fssm,
	struct PartitionHandle *table,
	struct PartitionHandle *pn,
	struct ExecBase * SysBase
    )
{
UBYTE name[32];
ULONG i;
ULONG *attrs;
STACKIPTR tags[5];
IPTR *pp;
struct DeviceNode *devnode;

    attrs = QueryPartitionAttrs(table);
    while ((*attrs) && (*attrs != PTA_NAME))
	attrs++;
    if (*attrs)
    {
	pp = AllocVec(sizeof(struct DosEnvec)+sizeof(IPTR)*4, MEMF_PUBLIC | MEMF_CLEAR);
	if (pp)
	{
	    tags[0] = PT_NAME;
	    tags[1] = (STACKIPTR)name;
	    tags[2] = PT_DOSENVEC;
	    tags[3] = (STACKIPTR)&pp[4];
	    tags[4] = TAG_DONE;
	    GetPartitionAttrs(pn, (struct TagItem *)tags);
//	    pp[0] = (IPTR)"afs.handler";
	    pp[1] = (IPTR)AROS_BSTR_ADDR(fssm->fssm_Device);
	    pp[2] = fssm->fssm_Unit;
	    pp[3] = fssm->fssm_Flags;
	    i = getOffset(PartitionBase, pn);
	    pp[DE_LOWCYL+4] += i;
	    pp[DE_HIGHCYL+4] += i;
	    
	    pp[0] = MatchHandler(pp[DE_DOSTYPE + 4]);
	    
	    devnode = MakeDosNode(pp);
	    if (devnode)
	    {
		devnode->dn_OldName = MKBADDR(AllocVec(strlen(name)+2, MEMF_PUBLIC | MEMF_CLEAR));
		if (devnode->dn_OldName)
		{
		    i=0;
		    while (name[i])
		    {
			AROS_BSTR_putchar(devnode->dn_OldName, i, name[i]);
			i++;
		    }
		    AROS_BSTR_setstrlen(devnode->dn_OldName, i);
		    devnode->dn_NewName = AROS_BSTR_ADDR(devnode->dn_OldName);
		    AddBootNode(pp[DE_BOOTPRI+4], 0, devnode, 0);
		    D(bug("[Boot] AddBootNode(%s,%lx,'%s')\n", devnode->dn_NewName, pp[DE_DOSTYPE + 4],
			pp[0]));
		    return;
		}
	    }
	    FreeVec(pp);
	}
    }
}

BOOL checkTables
    (
	struct ExpansionBase *ExpansionBase,
	struct PartitionBase *PartitionBase,
	struct FileSysStartupMsg *fssm,
	struct PartitionHandle *table,
	struct ExecBase * SysBase
    )
{
BOOL retval = FALSE;
struct PartitionHandle *ph;

	if (OpenPartitionTable(table) == 0)
	{
		ph = (struct PartitionHandle *)table->table->list.lh_Head;
		while (ph->ln.ln_Succ)
		{
			checkTables(ExpansionBase, PartitionBase, fssm, ph, SysBase);
			addPartitionVolume(ExpansionBase, PartitionBase, fssm, table, ph, SysBase);
			ph = (struct PartitionHandle *)ph->ln.ln_Succ;
		}
		retval = TRUE;
		ClosePartitionTable(table);
	}
	return retval;
}

BOOL isRemovable(struct ExecBase *SysBase, struct IOExtTD *ioreq) {
struct DriveGeometry dg;

	ioreq->iotd_Req.io_Command = TD_GETGEOMETRY;
	ioreq->iotd_Req.io_Data = &dg;
	ioreq->iotd_Req.io_Length = sizeof(struct DriveGeometry);
	DoIO((struct IORequest *)ioreq);
	return (dg.dg_Flags & DGF_REMOVABLE) ? TRUE : FALSE;
}

void checkPartitions
	(
		struct ExpansionBase *ExpansionBase,
		struct ExecBase *SysBase,
		struct BootNode *bn
	)
{
struct PartitionBase *PartitionBase;
struct PartitionHandle *pt;
struct FileSysStartupMsg *fssm;

	PartitionBase = (struct PartitionBase *)OpenLibrary("partition.library", 1);
	if (PartitionBase)
	{
		fssm = BADDR(((struct DeviceNode *)bn->bn_DeviceNode)->dn_Startup);
		pt=OpenRootPartition(AROS_BSTR_ADDR(fssm->fssm_Device), fssm->fssm_Unit);
		if (pt)
		{
			if (isRemovable(SysBase,pt->bd->ioreq))
			{
				/* don't check removable devices for partition tables */
				Enqueue(&ExpansionBase->MountList, (struct Node *)bn);
			}
			else
			{
				if (!checkTables(ExpansionBase, PartitionBase, fssm, pt, SysBase))
				{
					/* no partition table found, so reinsert node */
					Enqueue(&ExpansionBase->MountList, (struct Node *)bn);
				}
			}
			CloseRootPartition(pt);
		}
		else
		{
			/* amicdrom fails here because of not initialized libraries */
			Enqueue(&ExpansionBase->MountList, (struct Node *)bn);
		}
		CloseLibrary((struct Library *)PartitionBase);
	}
}

AROS_UFH3(int, AROS_SLIB_ENTRY(init,boot),
    AROS_UFHA(ULONG, dummy, D0),
    AROS_UFHA(ULONG, seglist, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT
    struct ExpansionBase *ExpansionBase;
    struct BootNode      *bootNode;
    struct List list;
    struct Resident *DOSResident;
#if !(AROS_FLAVOUR & AROS_FLAVOUR_EMULATION)
	ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library", 0);
	if( ExpansionBase == NULL )
	{
		D(bug( "Could not open expansion.library, something's wrong!\n"));
		Alert(AT_DeadEnd | AG_OpenLib | AN_BootStrap | AO_ExpansionLib);
	}
	/* move all boot nodes into another list */
	NEWLIST(&list);
	while ((bootNode = (struct BootNode *)RemHead(&ExpansionBase->MountList)))
	{
		AddTail(&list, &bootNode->bn_Node);
		bootNode = (struct BootNode *)bootNode->bn_Node.ln_Succ;
	}
	/* check boot nodes for partition tables */
	while ((bootNode = (struct BootNode *)RemHead(&list)))
		checkPartitions(ExpansionBase, SysBase, bootNode);
	CloseLibrary((struct Library *)ExpansionBase);
#endif
    DOSResident = FindResident( "dos.library" );

    if( DOSResident == NULL )
    {
        Alert( AT_DeadEnd | AG_OpenLib | AN_BootStrap | AO_DOSLib );
    }

    InitResident( DOSResident, NULL );

    return 0;

    AROS_USERFUNC_EXIT
}

static const char boot_end = 0;
