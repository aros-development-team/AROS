/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
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
#include "../expansion/expansion_intern.h"

#define uppercase(x) ((x >= 'a' && x <= 'z') ? (x & 0xdf) : x)

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
    LONG ppos;
    TEXT *devname;
    LONG bootable;
    ULONG pttype = PHPTT_UNKNOWN;
    BOOL appended, changed;
    struct Node *fsnode;

    D(bug("[Boot] AddPartitionVolume\n"));
    GetPartitionTableAttrsTags(table, PTT_TYPE, &pttype, TAG_DONE);

    attrs = QueryPartitionAttrs(table);
    while ((attrs->attribute != TAG_DONE) && (attrs->attribute != PT_NAME))
        attrs++;  /* look for name attr */

    if (attrs->attribute != TAG_DONE)
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
    }
    else
    {
        D(bug("[Boot] MBR/EBR partition\n"));

        /* partition doesn't have a name => MBR/EBR partition */
        tags[0] = PT_POSITION;
        tags[1] = (IPTR)&ppos;
        tags[2] = PT_DOSENVEC;
        tags[3] = (IPTR)&pp[4];
        tags[4] = TAG_DONE;
        GetPartitionAttrs(pn, (struct TagItem *)tags);

        /*
         * 'Active' is not the same as 'Bootable'. Theoretically we can set Active flag for multiple
         * partitions, but this may screw up Microsoft system software which expects to see only one
         * active partition.
         */
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

        D(bug("[Boot] Partition name: %s\n", name));
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

        /* Note that we already have the mount list semaphore */
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

    fsnode = FindFileSystem(table, FST_ID, pp[4 + DE_DOSTYPE], TAG_DONE);
    if (fsnode) {
        D(bug("[Boot] Found on-disk filesystem 0x%08x\n", pp[4 + DE_DOSTYPE]));
        AddBootFileSystem(fsnode);
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

    D(bug("CheckPartition('%b') handler seglist = %x, handler = %s\n", dn->dn_Name,
            dn->dn_SegList, AROS_BSTR_ADDR(dn->dn_Handler)));

    /* Examples:
     * ata.device registers a HDx device describing whole disk with no handler name and no seglist
     * massstorage.class registers each partition giving it a handler name but not seglist
     */

    /* If we already have filesystem handler, don't do anything */
    if (dn->dn_SegList == BNULL && dn->dn_Handler == BNULL)
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

    if (res)
        /* If any partitions were found for the DeviceNode, remove it */
        Remove(&bn->bn_Node);
}

/* Scan all partitions manually for additional volumes that can be mounted. */
void dosboot_BootScan(LIBBASETYPEPTR LIBBASE)
{
    APTR PartitionBase;
    struct BootNode *bootNode, *temp;

    /* If we have partition.library, we can look for partitions */
    PartitionBase = OpenLibrary("partition.library", 2);
    if (PartitionBase)
    {
        ObtainSemaphore(&IntExpBase(ExpansionBase)->eb_BootSemaphore);
        ForeachNodeSafe (&LIBBASE->bm_ExpansionBase->MountList, bootNode, temp)
            CheckPartitions(LIBBASE->bm_ExpansionBase, PartitionBase, SysBase,
                bootNode);
        ReleaseSemaphore(&IntExpBase(ExpansionBase)->eb_BootSemaphore);

	CloseLibrary(PartitionBase);
    }
}
