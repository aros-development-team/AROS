/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/intuition.h>
#include <proto/partition.h>
#include <proto/alib.h>

#include <devices/trackdisk.h>
#include <exec/memory.h>
#include <utility/tagitem.h>

#include <stdio.h>
#include <strings.h>

#include "partitions.h"
#include "hdtoolbox_support.h"
#include "platform.h"
#include "prefs.h"

#define DEBUG 0

#include "debug.h"

void setPartitionName(struct HDTBPartition *pnode)
{
    D(bug("[HDToolBox] setPartitionName()\n"));

    if (pnode->pos != -1)
        sprintf(pnode->listnode.ln.ln_Name, "Partition %d", (int)pnode->pos);
    else
    {
        sprintf
        (
            pnode->listnode.ln.ln_Name,
            "%ld-%ld",
            pnode->de.de_LowCyl,
            pnode->de.de_HighCyl
        );
    }
}

struct HDTBPartition *newPartition(struct ListNode *parent, struct HDTBPartition *partition)
{
    struct HDTBPartition *pn;

    D(bug("[HDToolBox] newPartition()\n"));

    pn = AllocMem(sizeof(struct HDTBPartition), MEMF_PUBLIC | MEMF_CLEAR);
    if (pn)
    {
        pn->listnode.ln.ln_Name = AllocVec(100, MEMF_PUBLIC | MEMF_CLEAR);
        if (pn->listnode.ln.ln_Name)
        {
            if (InitListNode(&pn->listnode, parent))
            {
                pn->listnode.ln.ln_Type = LNT_Partition;
                pn->root = partition;
		D(bug("[HDToolBox] newPartition successful - new partition at %p\n", pn));
                return pn;
            }
            FreeVec(pn->listnode.ln.ln_Name);
        }
        FreeMem(pn, sizeof(struct HDTBPartition));
    }
    D(bug("[HDToolBox] newPartition failed\n"));
    return NULL;
}

void findPartitions(struct ListNode *parent, struct HDTBPartition *partition) 
{
    struct PartitionHandle *ph;
    struct HDTBPartition *pn;
    LONG flag;

    D(bug("[HDToolBox] findPartitions()\n"));

    for
        (
            ph = (struct PartitionHandle *)partition->ph->table->list.lh_Head;
            ph->ln.ln_Succ;
            ph = (struct PartitionHandle *)ph->ln.ln_Succ
        )
    {
        pn = newPartition(parent, partition);
	D(bug("[HDToolBox] - found new partition at %p, created node at %p\n", ph, pn));
        if (pn != NULL)
        {
            pn->ph = ph;
            GetPartitionAttrsA
            (
                pn->ph,
                PT_GEOMETRY, &pn->dg,
                PT_DOSENVEC, &pn->de,
                TAG_DONE
            );
	    
	    /*
	     * check if we can read partition type
	     */
            if (getAttrInfo(pn->root->table->pattrlist, PTA_TYPE) & PLAM_READ)
	    {
                GetPartitionAttrsA(pn->ph, PT_TYPE, &pn->type, TAG_DONE);
		D(bug("[HDToolBox] - ID valid (%ld)\n", pn->type));
	    }
            else
            {
                pn->type.id[0] = 0;
                pn->type.id_len = 0;
		D(bug("[HDToolBox] - ID invalid\n"));
            }

	    /*
	     * read partition location
	     */
            if (getAttrInfo(pn->root->table->pattrlist, PTA_POSITION) & PLAM_READ)
	    {
                GetPartitionAttrsA(pn->ph, PT_POSITION, &pn->pos, TAG_DONE);
		D(bug("[HDToolBox] - Position valid (%ld)\n", pn->pos));
	    }
            else
	    {
                pn->pos = -1L;
		D(bug("[HDToolBox] - Position invalid\n"));
	    }

	    /*
	     * read partition name
	     */
            if (getAttrInfo(pn->root->table->pattrlist, PTA_NAME) & PLAM_READ)
	    {
                GetPartitionAttrsA(pn->ph, PT_NAME, pn->listnode.ln.ln_Name, TAG_DONE);
		D(bug("[HDToolBox] - Name valid (%s)\n", pn->listnode.ln.ln_Name));
	    }
            else
	    {
                setPartitionName(pn);
		D(bug("[HDToolBox] - Name generated (%s)\n", pn->listnode.ln.ln_Name));
	    }

	    /*
	     * check bootable flag
	     */
            if (getAttrInfo(pn->root->table->pattrlist, PTA_BOOTABLE) & PLAM_READ)
            {
                GetPartitionAttrsA(pn->ph, PT_BOOTABLE, &flag, TAG_DONE);
                if (flag)
                    pn->flags |= PNF_BOOTABLE;
		D(bug("[HDToolBox] - Bootable flag %s\n", flag ? "enabled" : "disabled"));
            }

	    /*
	     * automount flag
	     */
            if (getAttrInfo(pn->root->table->pattrlist, PTA_AUTOMOUNT) & PLAM_READ)
            {
                GetPartitionAttrsA(pn->ph, PT_AUTOMOUNT, &flag, TAG_DONE);
                if (flag)
                    pn->flags |= PNF_AUTOMOUNT;
		D(bug("[HDToolBox] - Automount flag %s\n", flag ? "enabled" : "disabled"));
            }

	    /*
	     * check if partition is active (MBR-specific)
	     */
            if (getAttrInfo(pn->root->table->pattrlist, PTA_ACTIVE) & PLAM_READ)
            {
                GetPartitionAttrsA(pn->ph, PT_ACTIVE, &flag, TAG_DONE);
                if (flag)
                    pn->flags |= PNF_ACTIVE;
		D(bug("[HDToolBox] - Active flag %s\n", flag ? "enabled" : "disabled"));
            }

            if (pn->root->table->max_partitions)
            {
                pn->listnode.ln.ln_Pri=pn->root->table->max_partitions-1-pn->pos;
                Enqueue(&parent->list, &pn->listnode.ln);
            }
            else
                AddTail(&parent->list, &pn->listnode.ln);
            if (findPartitionTable(pn))
            {
                findPartitions(&pn->listnode, pn);
                pn->listnode.flags |= LNF_Listable;
            }
        }
    }
    D(bug("[HDToolBox] findPartitions() successful\n"));
}

void freePartitionNode(struct HDTBPartition *node)
{
    D(bug("[HDToolBox] freePartitionNode(%p)\n", node));

    if (node->table)
    {
        D(bug("[HDToolBox] Freeing sub-table %p\n", node->table));
        freePartitionList(&node->listnode.list);
        freePartitionTable(node);
    }
    UninitListNode(&node->listnode);
    FreeVec(node->listnode.ln.ln_Name);
    FreeMem(node, sizeof(struct HDTBPartition));
}

void freePartitionList(struct List *list)
{
    struct HDTBPartition *node;
    struct HDTBPartition *next;

    D(bug("[HDToolBox] freePartitionList(%p)\n", list));

    node = (struct HDTBPartition *)list->lh_Head;
    while (node->listnode.ln.ln_Succ)
    {
        next = (struct HDTBPartition *)node->listnode.ln.ln_Succ;
        if (node->listnode.ln.ln_Type != LNT_Parent)
        {
            Remove(&node->listnode.ln);
            freePartitionNode(node);
        }
        node = next;
    }
}

BOOL validValue(struct HDTBPartition *table, struct HDTBPartition *current, ULONG value)
{
    struct HDTBPartition *pn;
    ULONG spc;

    D(bug("[HDToolBox] validValue()\n"));

    if (value<table->table->reserved)
    {
	D(bug("[HDToolBox] - value (%ld) < table->reserved (%ld) -> bad\n", value, table->table->reserved));
        return FALSE;
    }
    spc = table->dg.dg_Heads*table->dg.dg_TrackSectors;
    if (value>=(table->dg.dg_Cylinders*spc))
    {
	D(bug("[HDToolBox] - value (%ld) >= table->dg.dg_Cylinders*spc (%ld) -> bad\n", value, table->dg.dg_Cylinders*spc));
        return FALSE;
    }

    for (
        pn = (struct HDTBPartition *)table->listnode.list.lh_Head;
	pn->listnode.ln.ln_Succ;
        pn = (struct HDTBPartition *)pn->listnode.ln.ln_Succ)
    {
	D(bug("[HDToolBox] - List %p / Partition %p / Next %p / Current %p\n", &table->listnode, pn, pn->listnode.ln.ln_Succ, current));
	D(bug("[HDToolBox] - Partition Type: %lx\n", pn->type));
        
	/* don't analyze PARENT, don't analyze ROOT */
	if (pn->listnode.ln.ln_Type != LNT_Partition)
	    continue;	

	/* don't check currently processed partition */
        if (current != pn)
        {
            spc = pn->de.de_Surfaces*pn->de.de_BlocksPerTrack;
            if (
                    (value >= (pn->de.de_LowCyl*spc)) &&
                    (value <  (((pn->de.de_HighCyl+1)*spc)-1))
                )
	    {
		D(bug("[HDToolBox] - value (%ld) within bounds <%ld; %ld) -> bad\n", value, pn->de.de_LowCyl * spc, (pn->de.de_HighCyl+1)*spc-1));
                return FALSE;
	    }
        }
    }
    
    D(bug("[HDToolBox] - value (%ld) good\n", value));
    return TRUE;
}

struct HDTBPartition *addPartition(struct HDTBPartition *table, struct DosEnvec *de)
{
    struct HDTBPartition *partition;
    struct HDTBPartition *pn;
    struct TableTypeNode *ttn;
    ULONG leadin = 0, blocks_per_cyl;

    D(bug("[HDToolBox] addPartition()\n"));

    partition = newPartition(&table->listnode, table);
    if (partition)
    {
        if (table->table->max_partitions)
        {
            /* find first free position */
            ULONG count = 0;
            pn = (struct HDTBPartition *)table->listnode.list.lh_Head;
            while (pn->listnode.ln.ln_Succ)
            {
                if (pn->listnode.ln.ln_Type != LNT_Parent)
                {
                    if (count!=pn->pos)
                    {
                        partition->pos = count;
                        break;
                    }
                    count++;
                }
                pn = (struct HDTBPartition *)pn->listnode.ln.ln_Succ;
            }
            if (pn->listnode.ln.ln_Succ == NULL)
                partition->pos = count;
            partition->listnode.ln.ln_Pri = table->table->max_partitions-1-partition->pos;
            Enqueue(&table->listnode.list, &partition->listnode.ln);
        }
        else
            AddTail(&table->listnode.list, &partition->listnode.ln);

        de->de_TableSize   = DE_DOSTYPE;
        de->de_MaxTransfer = 0xFFFFFF;
        de->de_Mask        = 0xFFFFFFFE;
        de->de_Reserved    = 2;
        de->de_BufMemType  = MEMF_PUBLIC;

        CopyMem(de, &partition->de, sizeof(struct DosEnvec));
        ttn = findTableTypeNode(table->table->type);
        CopyMem(&ttn->defaulttype, &partition->type, sizeof(struct PartitionType));
        if (getAttrInfo(table->table->pattrlist, PTA_NAME) & PLAM_WRITE)
            strcpy(partition->listnode.ln.ln_Name, "DH0");
        else
            setPartitionName(partition);

        GetPartitionTableAttrsTags(table->ph, PTT_MAXLEADIN, &leadin, TAG_DONE);

        blocks_per_cyl = de->de_Surfaces * de->de_BlocksPerTrack;
        de->de_LowCyl += (leadin + blocks_per_cyl - 1) / blocks_per_cyl;
	D(bug("[HDToolBox] addPartition() Prepared Envec:\n"));
	D(bug("[HDToolBox] - LeadIn          : %ld\n", leadin));
	D(bug("[HDToolBox] - SizeBlock       : %ld\n", de->de_SizeBlock));
	D(bug("[HDToolBox] - SecOrg          : %ld\n", de->de_SecOrg));
	D(bug("[HDToolBox] - Surfaces        : %ld\n", de->de_Surfaces));
	D(bug("[HDToolBox] - SectorsPerBlock : %ld\n", de->de_SectorPerBlock));
	D(bug("[HDToolBox] - Reserved        : %ld\n", de->de_Reserved));
	D(bug("[HDToolBox] - LowCyl          : %ld\n", de->de_LowCyl));
	D(bug("[HDToolBox] - HighCyl         : %ld\n", de->de_HighCyl));
	D(bug("[HDToolBox] - BlocksPerTrack  : %ld\n", de->de_BlocksPerTrack));
	D(bug("[HDToolBox] - BootBlocks      : %ld\n", de->de_BootBlocks));

        partition->ph = AddPartitionTags(table->ph,
                                         PT_DOSENVEC, de,
                                         PT_TYPE    , &partition->type,
                                         PT_NAME    , partition->listnode.ln.ln_Name,
                                         PT_POSITION, partition->pos,
                                         TAG_DONE);
        if (partition->ph)
        {
            /* We did not set GEOMETRY so partitionlib did it. Update geometry and DOS type in local DOSEnvec */
            GetPartitionAttrsTags(partition->ph, PT_GEOMETRY, &partition->dg, 
                                                 PT_DOSENVEC, &partition->de,
                                                 TAG_DONE);

	    de = &partition->de;
	    D(bug("[HDToolBox] addPartition() Real Envec:\n"));
	    D(bug("[HDToolBox] - SizeBlock       : %ld\n", de->de_SizeBlock));
	    D(bug("[HDToolBox] - SecOrg          : %ld\n", de->de_SecOrg));
	    D(bug("[HDToolBox] - Surfaces        : %ld\n", de->de_Surfaces));
	    D(bug("[HDToolBox] - SectorsPerBlock : %ld\n", de->de_SectorPerBlock));
	    D(bug("[HDToolBox] - Reserved        : %ld\n", de->de_Reserved));
	    D(bug("[HDToolBox] - LowCyl          : %ld\n", de->de_LowCyl));
	    D(bug("[HDToolBox] - HighCyl         : %ld\n", de->de_HighCyl));
	    D(bug("[HDToolBox] - BlocksPerTrack  : %ld\n", de->de_BlocksPerTrack));
	    D(bug("[HDToolBox] - BootBlocks      : %ld\n", de->de_BootBlocks));
	    D(bug("[HDToolBox] addPartition() successful\n"));
	    return partition;
        }

	D(bug("[HDToolBox] addPartition() failed to add partition\n"));
        freePartitionNode(partition);
    }
    D(bug("[HDToolBox] addPartition() failed\n"));
    return NULL;
}
