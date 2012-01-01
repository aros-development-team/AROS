/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Workbook Virtual Area Class
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
#include <proto/layers.h>

#include <intuition/classusr.h>
#include <intuition/icclass.h>
#include <intuition/cghooks.h>
#include <libraries/gadtools.h>

#include "workbook_intern.h"
#include "classes.h"

static inline WORD max(WORD a, WORD b)
{
    return (a > b) ? a : b;
}

struct wbVirtual {
    Object        *Gadget;
    struct IBox    Virt;     /* Virtual pos in, and total size of the scroll area */
};

static BOOL wbRedimension(Class *cl, Object *obj, WORD vwidth, WORD vheight)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbVirtual *my = INST_DATA(cl, obj);
    struct Gadget *gadget = (struct Gadget *)obj;
    BOOL rc = FALSE;

    if ((my->Virt.Width != vwidth) ||
    	(my->Virt.Height != vheight))
    	rc = TRUE;

    my->Virt.Width = vwidth;
    my->Virt.Height = vheight;

    if (my->Virt.Left > (my->Virt.Width - gadget->Width)) {
    	my->Virt.Left = max(0, my->Virt.Width - gadget->Width);
    	rc = TRUE;
    }

    if (my->Virt.Top > (my->Virt.Height - gadget->Height)) {
    	my->Virt.Top  = max(0, my->Virt.Height - gadget->Height);
    	rc = TRUE;
    }

    D(bug("WBVirtual: wbRedimension(%d,%d) = (%d,%d) %dx%d\n",
    		vwidth,vheight,my->Virt.Left,my->Virt.Top,
    		my->Virt.Width, my->Virt.Height));
    D(bug("WBVirtual: Frame at: (%d,%d) %dx%d\n",
    		gadget->TopEdge, gadget->LeftEdge,
    		gadget->Width, gadget->Height));

    if (my->Gadget) {
    	SetAttrs(my->Gadget, GA_Top,   gadget->TopEdge - my->Virt.Top,
    			     GA_Left,  gadget->LeftEdge - my->Virt.Left,
    			     TAG_END);
    }

    return rc;
}

static BOOL wbMoveTo(Class *cl, Object *obj, WORD left, WORD top)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbVirtual *my = INST_DATA(cl, obj);
    struct Gadget *gadget = (struct Gadget *)obj;
    WORD dLeft, dTop;

    D(bug("GadgetSize %dx%d\n", gadget->Width, gadget->Width));
    D(bug("  VirtSize %dx%d\n", my->Virt.Width, my->Virt.Height));
    D(bug("  wbMoveTo(%d,%d) =", left,top));

    if (left > (my->Virt.Width - gadget->Width))
    	left = max(0, my->Virt.Width - gadget->Width);
    if (top > (my->Virt.Height - gadget->Height))
    	top = max(0, my->Virt.Height - gadget->Height);

    dLeft = left - my->Virt.Left;
    dTop  = top  - my->Virt.Top;

    D(bug(" (%d,%d) %dx%d\n",
    		left, top,
    		my->Virt.Width, my->Virt.Height));

    if (dLeft == 0 && dTop == 0)
    	return FALSE;

    my->Virt.Left = left;
    my->Virt.Top  = top;

    D(bug("Frame at: (%d,%d) %dx%d\n",
    		gadget->TopEdge, gadget->LeftEdge,
    		gadget->Width, gadget->Height));

    /* Set the position of the child */
    if (my->Gadget) {
    	SetAttrs(my->Gadget, GA_Top, gadget->TopEdge - my->Virt.Top,
    			     GA_Left, gadget->LeftEdge - my->Virt.Left);
    }

    return TRUE;
}


static IPTR WBVirtualNew(Class *cl, Object *obj, struct opSet *ops)
{
    IPTR rc = 0;

    rc = DoSuperMethodA(cl, obj, (Msg)ops);
    if (rc == 0)
    	return rc;

    obj = (Object *)rc;

    DoMethod(obj, OM_SET, ops->ops_AttrList, ops->ops_GInfo);

    return rc;
}

// OM_GET
static IPTR WBVirtualGet(Class *cl, Object *obj, struct opGet *opg)
{
    struct wbVirtual *my = INST_DATA(cl, obj);
    IPTR rc = TRUE;

    switch (opg->opg_AttrID) {
    case WBVA_Gadget:
    	*(opg->opg_Storage) = (IPTR)(my->Gadget);
    	break;
    case WBVA_VirtLeft:
    	*(opg->opg_Storage) = (IPTR)(SIPTR)(my->Virt.Left);
    	break;
    case WBVA_VirtTop:
    	*(opg->opg_Storage) = (IPTR)(SIPTR)(my->Virt.Top);
    	break;
    case WBVA_VirtWidth:
    	*(opg->opg_Storage) = (IPTR)(SIPTR)(my->Virt.Width);
    	break;
    case WBVA_VirtHeight:
    	*(opg->opg_Storage) = (IPTR)(SIPTR)(my->Virt.Height);
    	break;
    default:
    	rc = DoSuperMethodA(cl, obj, (Msg)opg);
    	break;
    }

    return rc;
}

// OM_UPDATE/OM_SET
static IPTR WBVirtualSetUpdate(Class *cl, Object *obj, struct opUpdate *opu)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbVirtual *my = INST_DATA(cl, obj);
    struct TagItem *tag;
    struct TagItem *tstate;
    IPTR rc;
    WORD val;

    rc = DoSuperMethodA(cl, obj, (Msg)opu);

    if ((opu->MethodID == OM_UPDATE) && (opu->opu_Flags & OPUF_INTERIM))
    	return rc;

    tstate = opu->opu_AttrList;
    while ((tag = NextTagItem(&tstate))) {
    	val = (WORD)tag->ti_Data;
D(bug("%s: Tag=0x%x, val=%d\n", __func__, tag->ti_Tag, val));
    	switch (tag->ti_Tag) {
    	case WBVA_Gadget:
    	    if (my->Gadget != (Object *)tag->ti_Data) {
    	    	my->Gadget = (Object *)tag->ti_Data;
    	    	IPTR vwidth = 0, vheight = 0;
    	    	GetAttr(GA_Width, my->Gadget, &vwidth);
    	    	GetAttr(GA_Height, my->Gadget, &vheight);
    	    	rc |= wbRedimension(cl, obj, vwidth, vheight);
    	    }
    	    break;
    	case WBVA_VirtTop:
    	    rc |= wbMoveTo(cl, obj, my->Virt.Left, val);
    	    break;
    	case WBVA_VirtLeft:
    	    rc |= wbMoveTo(cl, obj, val, my->Virt.Top);
    	    break;
    	case WBVA_VirtHeight:
    	    rc |= wbRedimension(cl, obj, my->Virt.Width, val);
    	    break;
    	case WBVA_VirtWidth:
    	    rc |= wbRedimension(cl, obj, val, my->Virt.Height);
    	    break;
    	case GA_Width:
    	case GA_Height:
    	    rc |= wbRedimension(cl, obj, my->Virt.Width, my->Virt.Height);
    	    break;
    	default:
    	    break;
    	}
    }

    return rc;
}

// GM_HITTEST
static IPTR WBVirtualHitTest(Class *cl, Object *obj, struct gpHitTest *gph)
{
    struct wbVirtual *my = INST_DATA(cl, obj);
    struct gpHitTest subtest = *gph;

    if (!my->Gadget)
    	return 0;

    /* Forward to our client */
    subtest.gpht_Mouse.X += my->Virt.Left;
    subtest.gpht_Mouse.Y += my->Virt.Top;

    return DoMethodA(my->Gadget, (Msg)&subtest);
}

// GM_RENDER
static IPTR WBVirtualRender(Class *cl, Object *obj, struct gpRender *gpr)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbVirtual *my = INST_DATA(cl, obj);
    struct Gadget *gadget = (struct Gadget *)obj;
    struct GadgetInfo *ginfo = gpr->gpr_GInfo;
   
    if (my->Gadget == NULL)
    	return FALSE;

    if (ginfo == NULL)
    	return FALSE;

    /* Redraw the child */
    struct Region *region, *old;
    if ((region = NewRegion())) {
    	struct Rectangle rect = {
    	    .MinX = gadget->LeftEdge,
    	    .MinY = gadget->TopEdge,
    	    .MaxX = gadget->LeftEdge + gadget->Width - 1,
    	    .MaxY = gadget->TopEdge  + gadget->Height - 1,
    	};
    	OrRectRegion(region, &rect);
    	old = InstallClipRegion(ginfo->gi_Layer, region);
    	AddGadget(ginfo->gi_Window, (struct Gadget *)my->Gadget, 0);
    	RefreshGList((struct Gadget *)my->Gadget, ginfo->gi_Window, ginfo->gi_Requester, 1);
    	RemoveGadget(ginfo->gi_Window, (struct Gadget *)my->Gadget);
    	InstallClipRegion(ginfo->gi_Layer, old);
    	DisposeRegion(region);
    }

    return TRUE;
}

// GM_GOACTIVE
static IPTR WBVirtualGoActive(Class *cl, Object *obj, struct gpInput *gpi)
{
    struct wbVirtual *my = INST_DATA(cl, obj);
    struct gpInput m;

    m = *gpi;
    m.gpi_Mouse.X += my->Virt.Left;
    m.gpi_Mouse.Y += my->Virt.Top;

    return DoMethodA(my->Gadget, (Msg)&m);
}

// GM_GOINACTIVE
static IPTR WBVirtualGoInactive(Class *cl, Object *obj, struct gpGoInactive *gpgi)
{
    struct wbVirtual *my = INST_DATA(cl, obj);

    return DoMethodA(my->Gadget, (Msg)gpgi);
}

// GM_HANDLEINPUT
static IPTR WBVirtualHandleInput(Class *cl, Object *obj, struct gpInput *gpi)
{
    struct wbVirtual *my = INST_DATA(cl, obj);
    struct gpInput m;

    m = *gpi;
    m.gpi_Mouse.X += my->Virt.Left;
    m.gpi_Mouse.Y += my->Virt.Top;

    return DoMethodA(my->Gadget, (Msg)&m);
}

static IPTR dispatcher(Class *cl, Object *obj, Msg msg)
{
    IPTR rc = 0;

    switch (msg->MethodID) {
    case OM_NEW:     rc = WBVirtualNew(cl, obj, (APTR)msg); break;
    case OM_GET:     rc = WBVirtualGet(cl, obj, (APTR)msg); break;
    case OM_SET:
    case OM_UPDATE:  rc = WBVirtualSetUpdate(cl, obj, (APTR)msg); break;
    case GM_RENDER:  rc = WBVirtualRender(cl, obj, (APTR)msg); break;
    case GM_HITTEST: rc = WBVirtualHitTest(cl, obj, (APTR)msg); break;
    case GM_GOACTIVE: rc = WBVirtualGoActive(cl, obj, (APTR)msg); break;
    case GM_GOINACTIVE: rc = WBVirtualGoInactive(cl, obj, (APTR)msg); break;
    case GM_HANDLEINPUT: rc = WBVirtualHandleInput(cl, obj, (APTR)msg); break;
    default:        rc = DoSuperMethodA(cl, obj, msg); break;
    }

    return rc;
}

Class *WBVirtual_MakeClass(struct WorkbookBase *wb)
{
    Class *cl;

    cl = MakeClass( NULL, "gadgetclass", NULL,
                    sizeof(struct wbVirtual),
                    0);
    if (cl != NULL) {
    	cl->cl_Dispatcher.h_Entry = HookEntry;
    	cl->cl_Dispatcher.h_SubEntry = dispatcher;
    	cl->cl_Dispatcher.h_Data = NULL;
    	cl->cl_UserData = (IPTR)wb;
    }

    return cl;
}
