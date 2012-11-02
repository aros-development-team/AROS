/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/partition.h>

#include <devices/scsidisk.h>
#include <exec/io.h>
#include <exec/memory.h>

#include <stdio.h>

#define DEBUG 0
#include "debug.h"

#include "harddisks.h"
#include "hdtoolbox_support.h"
#include "platform.h"

extern struct GUIGadgets gadgets;

void w2strcpy(STRPTR name, UWORD *wstr, ULONG len)
{
    STRPTR p = name;

    while (len)
    {
        *((UWORD *)p) = AROS_BE2WORD(*wstr);
        p += sizeof(UWORD);
        len -= 2;
        wstr++;
    }
    p -= 2;
    while (p >= name && (*p == 0 || *p == ' '))
        *p-- = 0;
}

BOOL identify(struct IOStdReq *ioreq, STRPTR name)
{
    struct SCSICmd scsicmd = {0};
    UWORD data[256];
    UBYTE cmd = 0xEC; /* identify */

    D(bug("[HDToolBox] identify('%s')\n", name));

    scsicmd.scsi_Data = data;
    scsicmd.scsi_Length = 512;
    scsicmd.scsi_Command = &cmd;
    scsicmd.scsi_CmdLength = 1;
    ioreq->io_Command = HD_SCSICMD;
    ioreq->io_Data = &scsicmd;
    ioreq->io_Length = sizeof(struct SCSICmd);

    if (DoIO((struct IORequest *)ioreq))
        return FALSE;

    w2strcpy(name, &data[27], 40);
    return TRUE;
}

void findHDs(struct HDTBDevice *parent)
{
    struct IOStdReq *ioreq;
    struct MsgPort *mp;
    struct HDNode *node;
    int i;

    D(bug("[HDToolBox] findHDs()\n"));

    mp = CreateMsgPort();
    if (mp)
    {
        ioreq = (struct IOStdReq *)CreateIORequest(mp, sizeof(struct IOStdReq));
        if (ioreq)
        {
	    int maxunits = 8;
	    if (parent->maxunits > maxunits)
		maxunits = parent->maxunits;

            for (i=0;i<maxunits;i++)
            {
                node = AllocMem(sizeof(struct HDNode), MEMF_PUBLIC | MEMF_CLEAR);
                if (node)
                {
                    node->root_partition.listnode.ln.ln_Name = AllocVec(100, MEMF_PUBLIC | MEMF_CLEAR);
                    if (node->root_partition.listnode.ln.ln_Name)
                    {
                        if (InitListNode(&node->root_partition.listnode, (struct ListNode *)parent))
                        {
                            node->root_partition.listnode.ln.ln_Type = LNT_Harddisk;
                            node->unit = i;
                            if (OpenDevice(parent->listnode.ln.ln_Name, i, (struct IORequest *)ioreq, 0) == 0)
                            {
                                D(bug("[HDToolBox] findHDs: Opened %s:%d\n",parent->listnode.ln.ln_Name, i));
                                if (!identify(ioreq, node->root_partition.listnode.ln.ln_Name))
                                    sprintf(node->root_partition.listnode.ln.ln_Name, "Unit %d", i);
                                CloseDevice((struct IORequest *)ioreq);
                                node->root_partition.ph = OpenRootPartition(parent->listnode.ln.ln_Name, node->unit);
                                if (node->root_partition.ph)
                                {
				    D(bug("[HDToolBox] - appending ROOT partition %p to list %p\n", &node->root_partition.listnode.ln, &parent->listnode.list));
				    D(bug("[HDToolBox] - first entry at %p\n", node->root_partition.listnode.list.lh_Head));
                                    AddTail(&parent->listnode.list, &node->root_partition.listnode.ln);
                                    if (findPartitionTable(&node->root_partition))
                                    {
					D(bug("[HDToolBox] - partition table found. searching for partitions\n"));
                                        findPartitions(&node->root_partition.listnode, &node->root_partition);
                                        node->root_partition.listnode.flags |= LNF_Listable;
                                    }
				    else
				    {
					D(bug("[HDToolBox] - partition table not found.\n"));
				    }
                                    GetPartitionAttrsA
                                    (
                                        node->root_partition.ph,
                                        PT_GEOMETRY, &node->root_partition.dg,
                                        PT_DOSENVEC, &node->root_partition.de,
                                        TAG_DONE
                                    );
                                    continue;
                                }
                            }
                            UninitListNode(&node->root_partition.listnode);
                        }
                        FreeVec(node->root_partition.listnode.ln.ln_Name);
                    }
                    FreeMem(node, sizeof(struct HDNode));
                }
            }
            DeleteIORequest((struct IORequest *)ioreq);
        }
        DeleteMsgPort(mp);
    }
    D(bug("[HDToolBox] findHDs() successful\n"));
}

void freeHDList(struct List *list)
{
    struct HDNode *node;
    struct HDNode *next;

    D(bug("[HDToolBox] freeHDList(%p)\n", list));

    node = (struct HDNode *)list->lh_Head;
    while (node->root_partition.listnode.ln.ln_Succ)
    {
        next = (struct HDNode *)node->root_partition.listnode.ln.ln_Succ;
        if (node->root_partition.listnode.ln.ln_Type != LNT_Parent)
        {
            Remove(&node->root_partition.listnode.ln);
            if (node->root_partition.table)
            {
                freePartitionList(&node->root_partition.listnode.list);
                freePartitionTable(&node->root_partition);
            }
            CloseRootPartition(node->root_partition.ph);
            UninitListNode(&node->root_partition.listnode);
            FreeVec(node->root_partition.listnode.ln.ln_Name);
            FreeMem(node, sizeof(struct HDNode));
        }
        node = next;
    }
}
