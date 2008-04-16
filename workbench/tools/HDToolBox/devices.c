/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>

#include <proto/exec.h>
#include <exec/memory.h>
#include <string.h>

#include "devices.h"
#include "gui.h"
#include "harddisks.h"

#include "debug.h"

extern struct GUIGadgets gadgets;

struct ListNode root;

struct HDTBDevice *addDevice(struct ListNode *parent, STRPTR name)
{
    struct HDTBDevice *ln;

    D(bug("[HDToolBox] addDevice('%s')\n", name));

    ln = AllocMem(sizeof(struct HDTBDevice), MEMF_PUBLIC | MEMF_CLEAR);
    if (ln)
    {
        ln->listnode.ln.ln_Name = AllocVec(strlen(name)+1, MEMF_PUBLIC);
        if (ln->listnode.ln.ln_Name)
        {
            if (InitListNode(&ln->listnode, parent))
            {
                ln->listnode.ln.ln_Type = LNT_Device;
                CopyMem(name, ln->listnode.ln.ln_Name, strlen(name)+1);
                findHDs(&ln->listnode);
                if (ln->listnode.list.lh_Head->ln_Succ->ln_Succ)
                    ln->listnode.flags |= LNF_Listable;
                InsertList(gadgets.leftlv, &ln->listnode);
                AddTail(&parent->list, &ln->listnode.ln);
                return ln;
            }
            FreeVec(ln->listnode.ln.ln_Name);
        }
        FreeMem(ln, sizeof(struct HDTBDevice));
    }
    return NULL;
}

struct HDTBDevice *addDeviceName(STRPTR name) 
{
    D(bug("[HDToolBox] addDeviceName('%s')\n", name));

    return addDevice(&root, name);
}

void freeDeviceNode(struct HDTBDevice *node)
{
    D(bug("[HDToolBox] freeDeviceNode()\n"));

    freeHDList(&node->listnode.list);
    FreeVec(node->listnode.ln.ln_Name);
    FreeMem(node, sizeof(struct HDTBDevice));
}

void freeDeviceList(void)
{
    struct HDTBDevice *node;
    struct HDTBDevice *next;

    D(bug("[HDToolBox] freeDeviceList()\n"));

    node = (struct HDTBDevice *)root.list.lh_Head;
    while (node->listnode.ln.ln_Succ)
    {
        next = (struct HDTBDevice *)node->listnode.ln.ln_Succ;
        if (node->listnode.ln.ln_Type != LNT_Parent)
        {
            Remove(&node->listnode.ln);
            freeDeviceNode(node);
        }
        node = next;
    }
    UninitListNode(&root);
}
