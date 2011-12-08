/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Discover all mountable partitions
    Lang: english
*/

#include <string.h>
#include <stdlib.h>

#include <aros/debug.h>
#include <exec/alerts.h>
#include <aros/asmcall.h>
#include <aros/bootloader.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/types.h>
#include <libraries/configvars.h>
#include <libraries/expansion.h>
#include <libraries/expansionbase.h>
#include <libraries/partition.h>
#include <utility/tagitem.h>
#include <devices/bootblock.h>
#include <devices/timer.h>
#include <dos/dosextens.h>
#include <resources/filesysres.h>

#include <proto/exec.h>
#include <proto/expansion.h>
#include <proto/partition.h>
#include <proto/bootloader.h>

#include LC_LIBDEFS_FILE

#include "dosboot_intern.h"

#define uppercase(x) ((x >= 'a' && x <= 'z') ? (x & 0xdf) : x)

/* 
 * TODO: Check if DOSType lookup in partition.library really works
 * and remove this table and related code.
 */
static const struct _pt {
    IPTR    part,type;
} PartTypes[] = {
    { 0x01, AROS_MAKE_ID('F','A','T','\0') },	/* DOS 12-bit FAT */
    { 0x04, AROS_MAKE_ID('F','A','T','\0') },	/* DOS 16-bit FAT (up to 32M) */
    { 0x06, AROS_MAKE_ID('F','A','T','\0') },	/* DOS 16-bit FAT (over 32M) */
    { 0x07, AROS_MAKE_ID('N','T','F','S')  },	/* Windows NT NTFS */
    { 0x0b, AROS_MAKE_ID('F','A','T','\0') },	/* W95 FAT32 */
    { 0x0c, AROS_MAKE_ID('F','A','T','\0') },	/* W95 LBA FAT32 */
    { 0x0e, AROS_MAKE_ID('F','A','T','\0') },	/* W95 16-bit LBA FAT */
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

static ULONG GetOffset(struct Library *PartitionBase, struct PartitionHandle *ph)
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

static VOID AddPartitionVolume(struct ExpansionBase *ExpansionBase, struct Library *PartitionBase,
			       struct FileSysStartupMsg *fssm, struct PartitionHandle *table,
			       struct PartitionHandle *pn, struct ExecBase *SysBase)
{
    UBYTE name[32];
    ULONG i, blockspercyl;
    const struct PartitionAttribute *attrs;
    IPTR tags[7];
    IPTR pp[4 + DE_BOOTBLOCKS + 1] = { };
    struct DeviceNode *devnode;
    struct PartitionType ptyp;
    LONG ppos;
    TEXT *devname;
    LONG bootable;
    ULONG pttype = PHPTT_UNKNOWN;
    BOOL appended, changed;


    /*
     * TODO: Try to locate RDB filesystem for this volume and make it bootable.
     * Use FindFileSystem() and AddBootFileSystem() for this.
     */

    D(bug("[Boot] AddPartitionVolume\n"));
    GetPartitionTableAttrsTags(table, PTT_TYPE, &pttype, TAG_DONE);

    attrs = QueryPartitionAttrs(table);
    while ((attrs->attribute != PTA_DONE) && (attrs->attribute != PTA_NAME))
        attrs++;  /* look for name attr */
    if (attrs->attribute != PTA_DONE)
    {
        D(bug("[Boot] RDB/GPT partition\n"));

        /* partition has a name => RDB/GPT partition */
        tags[0] = PT_NAME;
        tags[1] = (IPTR)name;
        tags[2] = PT_DOSENVEC;
        tags[3] = (IPTR)&pp[4];
        tags[4] = PT_BOOTABLE;
        tags[5] = (IPTR)&bootable;
        tags[6] = TAG_DONE;
        GetPartitionAttrs(pn, (struct TagItem *)tags);
        D(bug("[Boot] Partition name: %s bootable: %d\n", name, bootable));

        /*
         * CHECKME: This should not be needed at all. Partition.library knows what it does,
         * and it knows DosEnvec size. RDB partitions should have complete DosEnvec. GPT
         * partitions (also processed here) have only fields up to de_DosType filled in,
         * and this is correctly reflected in the DosEnvec.
        pp[4 + DE_TABLESIZE] = DE_BOOTBLOCKS;
         */
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
        D(bug("[Boot] Partition name: %s type: %u\n", name, ptyp.id[0]));

        /*
         * FIXME: These MBR-related DosEnvec patches should already be correctly done
         * by partition.library. Test this and remove the unneeded code from here.
         */

        /* set DOSTYPE based on the partition type */
        pp[4 + DE_DOSTYPE] = MatchPartType(ptyp.id[0]);
        /* set some common DOSENV fields */
        pp[4 + DE_TABLESIZE] = DE_BOOTBLOCKS;
        pp[4 + DE_NUMBUFFERS] = 20;
        pp[4 + DE_BUFMEMTYPE] = MEMF_PUBLIC;
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

    if ((pp[4 + DE_TABLESIZE] < DE_DOSTYPE) || (pp[4 + DE_DOSTYPE] == 0))
    {
    	/*
    	 * partition.library reports DosType == 0 for unknown filesystems.
    	 * However dos.library will mount such DeviceNodes using rn_DefaultHandler
    	 * (FFS). This is done for compatibility with 3rd party expansion ROMs.
    	 * Here we ignore partitions with DosType == 0 and won't enter them into
    	 * mountlist.
    	 */
    	D(bug("[Boot] Unknown DosType for %s, skipping partition\n"));
    	return;
    }

    if (pttype != PHPTT_RDB)
    {
        /*
         * Only RDB partitions can store the complete DosEnvec.
         * For other partition types partition.library puts some defaults
         * into these fields, however they do not have anything to do with
         * real values, which are device-dependent.
         * However, the device itself knows them. Here we inherit these settings
         * from the original DeviceNode which represents the whole drive.
         * Note that we don't change DosEnvec size. If these fields are not included,
         * it will stay this way.
         * Copy members only if they are present in device's DosEnvec.
         */
        struct DosEnvec *devenv = BADDR(fssm->fssm_Environ);

        if (devenv->de_TableSize >= DE_MAXTRANSFER)
        {
            pp[4 + DE_MAXTRANSFER] = devenv->de_MaxTransfer;

            if (devenv->de_TableSize >= DE_MASK)
                pp[4 + DE_MASK] = devenv->de_Mask;
        }
    }

    /*
     * BHFormat complains if this bit is not set, and it's really wrong to have it unset.
     * So we explicitly set it here. Pavel Fedin <pavel.fedin@mail.ru>
     */
    pp[4 + DE_BUFMEMTYPE] |= MEMF_PUBLIC;

    pp[0] = (IPTR)name;
    pp[1] = (IPTR)AROS_BSTR_ADDR(fssm->fssm_Device);
    pp[2] = fssm->fssm_Unit;
    pp[3] = fssm->fssm_Flags;

    i = GetOffset(PartitionBase, pn);
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

    /* Append .n if same device name already exists */
    appended = FALSE;
    changed = TRUE;
    while (changed)
    {
        struct BootNode *bn;
        changed = FALSE;
        ForeachNode(&ExpansionBase->MountList, bn)
        {
            if (stricmp(AROS_BSTR_ADDR(((struct DeviceNode*)bn->bn_DeviceNode)->dn_Name), name) == 0)
            {
                if (!appended)
                    strcat(name, ".1");
                else
                    name[strlen(name) - 1]++;
                appended = TRUE;
                changed = TRUE;
            }
        }
    }

    devnode = MakeDosNode(pp);
    if (devnode != NULL) {
        AddBootNode(bootable ? pp[4 + DE_BOOTPRI] : -128, ADNF_STARTPROC, devnode, NULL);
        D(bug("[Boot] AddBootNode(%b, 0, 0x%p, NULL)\n",  devnode->dn_Name, pp[4 + DE_DOSTYPE]));
        return;
    }
}

static BOOL CheckTables(struct ExpansionBase *ExpansionBase, struct Library *PartitionBase,
			struct FileSysStartupMsg *fssm,	struct PartitionHandle *table,
			struct ExecBase *SysBase)
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

static VOID CheckPartitions(struct ExpansionBase *ExpansionBase, struct Library *PartitionBase, struct ExecBase *SysBase, struct BootNode *bn)
{
    struct DeviceNode *dn = bn->bn_DeviceNode;
    BOOL res = FALSE;

    D(bug("CheckPartition('%b') handler = %x\n", dn->dn_Name, dn->dn_SegList));
    
    /* If we already have filesystem handler, don't do anything */
    if (dn->dn_SegList == BNULL)
    {
    	struct FileSysStartupMsg *fssm = BADDR(dn->dn_Startup);

	if (fssm && fssm->fssm_Device)
	{
            struct PartitionHandle *pt = OpenRootPartition(AROS_BSTR_ADDR(fssm->fssm_Device), fssm->fssm_Unit);

	    if (pt)
            {
                res = CheckTables(ExpansionBase, PartitionBase, fssm, pt, SysBase);

           	CloseRootPartition(pt);
           }
        }
    }

    if (!res)    
        /* If no partitions were found for the DeviceNode, put it back */
        Enqueue(&ExpansionBase->MountList, &bn->bn_Node);
}

/* Scan all partitions manually for additional volumes that can be mounted. */
void dosboot_BootScan(LIBBASETYPEPTR LIBBASE)
{
    APTR PartitionBase;

    /* If we have partition.library, we can look for partitions */
    PartitionBase = OpenLibrary("partition.library", 2);
    if (PartitionBase)
    {
    	/*
    	 * Remove the whole chain of BootNodes from the list and re-initialize it.
    	 * We will insert new nodes into it, based on old ones.
    	 * What is done here is safe as long as we don't move the list itself.
    	 * ln_Succ of the last node in chain points to the lh_Tail of our list
    	 * which always contains NULL.
    	 */
	struct BootNode *bootNode = (struct BootNode *)LIBBASE->bm_ExpansionBase->MountList.lh_Head;

	NEWLIST(&LIBBASE->bm_ExpansionBase->MountList);

	while (bootNode->bn_Node.ln_Succ)
	{
	    /* Keep ln_Succ because it can be clobbered by reinsertion */
	    struct BootNode *nextNode = (struct BootNode *)bootNode->bn_Node.ln_Succ;

	    CheckPartitions(LIBBASE->bm_ExpansionBase, PartitionBase, SysBase, bootNode);
	    bootNode = nextNode;
	}

	CloseLibrary(PartitionBase);
    }
}
