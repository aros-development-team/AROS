#include <string.h>

#include <proto/exec.h>
#include <exec/memory.h>

#include "devices.h"
#include "gui.h"
#include "harddisks.h"
#define DEBUG 1
#include "debug.h"

extern struct GUIGadgets gadgets;
struct ListNode root;

struct HDTBDevice *addDevice(struct ListNode *parent, STRPTR name) {
struct HDTBDevice *ln;

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

struct HDTBDevice *addDeviceName(STRPTR name) {
	return addDevice(&root, name);
}

void freeDeviceNode(struct HDTBDevice *node) {
	freeHDList(&node->listnode.list);
	FreeVec(node->listnode.ln.ln_Name);
	FreeMem(node, sizeof(struct HDTBDevice));
}

void freeDeviceList(void) {
struct HDTBDevice *node;
struct HDTBDevice *next;

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

