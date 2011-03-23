/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Workbook Icon Class
    Lang: english
*/

#define DEBUG 0
#include <string.h>

#include <aros/debug.h>

#include <proto/icon.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/graphics.h>
#include <proto/workbench.h>

#include <intuition/cghooks.h>

#include "workbook_intern.h"
#include "classes.h"

struct wbIcon {
    STRPTR             File;
    struct DiskObject *Icon;
    STRPTR             Label;
};

void wbIcon_Update(Class *cl, Object *obj)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbIcon *my = INST_DATA(cl, obj);
    struct Rectangle rect;
    UWORD w,h;
    struct RastPort rp;

    /* Update the parent's idea of how big we are
     */
    InitRastPort(&rp);
    GetIconRectangle(&rp, my->Icon, (STRPTR)my->Label, &rect,
	ICONDRAWA_Borderless, FALSE,
	TAG_END);
    DeinitRastPort(&rp);

    w = (rect.MaxX - rect.MinX) + 1;
    h = (rect.MaxY - rect.MinY) + 1;

//    D(bug("%s: %dx%d @%d,%d\n", my->File, (int)w, (int)h, (int)my->Icon->do_CurrentX, (int)my->Icon->do_CurrentY));
    SetAttrs(obj,
    	GA_Left, my->Icon->do_CurrentX,
    	GA_Top, my->Icon->do_CurrentY,
    	GA_Width, w,
    	GA_Height, h,
    	TAG_END);
}

// OM_NEW
static IPTR wbIconNew(Class *cl, Object *obj, struct opSet *ops)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbIcon *my;
    CONST_STRPTR file, label = "???";

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)ops);
    if (obj == NULL)
    	return 0;

    my = INST_DATA(cl, obj);

    my->File = NULL;
    my->Icon = (struct DiskObject *)GetTagData(WBIA_Icon, (IPTR)NULL, ops->ops_AttrList);
    if (my->Icon != NULL) {
    	if (my->Icon->do_Gadget.GadgetText != NULL &&
    	    my->Icon->do_Gadget.GadgetText->IText != NULL)
    	    label = my->Icon->do_Gadget.GadgetText->IText;
    } else {
	file = (CONST_STRPTR)GetTagData(WBIA_File, (IPTR)NULL, ops->ops_AttrList);
	if (file == NULL)
	    goto error;

	my->File = StrDup(file);
	if (my->File == NULL)
	    goto error;

	strcpy(my->File, file);

	my->Icon = GetDiskObjectNew(my->File);
	if (my->Icon == NULL)
	    goto error;

	label = FilePart(my->File);
    }

    my->Label = StrDup((CONST_STRPTR)GetTagData(WBIA_Label, label, ops->ops_AttrList));
    if (my->Label == NULL)
    	goto error;

    wbIcon_Update(cl, obj);

    return (IPTR)obj;

error:
    if (my->File)
    	FreeVec(my->File);
    DoSuperMethod(cl, obj, OM_DISPOSE);
    return 0;
}

// OM_DISPOSE
static IPTR wbIconDispose(Class *cl, Object *obj, Msg msg)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbIcon *my = INST_DATA(cl, obj);
#if 0
    struct TagItem tags[] = {
	{ ICONPUTA_OnlyUpdatePosition, TRUE },
	{ TAG_END } };

    /* If need be, update the on-disk Icon's position information */
    PutIconTagList(my->File, my->Icon, tags);
#endif

    /* If my->File is set, then we allocated it
     * and my->Icon. Otherwise, Icon was passed in
     * via WBIA_Icon, and its the caller's responsibility.
     */
    if (my->File) {
    	FreeVec(my->File);
	if (my->Icon)
	    FreeDiskObject(my->Icon);
    }

    if (my->Label)
    	FreeVec(my->Label);

    return DoSuperMethodA(cl, obj, msg);
}

// OM_GET
static IPTR wbIconGet(Class *cl, Object *obj, struct opGet *opg)
{
    struct wbIcon *my = INST_DATA(cl, obj);
    IPTR rc = FALSE;

    switch (opg->opg_AttrID) {
    case WBIA_File:
    	*(opg->opg_Storage) = (IPTR)my->File;
    	rc = TRUE;
    	break;
    default:
    	rc = DoSuperMethodA(cl, obj, (Msg)opg);
    	break;
    }

    return rc;
}

// GM_RENDER
static IPTR wbIconRender(Class *cl, Object *obj, struct gpRender *gpr)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbIcon *my = INST_DATA(cl, obj);
    struct RastPort *rp = gpr->gpr_RPort;
    struct Gadget *gadget = (struct Gadget *)obj;	/* Legal for 'gadgetclass' */
    WORD x,y;
    struct TagItem tags[] = {
	{ ICONDRAWA_DrawInfo, (IPTR)gpr->gpr_GInfo->gi_DrInfo, },
	{ ICONDRAWA_Frameless, FALSE, },
	{ TAG_END } };

    x = gadget->LeftEdge;
    y = gadget->TopEdge;

    EraseRect(rp, 
    	    gadget->LeftEdge, gadget->TopEdge,
    	    gadget->LeftEdge + gadget->Width - 1,
    	    gadget->TopEdge + gadget->Height - 1);

    /* FIXME: Setting the icon's text color this way doesn't feel right...
     */
    SetAPen(rp, 1);
    SetBPen(rp, 0);
    SetOutlinePen(rp, 2);

    DrawIconStateA(rp, my->Icon, (STRPTR)my->Label, x, y,
    	(gadget->Flags & GFLG_SELECTED) ? TRUE : FALSE, tags);

    return 0;
}

// GM_GOACTIVE
static IPTR wbIconGoActive(Class *cl, Object *obj, struct gpInput *gpi)
{
    struct Gadget *gadget = (struct Gadget *)obj;

    gadget->Flags |= GFLG_SELECTED;

    DoMethod(obj, GM_RENDER, gpi->gpi_GInfo, gpi->gpi_GInfo->gi_RastPort,GREDRAW_TOGGLE);

    return GMR_MEACTIVE;
}

// GM_GOINACTIVE
static IPTR wbIconGoInactive(Class *cl, Object *obj, struct gpGoInactive *gpgi)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbIcon *my = INST_DATA(cl, obj);
    struct Gadget *gadget = (struct Gadget *)obj;
    struct TagItem tags[] = {
	{ NP_Seglist,     (IPTR)wb->wb_OpenerSegList },
	{ NP_Arguments,   (IPTR)my->File },
	{ NP_FreeSeglist, FALSE },
	{ TAG_END, 0 },
    };

    gadget->Flags &= ~GFLG_SELECTED;

    DoMethod(obj, GM_RENDER, gpgi->gpgi_GInfo, gpgi->gpgi_GInfo->gi_RastPort,GREDRAW_TOGGLE);

    CreateNewProc(tags);

    return GMR_NOREUSE;
}

// GM_HANDLEINPUT
static IPTR wbIconHandleInput(Class *cl, Object *obj, struct gpInput *gpi)
{
    struct InputEvent *iev = gpi->gpi_IEvent;

    if ((iev->ie_Class == IECLASS_RAWMOUSE) &&
        (iev->ie_Code  == SELECTUP))
        return GMR_REUSE;

    return GMR_MEACTIVE;
}


static IPTR dispatcher(Class *cl, Object *obj, Msg msg)
{
    IPTR rc = 0;

    switch (msg->MethodID) {
    case OM_NEW:     rc = wbIconNew(cl, obj, (APTR)msg); break;
    case OM_DISPOSE: rc = wbIconDispose(cl, obj, (APTR)msg); break;
    case OM_GET:     rc = wbIconGet(cl, obj, (APTR)msg); break;
    case GM_RENDER:  rc = wbIconRender(cl, obj, (APTR)msg); break;
    case GM_GOACTIVE: rc = wbIconGoActive(cl, obj, (APTR)msg); break;
    case GM_GOINACTIVE: rc = wbIconGoInactive(cl, obj, (APTR)msg); break;
    case GM_HANDLEINPUT: rc = wbIconHandleInput(cl, obj, (APTR)msg); break;
    default:         rc = DoSuperMethodA(cl, obj, msg); break;
    }

    return rc;
}

Class *WBIcon_MakeClass(struct WorkbookBase *wb)
{
    Class *cl;

    cl = MakeClass( NULL, "gadgetclass", NULL,
                    sizeof(struct wbIcon),
                    0);
    if (cl != NULL) {
    	cl->cl_Dispatcher.h_Entry = HookEntry;
    	cl->cl_Dispatcher.h_SubEntry = dispatcher;
    	cl->cl_Dispatcher.h_Data = NULL;
    	cl->cl_UserData = (IPTR)wb;
    }

    return cl;
}
