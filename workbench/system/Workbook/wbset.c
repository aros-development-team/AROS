/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Workbook Icon Set Class
    Lang: english
*/

#define DEBUG 0
#include <aros/debug.h>

#include <string.h>
#include <limits.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/gadtools.h>
#include <proto/workbench.h>
#include <proto/graphics.h>

#include <intuition/classusr.h>
#include <libraries/gadtools.h>

#include "workbook_intern.h"
#include "classes.h"

struct wbSetNode {
    struct MinNode sn_Node;
    Object        *sn_Object;      /* Gadget object */
};

struct wbSet {
    WORD MaxWidth;
    struct List FixedObjects;
    struct List AutoObjects;
};

static void wbGABox(struct WorkbookBase *wb, Object *obj, struct IBox *box)
{
    struct Gadget *gadget = (struct Gadget *)obj;

    box->Top = gadget->TopEdge;
    box->Left = gadget->LeftEdge;
    box->Width = gadget->Width;
    box->Height = gadget->Height;
}

static void rearrange(Class *cl, Object *obj)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbSet *my = INST_DATA(cl, obj);
    struct wbSetNode *node;
    struct IBox sbox;
    WORD CurrRight, CurrBottom;

    /* First, remove all autoobjects from the superclass */
    ForeachNode(&my->AutoObjects, node) {
    	DoSuperMethod(cl, obj, OM_REMMEMBER, node->sn_Object);
    }

    /* Find the set size with just the fixed objects */
    wbGABox(wb, obj, &sbox);

    /* Set the start of the auto area to be
     * immediately below the fixed objects.
     */
    CurrRight = sbox.Left;
    CurrBottom = sbox.Top + sbox.Height;

    /* For each item in the auto list, add it to the right */
    ForeachNode(&my->AutoObjects, node) {
    	Object *iobj = node->sn_Object;
    	struct IBox ibox;

    	wbGABox(wb, iobj, &ibox);

    	if ((CurrRight + ibox.Width) < my->MaxWidth) {
    	    ibox.Left = CurrRight;
    	} else {
    	    wbGABox(wb, obj, &sbox);
    	    ibox.Left = sbox.Left;
    	    CurrRight = sbox.Left;
    	    CurrBottom = sbox.Top + sbox.Height;
    	}
    	ibox.Top  = CurrBottom;
    	CurrRight += ibox.Width;

    	D(bug("New icon position: @%d,%d\n", ibox.Left, ibox.Top));

    	SetAttrs(iobj, GA_Left, ibox.Left, GA_Top, ibox.Top, TAG_END);

    	DoSuperMethod(cl, obj, OM_ADDMEMBER, iobj);
    }
}

// OM_ADDMEMBER
static IPTR WBSetAddMember(Class *cl, Object *obj, struct opMember *opm)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    Object *iobj = opm->opam_Object;
    struct IBox ibox;
    struct wbSet *my = INST_DATA(cl, obj);
    struct wbSetNode *node;
    IPTR rc;

    node = AllocMem(sizeof(*node), MEMF_ANY);
    node->sn_Object = iobj;

    /* Get bounding box of item to add */
    wbGABox(wb, iobj, &ibox);

    if (ibox.Left == ~0 ||
    	ibox.Top == ~0) {
    	AddHead(&my->AutoObjects, (struct Node *)&node->sn_Node);
    } else {
    	AddHead(&my->FixedObjects, (struct Node *)&node->sn_Node);
    }

    rc = DoSuperMethodA(cl, obj, (Msg)opm);

    /* Recalculate the set's positions */
    rearrange(cl, obj);

    return rc;
}

static IPTR WBSetRemMember(Class *cl, Object *obj, struct opMember *opm)
{
    Object *iobj = opm->opam_Object;
    struct wbSet *my = INST_DATA(cl, obj);
    struct wbSetNode *node, *next;
    IPTR rc;

    rc = DoSuperMethodA(cl, obj, (Msg)opm);

    ForeachNodeSafe(&my->FixedObjects, node, next) {
    	if (node->sn_Object == iobj) {
    	    Remove((struct Node *)node);
    	    FreeMem(node, sizeof(*node));
    	}
    }

    ForeachNodeSafe(&my->AutoObjects, node, next) {
    	if (node->sn_Object == iobj) {
    	    Remove((struct Node *)node);
    	    FreeMem(node, sizeof(*node));
    	}
    }

    /* Recalculate the set's positions */
    rearrange(cl, obj);

    return rc;
}

// OM_NEW
static IPTR WBSetNew(Class *cl, Object *obj, struct opSet *ops)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbSet *my;
    IPTR rc;

    rc = DoSuperMethodA(cl, obj, (Msg)ops);
    if (rc == 0)
    	return 0;

    my = INST_DATA(cl, rc);

    my->MaxWidth = (WORD)GetTagData(WBSA_MaxWidth, 0, ops->ops_AttrList);

    NEWLIST(&my->FixedObjects);
    NEWLIST(&my->AutoObjects);

    return rc;
}

static IPTR WBSetGet(Class *cl, Object *obj, struct opGet *opg)
{
    struct wbSet *my = INST_DATA(cl, obj);
    IPTR rc = TRUE;

    switch (opg->opg_AttrID) {
    case WBSA_MaxWidth:
    	*(opg->opg_Storage) = (IPTR)my->MaxWidth;
    	break;
    default:
    	rc = DoSuperMethodA(cl, obj, (Msg)opg);
    	break;
    }

    return rc;
}

// OM_SET/OM_UPDATE
static IPTR WBSetUpdate(Class *cl, Object *obj, struct opUpdate *opu)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbSet *my = INST_DATA(cl, obj);
    IPTR rc;
    struct TagItem *tag;
    struct TagItem *tstate;

    rc = DoSuperMethodA(cl, obj, (Msg)opu);

    if ((opu->MethodID == OM_UPDATE) && (opu->opu_Flags & OPUF_INTERIM))
    	return rc;

    tstate = opu->opu_AttrList;
    while ((tag = NextTagItem(&tstate))) {
    	switch (tag->ti_Tag) {
    	case WBSA_MaxWidth:
    	    my->MaxWidth = (WORD)tag->ti_Data;
    	    rearrange(cl, obj);
    	    rc |= TRUE;
    	default:
    	    break;
    	}
    }

    return rc;
}



// OM_DISPOSE
static IPTR WBSetDispose(Class *cl, Object *obj, Msg msg)
{
    struct wbSet *my = INST_DATA(cl, obj);
    struct wbSetNode *node, *next;

    /* Remove all the nodes */
    ForeachNodeSafe(&my->FixedObjects, node, next) {
    	Remove((struct Node *)node);
    	FreeMem(node, sizeof(*node));
    }

    ForeachNodeSafe(&my->AutoObjects, node, next) {
    	Remove((struct Node *)node);
    	FreeMem(node, sizeof(*node));
    }

    return DoSuperMethodA(cl, obj, msg);
}

// GM_RENDER
static IPTR WBSetRender(Class *cl, Object *obj, struct gpRender *gpr)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct IBox box;

    wbGABox(wb, obj, &box);

    /* Clear the area first */
    EraseRect(gpr->gpr_RPort, box.Left, box.Top,
              box.Left + box.Width - 1,
              box.Top  + box.Height - 1);

    return DoSuperMethodA(cl, obj, (Msg)gpr);
}


static IPTR dispatcher(Class *cl, Object *obj, Msg msg)
{
    IPTR rc = 0;

    switch (msg->MethodID) {
    case OM_NEW:        rc = WBSetNew(cl, obj, (APTR)msg); break;
    case OM_DISPOSE:    rc = WBSetDispose(cl, obj, (APTR)msg); break;
    case OM_GET:        rc = WBSetGet(cl, obj, (APTR)msg); break;
    case OM_SET:        rc = WBSetUpdate(cl, obj, (APTR)msg); break;
    case OM_UPDATE:     rc = WBSetUpdate(cl, obj, (APTR)msg); break;
    case OM_ADDMEMBER:  rc = WBSetAddMember(cl, obj, (APTR)msg); break;
    case OM_REMMEMBER:  rc = WBSetRemMember(cl, obj, (APTR)msg); break;
    case GM_RENDER: rc = WBSetRender(cl, obj, (APTR)msg); break;
    default:        rc = DoSuperMethodA(cl, obj, msg); break;
    }

    return rc;
}

Class *WBSet_MakeClass(struct WorkbookBase *wb)
{
    Class *cl;

    cl = MakeClass( NULL, "groupgclass", NULL,
                    sizeof(struct wbSet),
                    0);
    if (cl != NULL) {
    	cl->cl_Dispatcher.h_Entry = HookEntry;
    	cl->cl_Dispatcher.h_SubEntry = dispatcher;
    	cl->cl_Dispatcher.h_Data = NULL;
    	cl->cl_UserData = (IPTR)wb;
    }

    return cl;
}
