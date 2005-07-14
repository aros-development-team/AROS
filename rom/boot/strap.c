/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Boot AROS
    Lang: english
*/

#define DEBUG 1

#include <string.h>

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

#define BOOT_CHECK 0

#define uppercase(x) ((x >= 'a' && x <= 'z') ? (x & 0xdf) : x)

int boot_entry()
{
	return -1;
}

static const UBYTE boot_end;
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
    { 0xffffffff, AROS_MAKE_ID('B','E','F','S' ), (IPTR)"befs.handler"  },
    { 0xffffff00, AROS_MAKE_ID('B','S','D','\0'), (IPTR)"bsd.handler"   },
    { 0xffffff00, AROS_MAKE_ID('C','P','M','\0'), (IPTR)"cpm.handler"   },
    { 0xffffff00, AROS_MAKE_ID('D','O','S','\0'), (IPTR)"afs.handler"   },
    { 0xffffff00, AROS_MAKE_ID('E','X','T','\0'), (IPTR)"ext.handler"   },
    { 0xffffff00, AROS_MAKE_ID('F','A','T','\0'), (IPTR)"fat.handler"   },
    { 0xffffff00, AROS_MAKE_ID('L','V','M','\0'), (IPTR)"lvm.handler"   },
    { 0xffffff00, AROS_MAKE_ID('M','N','X','\0'), (IPTR)"minix.handler" },
    { 0xffffffff, AROS_MAKE_ID('N','T','F','S' ), (IPTR)"ntfs.handler"  },
    { 0xffffffff, AROS_MAKE_ID('R','A','I','D' ), (IPTR)"raid.handler"  },
    { 0xffffff00, AROS_MAKE_ID('S','F','S','\0'), (IPTR)"sfs.handler"   },
    { 0xffffff00, AROS_MAKE_ID('S','K','Y','\0'), (IPTR)"skyfs.handler" },
    { 0xffffffff, AROS_MAKE_ID('V','F','A','T' ), (IPTR)"fat.handler"   },
    { 0,0,0, }
};

static const struct _pt {
    IPTR    part,type;
} PartTypes[] = {
    { 0x01, AROS_MAKE_ID('F','A','T',' ')  },	/* DOS 12-bit FAT */
    { 0x04, AROS_MAKE_ID('F','A','T',' ')  },	/* DOS 16-bit FAT (up to 32M) */
    { 0x06, AROS_MAKE_ID('F','A','T',' ')  },	/* DOS 16-bit FAT (over 32M) */
    { 0x07, AROS_MAKE_ID('N','T','F','S')  },	/* Windows NT NTFS */
    { 0x0b, AROS_MAKE_ID('V','F','A','T')  },	/* W95 FAT32 */
    { 0x0c, AROS_MAKE_ID('V','F','A','T')  },	/* W95 LBA FAT32 */
    { 0x0e, AROS_MAKE_ID('F','A','T',' ')  },	/* W95 16-bit LBA FAT */
    { 0x2c, AROS_MAKE_ID('D','O','S','\0') },	/* AOS OFS */
    { 0x2d, AROS_MAKE_ID('D','O','S','\1') },	/* AOS FFS */
    { 0x2e, AROS_MAKE_ID('D','O','S','\3') },	/* AOS FFS-I */
    { 0x2f, AROS_MAKE_ID('S','F','S','\0') },	/* AOS SFS */
    { 0x80, AROS_MAKE_ID('M','N','X','\0') },	/* MINIX until 1.4a */
    { 0x81, AROS_MAKE_ID('M','N','X','\1') },	/* MINIX since 1.4b */
    { 0x83, AROS_MAKE_ID('E','X','T','\2') },	/* linux native partition */
    { 0x8e, AROS_MAKE_ID('L','V','M','\0') },	/* linux LVM partition */
    { 0x9f, AROS_MAKE_ID('B','S','D','\0') },	/* BSD/OS */
    { 0xa5, AROS_MAKE_ID('B','S','D','\1') },	/* NetBSD, FreeBSD */
    { 0xa6, AROS_MAKE_ID('B','S','D','\2') },	/* OpenBSD */
    { 0xdb, AROS_MAKE_ID('C','P','M','\2') },	/* CPM/M */
    { 0xeb, AROS_MAKE_ID('B','E','F','S')  },	/* BeOS FS */
    { 0xec, AROS_MAKE_ID('S','K','Y','\0') },	/* SkyOS FS */
    { 0xfd, AROS_MAKE_ID('R','A','I','D')  },	/* linux RAID with autodetect */
    { 0, 0 }
};

static IPTR MatchHandler(IPTR DosType)
{
    int i;
    IPTR fs = 0;

    for (i = 0; i < (sizeof(DosTypes) / sizeof(struct _dt)); i++)
    {
	if ((DosType & DosTypes[i].mask) == DosTypes[i].type)
	{
	    fs = DosTypes[i].fs;
	    break;
	}
    }
    return fs;
}

static IPTR MatchPartType(UBYTE PartType)
{
    int i;
    IPTR type = 0;

    for (i = 0; i < (sizeof(PartTypes) / sizeof(struct _pt)); i++)
    {
	if ((IPTR)PartType == PartTypes[i].part)
	{
	    type = PartTypes[i].type;
	    break;
	}
    }
    return type;
}

static ULONG GetOffset(struct PartitionBase *PartitionBase,
    struct PartitionHandle *ph)
{
    STACKIPTR tags[3];
    struct DosEnvec de;
    ULONG offset = 0;

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

static VOID AddPartitionVolume
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
    const struct PartitionAttribute *attrs;
    STACKIPTR tags[7];
    IPTR *pp;
    struct DeviceNode *devnode;
    struct PartitionType ptyp;
    LONG ppos;
    TEXT *devname;

    pp = AllocVec(sizeof(struct DosEnvec) + sizeof(IPTR) * 4,
        MEMF_PUBLIC | MEMF_CLEAR);
    if (pp)
    {
        attrs = QueryPartitionAttrs(table);
        while ((attrs->attribute != PTA_DONE) && (attrs->attribute != PTA_NAME))
            attrs++;  /* look for name attr */
        if (attrs->attribute != PTA_DONE)
        {
            /* partition has a name => RDB partition */
	    tags[0] = PT_NAME;
	    tags[1] = (STACKIPTR)name;
	    tags[2] = PT_DOSENVEC;
	    tags[3] = (STACKIPTR)&pp[4];
	    tags[4] = TAG_DONE;
	    GetPartitionAttrs(pn, (struct TagItem *)tags);
        }
        else
        {
            /* partition doesn't have a name => MBR partition */
	    tags[0] = PT_POSITION;
	    tags[1] = (STACKIPTR)&ppos;
	    tags[2] = PT_TYPE;
	    tags[3] = (STACKIPTR)&ptyp;
	    tags[4] = PT_DOSENVEC;
	    tags[5] = (STACKIPTR)&pp[4];
	    tags[6] = TAG_DONE;
	    GetPartitionAttrs(pn, (struct TagItem *)tags);

            /* make the name */
            devname = AROS_BSTR_ADDR(fssm->fssm_Device);
            for (i = 0; i < 26; i++)
            {
                if (*devname == '.' || *devname == '\0')
                    break;
                name[i] = (UBYTE)uppercase(*devname);
                devname++;
            }
            if ((fssm->fssm_Unit / 10))
                name[i++] = '0' + (UBYTE)(fssm->fssm_Unit / 10);
            name[i++] = '0' + (UBYTE)(fssm->fssm_Unit % 10);
            name[i++] = 'P';
            if ((ppos / 10))
                name[i++] = '0' + (UBYTE)(ppos / 10);
            name[i++] = '0' + (UBYTE)(ppos % 10);
            name[i] = '\0';
            /* set DOSTYPE based on the partition type */
            pp[4 + DE_DOSTYPE] = MatchPartType(ptyp.id[0]);
            /* set some common DOSENV fields */
            pp[4 + DE_TABLESIZE] = DE_BOOTBLOCKS;
            pp[4 + DE_NUMBUFFERS] = 20;
            pp[4 + DE_BUFMEMTYPE] = MEMF_PUBLIC;
            pp[4 + DE_MAXTRANSFER] = 0x00200000;
            pp[4 + DE_MASK] = 0x7ffffffe;
            /* set some fs specific fields */
            switch(ptyp.id[0])
            {
                case 0x2c:	/* OFS */
                case 0x2d:	/* FFS */
                case 0x2e:	/* FFS I */
                case 0x2f:	/* SFS */
                    pp[4 + DE_SECSPERBLOCK] = 1;
                    pp[4 + DE_RESERVEDBLKS] = 2;
                    pp[4 + DE_BOOTBLOCKS] = 2;
                    break;
            }
        }

        pp[1] = (IPTR)AROS_BSTR_ADDR(fssm->fssm_Device);
        pp[2] = fssm->fssm_Unit;
        pp[3] = fssm->fssm_Flags;
        i = GetOffset(PartitionBase, pn);
        pp[4 + DE_LOWCYL] += i;
        pp[4 + DE_HIGHCYL] += i;

        pp[0] = MatchHandler(pp[4 + DE_DOSTYPE]);

        /* Skip unknown partition types */
        if (pp[0] != 0)
        {
            devnode = MakeDosNode(pp);
            if (devnode)
            {
                devnode->dn_OldName = MKBADDR(AllocVec(strlen(name) + 2,
                    MEMF_PUBLIC | MEMF_CLEAR));
                if (devnode->dn_OldName)
                {
                    i = 0;
                    while (name[i])
                    {
                        AROS_BSTR_putchar(devnode->dn_OldName, i, name[i]);
                        i++;
                    }
                    AROS_BSTR_setstrlen(devnode->dn_OldName, i);
                    devnode->dn_NewName = AROS_BSTR_ADDR(devnode->dn_OldName);
                    AddBootNode(pp[4 + DE_BOOTPRI], 0, devnode, 0);
                    D(bug("[Boot] AddBootNode(%s,0x%lx,'%s')\n",
                        devnode->dn_NewName, pp[4 + DE_DOSTYPE], pp[0]));
                    return;
                }
            }
        }
        FreeVec(pp);
    }
}

static BOOL CheckTables
    (
	struct ExpansionBase *ExpansionBase,
	struct PartitionBase *PartitionBase,
	struct FileSysStartupMsg *fssm,
	struct PartitionHandle *table,
	struct ExecBase *SysBase
    )
{
    BOOL retval = FALSE;
    struct PartitionHandle *ph;

    /* Traverse partition tables recursively, and attempt to add a BootNode
       for any non-subtable partitions found */
    if (OpenPartitionTable(table) == 0)
    {
        ph = (struct PartitionHandle *)table->table->list.lh_Head;
        while (ph->ln.ln_Succ)
        {
            /* Attempt to add partition to system if it isn't a subtable */
            if (!CheckTables(ExpansionBase, PartitionBase, fssm, ph, SysBase))
                AddPartitionVolume(ExpansionBase, PartitionBase, fssm, table,
                    ph, SysBase);
            ph = (struct PartitionHandle *)ph->ln.ln_Succ;
        }
        retval = TRUE;
        ClosePartitionTable(table);
    }
    return retval;
}

static BOOL IsRemovable(struct ExecBase *SysBase, struct IOExtTD *ioreq)
{
    struct DriveGeometry dg;

    ioreq->iotd_Req.io_Command = TD_GETGEOMETRY;
    ioreq->iotd_Req.io_Data = &dg;
    ioreq->iotd_Req.io_Length = sizeof(struct DriveGeometry);
    DoIO((struct IORequest *)ioreq);
    return (dg.dg_Flags & DGF_REMOVABLE) ? TRUE : FALSE;
}

static VOID CheckPartitions
	(
		struct ExpansionBase *ExpansionBase,
		struct ExecBase *SysBase,
		struct BootNode *bn
	)
{
    struct PartitionBase *PartitionBase;
    struct PartitionHandle *pt;
    struct FileSysStartupMsg *fssm;

    PartitionBase =
        (struct PartitionBase *)OpenLibrary("partition.library", 1);
    if (PartitionBase)
    {
        fssm = BADDR(((struct DeviceNode *)bn->bn_DeviceNode)->dn_Startup);
        pt = OpenRootPartition(AROS_BSTR_ADDR(fssm->fssm_Device),
            fssm->fssm_Unit);
        if (pt)
        {
            if (IsRemovable(SysBase, pt->bd->ioreq))
            {
                /* don't check removable devices for partition tables */
                Enqueue(&ExpansionBase->MountList, (struct Node *)bn);
            }
            else
            {
                if (!CheckTables(ExpansionBase, PartitionBase, fssm, pt,
                    SysBase))
                {
                    /* no partition table found, so reinsert node */
                    Enqueue(&ExpansionBase->MountList, (struct Node *)bn);
                }
            }
            CloseRootPartition(pt);
        }
        else
        {
            /* amicdrom fails here because of non-initialized libraries */
            Enqueue(&ExpansionBase->MountList, (struct Node *)bn);
        }
        CloseLibrary((struct Library *)PartitionBase);
    }
}

AROS_UFH3(int, AROS_SLIB_ENTRY(init, boot),
    AROS_UFHA(ULONG, dummy, D0),
    AROS_UFHA(ULONG, seglist, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT
    struct ExpansionBase *ExpansionBase;
    struct BootNode *bootNode;
    struct List list;
    struct Resident *DOSResident;

#if !(AROS_FLAVOUR & AROS_FLAVOUR_EMULATION)
    ExpansionBase =
        (struct ExpansionBase *)OpenLibrary("expansion.library", 0);
    if( ExpansionBase == NULL )
    {
        D(bug( "Could not open expansion.library, something's wrong!\n"));
        Alert(AT_DeadEnd | AG_OpenLib | AN_BootStrap | AO_ExpansionLib);
    }

    /* move all boot nodes into another list */
    NEWLIST(&list);
    while ((bootNode = (struct BootNode *)RemHead(&ExpansionBase->MountList)))
        AddTail(&list, &bootNode->bn_Node);

    /* check boot nodes for partition tables */
    while ((bootNode = (struct BootNode *)RemHead(&list)))
        CheckPartitions(ExpansionBase, SysBase, bootNode);
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

static const UBYTE boot_end = 0;
