/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Find and mount partitions.
    Lang: English
*/

#define DEBUG 0

#include <string.h>
#include <stdio.h>

#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <libraries/partition.h>
#include <utility/tagitem.h>
#include <devices/bootblock.h>
#include <devices/timer.h>

#include <proto/exec.h>
#include <proto/expansion.h>
#include <proto/partition.h>
#include <proto/bootloader.h>

#include <aros/debug.h>
#include <aros/macros.h>

#include "massstorage.h"
#include "massstorage.class.h"

#define PartitionBase ncm->ncm_ClsBase->nh_PartitionBase
#define ExpansionBase ncm->ncm_ClsBase->nh_ExpansionBase

#define uppercase(x) ((x >= 'a' && x <= 'z') ? (x & 0xdf) : x)

static const struct _dt {
    IPTR    mask,type;
    STRPTR  fs;
} DosTypes[] = {
    { 0xffffffff, AROS_MAKE_ID('B','E','F','S' ), "befs-handler"  },
    { 0xffffff00, AROS_MAKE_ID('B','S','D','\0'), "bsd-handler"   },
    { 0xffffff00, AROS_MAKE_ID('C','P','M','\0'), "cpm-handler"   },
    { 0xffffff00, AROS_MAKE_ID('D','O','S','\0'), "afs-handler"   },
    { 0xffffff00, AROS_MAKE_ID('E','X','T','\0'), "ext-handler"   },
    { 0xffffff00, AROS_MAKE_ID('F','A','T','\0'), "fat-handler"   },
    { 0xffffff00, AROS_MAKE_ID('L','V','M','\0'), "lvm-handler"   },
    { 0xffffff00, AROS_MAKE_ID('M','N','X','\0'), "minix-handler" },
    { 0xffffffff, AROS_MAKE_ID('N','T','F','S' ), "ntfs-handler"  },
    { 0xffffffff, AROS_MAKE_ID('R','A','I','D' ), "raid-handler"  },
    { 0xffffff00, AROS_MAKE_ID('S','F','S','\0'), "sfs-handler"   },
    { 0xffffff00, AROS_MAKE_ID('S','K','Y','\0'), "skyfs-handler" },
    { 0xffffffff, AROS_MAKE_ID('V','F','A','T' ), "fat-handler"   },
    { 0,0,NULL }
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

static STRPTR MatchHandler(IPTR DosType)
{
    int i;
    STRPTR fs = NULL;

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

static ULONG GetOffset(struct NepClassMS *ncm,
                       struct PartitionHandle *ph)
{
    IPTR tags[3];
    struct DosEnvec de;
    ULONG offset = 0;

    tags[0] = PT_DOSENVEC;
    tags[1] = (IPTR)&de;
    tags[2] = TAG_DONE;
    ph = ph->root;
    while (ph->root)
    {
        GetPartitionAttrs(ph, (struct TagItem *)tags);
        offset += de.de_LowCyl * de.de_Surfaces * de.de_BlocksPerTrack;
        ph = ph->root;
    }
    return offset;
}

static VOID AddPartitionVolume(struct NepClassMS *ncm,
                               struct FileSysStartupMsg *fssm,
                               struct PartitionHandle *table,
                               struct PartitionHandle *pn)
{
    UBYTE name[32];
    ULONG i, blockspercyl;
    const struct PartitionAttribute *attrs;
    IPTR tags[7];
    IPTR *pp;
    struct DeviceNode *devnode;
    struct PartitionType ptyp;
    LONG ppos;
    TEXT *devname, *handler;
    LONG bootable;

    D(bug("[Boot] AddPartitionVolume\n"));
    pp = AllocVec(sizeof(struct DosEnvec) + sizeof(IPTR) * 4,
        MEMF_PUBLIC | MEMF_CLEAR);
    if (pp)
    {
        attrs = QueryPartitionAttrs(table);
        while ((attrs->attribute != PTA_DONE) && (attrs->attribute != PTA_NAME))
            attrs++;  /* look for name attr */
        if (attrs->attribute != PTA_DONE)
        {
            D(bug("[Boot] RDB partition\n"));
            /* partition has a name => RDB partition */
            tags[0] = PT_NAME;
            tags[1] = (IPTR)name;
            tags[2] = PT_DOSENVEC;
            tags[3] = (IPTR)&pp[4];
            tags[4] = PT_BOOTABLE;
            tags[5] = (IPTR)&bootable;
            tags[6] = TAG_DONE;
            GetPartitionAttrs(pn, (struct TagItem *)tags);
            D(bug("[Boot] Partition name: %s bootable: %d\n", name, bootable));
            /* BHFormat complains if this bit is not set, and it's really wrong to have it unset. So we explicitly set it here.
	       Pavel Fedin <sonic_amiga@rambler.ru> */
            pp[4 + DE_TABLESIZE] = DE_BOOTBLOCKS;
            pp[4 + DE_BUFMEMTYPE] |= MEMF_PUBLIC;
        }
        else
        {
            D(bug("[Boot] MBR partition\n"));
            /* partition doesn't have a name => MBR partition */
            tags[0] = PT_POSITION;
            tags[1] = (IPTR)&ppos;
            tags[2] = PT_TYPE;
            tags[3] = (IPTR)&ptyp;
            tags[4] = PT_DOSENVEC;
            tags[5] = (IPTR)&pp[4];
            tags[6] = TAG_DONE;
            GetPartitionAttrs(pn, (struct TagItem *)tags);
            bootable = TRUE;

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
            if (table->table->type == PHPTT_EBR)
                ppos += 4;
            if ((ppos / 10))
                name[i++] = '0' + (UBYTE)(ppos / 10);
            name[i++] = '0' + (UBYTE)(ppos % 10);
            name[i] = '\0';
            D(bug("[Boot] Partition name: %s type: %lu bootable: %d\n", name, ptyp.id[0], bootable));
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

        pp[0] = (IPTR)name;
        pp[1] = (IPTR)AROS_BSTR_ADDR(fssm->fssm_Device);
        pp[2] = fssm->fssm_Unit;
        pp[3] = fssm->fssm_Flags;
        i = GetOffset(ncm, pn);
        blockspercyl = pp[4 + DE_BLKSPERTRACK] * pp[4 + DE_NUMHEADS];
        if (i % blockspercyl != 0)
        {
            D(bug("[Boot] Start block of subtable not on cylinder boundary: "
                "%ld (Blocks per Cylinder = %ld)\n", i, blockspercyl));
            return;
        }
        i /= blockspercyl;
        pp[4 + DE_LOWCYL] += i;
        pp[4 + DE_HIGHCYL] += i;

        D(bug("[Boot] Looking up handler for 0x%08lX\n", pp[4+DE_DOSTYPE]));
        handler = MatchHandler(pp[4 + DE_DOSTYPE]);

        /* Skip unknown partition types */
        if (handler != NULL)
        {
            D(bug("[Boot] found handler: %s\n", handler));
            devnode = MakeDosNode(pp);
            if (devnode != NULL)
            {
                devnode->dn_Handler = MKBADDR(AllocVec(AROS_BSTR_MEMSIZE4LEN(
                    strlen(handler)), MEMF_PUBLIC | MEMF_CLEAR));
                if (devnode->dn_Handler)
                {
                    i = 0;
                    while (handler[i] != '\0')
                    {
                        AROS_BSTR_putchar(devnode->dn_Handler, i, handler[i]);
                        i++;
                    }
                    AROS_BSTR_setstrlen(devnode->dn_Handler, i);
                    AddBootNode(bootable ? pp[4 + DE_BOOTPRI] : -128, ADNF_STARTPROC, devnode, NULL);
                    D(bug("[Boot] AddBootNode(%b, 0, 0x%p, NULL)\n",
                        devnode->dn_Name, pp[4 + DE_DOSTYPE]));
                    return;
                }
            }
        }
        FreeVec(pp);
    }
}

static BOOL CheckTables(struct NepClassMS *ncm,
                        struct FileSysStartupMsg *fssm,
                        struct PartitionHandle *table)
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
            if (!CheckTables(ncm, fssm, ph))
                AddPartitionVolume(ncm, fssm, table, ph);
            ph = (struct PartitionHandle *)ph->ln.ln_Succ;
        }
        retval = TRUE;
        ClosePartitionTable(table);
    }
    return retval;
}

BOOL CheckPartitions(struct NepClassMS *ncm)
{
    BOOL found = FALSE;
    struct PartitionHandle *pt;
    struct FileSysStartupMsg fssm;

    pt = OpenRootPartition(DEVNAME, ncm->ncm_UnitNo);
    if(pt)
    {
        fssm.fssm_Device = DEVNAME;
        fssm.fssm_Unit = ncm->ncm_UnitNo;
        fssm.fssm_Flags = 0;

        found = CheckTables(ncm, &fssm, pt);
        CloseRootPartition(pt);
    }

    return found;
}

