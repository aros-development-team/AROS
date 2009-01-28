/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <exec/memory.h>

#include <stdlib.h>
#include <string.h>

#include "devices.h"
#include "gui.h"
#include "harddisks.h"

#define DEBUG 0
#include "debug.h"

extern struct GUIGadgets gadgets;

struct ListNode root;

struct HDTBDevice *addDevice(struct ListNode *parent, STRPTR name)
{
    struct HDTBDevice *ln;
    STRPTR seppoint = NULL;
    int devnamelen = 0;

    D(bug("[HDToolBox] addDevice('%s')\n", name));

    seppoint = strstr(name, ":");

    if (seppoint != NULL)
	devnamelen = (int)(seppoint - name);
    else
	devnamelen = strlen(name);
    
    ln = AllocMem(sizeof(struct HDTBDevice), MEMF_PUBLIC | MEMF_CLEAR);
    if (ln)
    {
        ln->listnode.ln.ln_Name = AllocVec(devnamelen+1, MEMF_PUBLIC | MEMF_CLEAR);
        if (ln->listnode.ln.ln_Name)
        {
            if (InitListNode(&ln->listnode, parent))
            {
                ln->listnode.ln.ln_Type = LNT_Device;
                CopyMem(name, ln->listnode.ln.ln_Name, devnamelen);
		D(bug("[HDToolBox] addDevice: device '%s'\n", ln->listnode.ln.ln_Name));

		if (seppoint != NULL)
		{
		    ln->maxunits = atoi(name + devnamelen + 1);
		    D(bug("[HDToolBox] addDevice: maxunits %d\n", ln->maxunits));
		}

                findHDs(ln);

		/*
		 * check if device carries at least one element (empty partition?)
		 */
                if (ln->listnode.list.lh_Head->ln_Succ->ln_Succ)
                    ln->listnode.flags |= LNF_Listable;

		/*
		 * add device to the list
		 */
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
