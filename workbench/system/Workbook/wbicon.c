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
    struct Screen     *Screen;

    struct timeval LastActive;
};

const struct TagItem wbIcon_DrawTags[] = {
    { ICONDRAWA_Frameless, TRUE, },
    { ICONDRAWA_Borderless, TRUE, },
    { ICONDRAWA_EraseBackground, FALSE, },
    { TAG_DONE },
};

void wbIcon_Update(Class *cl, Object *obj)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbIcon *my = INST_DATA(cl, obj);
    struct Rectangle rect;
    UWORD w,h;

    /* Update the parent's idea of how big we are
     */
    GetIconRectangleA(&my->Screen->RastPort, my->Icon, (STRPTR)my->Label, &rect, (struct TagItem *)wbIcon_DrawTags);

    w = (rect.MaxX - rect.MinX) + 1;
    h = (rect.MaxY - rect.MinY) + 1;

    /* If the icon is outside of the bounds for this
     * screen, ignore the position information
     */
    if ((my->Icon->do_CurrentX != NO_ICON_POSITION ||
         my->Icon->do_CurrentY != NO_ICON_POSITION) && my->Screen) {
        if ((my->Icon->do_CurrentX != NO_ICON_POSITION &&
            (my->Icon->do_CurrentX < my->Screen->LeftEdge ||
            (my->Icon->do_CurrentX > (my->Screen->LeftEdge + my->Screen->Width - w)))) ||
            (my->Icon->do_CurrentY != NO_ICON_POSITION &&
            (my->Icon->do_CurrentY < my->Screen->TopEdge ||
            (my->Icon->do_CurrentY > (my->Screen->TopEdge + my->Screen->Height - h))))) {
            my->Icon->do_CurrentY = NO_ICON_POSITION;
            my->Icon->do_CurrentX = NO_ICON_POSITION;
        }
    }

    D(bug("%s: %dx%d @%d,%d (%s)\n", my->File, (int)w, (int)h, (WORD)my->Icon->do_CurrentX, (WORD)my->Icon->do_CurrentY, my->Label));
    SetAttrs(obj,
    	GA_Left, (my->Icon->do_CurrentX == NO_ICON_POSITION) ? ~0 : my->Icon->do_CurrentX,
    	GA_Top, (my->Icon->do_CurrentY == NO_ICON_POSITION) ? ~0 : my->Icon->do_CurrentY,
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
    my->Screen = (struct Screen *)GetTagData(WBIA_Screen, (IPTR)NULL, ops->ops_AttrList);
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

	label = FilePart(my->File);

	my->Icon = GetIconTags(my->File,
	                       ICONGETA_Screen, my->Screen,
	                       ICONGETA_FailIfUnavailable, FALSE,
	                       TAG_END);
	if (my->Icon == NULL)
	    goto error;

    }

    my->Label = StrDup((CONST_STRPTR)GetTagData(WBIA_Label, (IPTR)label, ops->ops_AttrList));
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
    IPTR rc = TRUE;

    switch (opg->opg_AttrID) {
    case WBIA_File:
    	*(opg->opg_Storage) = (IPTR)my->File;
    	break;
    case WBIA_Label:
    	*(opg->opg_Storage) = (IPTR)my->Label;
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

    x = gadget->LeftEdge;
    y = gadget->TopEdge;

    DrawIconStateA(rp, my->Icon, (STRPTR)my->Label, x, y,
    	(gadget->Flags & GFLG_SELECTED) ? TRUE : FALSE, (struct TagItem *)wbIcon_DrawTags);

    return 0;
}

// GM_GOACTIVE
static IPTR wbIconGoActive(Class *cl, Object *obj, struct gpInput *gpi)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbIcon *my = INST_DATA(cl, obj);
    struct Gadget *gadget = (struct Gadget *)obj;
    BOOL dclicked;

    gadget->Flags ^= GFLG_SELECTED;
    DoMethod(obj, GM_RENDER, gpi->gpi_GInfo, gpi->gpi_GInfo->gi_RastPort,GREDRAW_TOGGLE);
    
    /* On a double-click, don't go 'active', just
     * do the action.
     */
    dclicked = DoubleClick(my->LastActive.tv_secs,
    		           my->LastActive.tv_micro,
    		           gpi->gpi_IEvent->ie_TimeStamp.tv_secs,
    		           gpi->gpi_IEvent->ie_TimeStamp.tv_micro);

    my->LastActive = gpi->gpi_IEvent->ie_TimeStamp;

    if (dclicked)
    	DoMethod(obj, WBIM_Open);

    return GMR_MEACTIVE;
}

// GM_GOINACTIVE
static IPTR wbIconGoInactive(Class *cl, Object *obj, struct gpGoInactive *gpgi)
{
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

// WBIM_Open
static IPTR wbIconOpen(Class *cl, Object *obj, Msg msg)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbIcon *my = INST_DATA(cl, obj);

    struct TagItem tags[] = {
	{ NP_Seglist,     (IPTR)wb->wb_OpenerSegList },
	{ NP_Arguments,   (IPTR)my->File },
	{ NP_FreeSeglist, FALSE },
	{ TAG_END, 0 },
    };
    CreateNewProc(tags);

    return TRUE;
}

// WBIM_Copy
static IPTR wbIconCopy(Class *cl, Object *obj, Msg msg)
{
    return FALSE;
}

// WBIM_Rename
static IPTR wbIconRename(Class *cl, Object *obj, Msg msg)
{
    return FALSE;
}

// WBIM_Info
static IPTR wbIconInfo(Class *cl, Object *obj, Msg msg)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbIcon *my = INST_DATA(cl, obj);

    return WBInfo(BNULL, my->File, NULL);
}

// WBIM_Snapshot
static IPTR wbIconSnapshot(Class *cl, Object *obj, Msg msg)
{
    return FALSE;
}

// WBIM_Unsnapshot
static IPTR wbIconUnsnapshot(Class *cl, Object *obj, Msg msg)
{
    return FALSE;
}

// WBIM_Leave_Out
static IPTR wbIconLeaveOut(Class *cl, Object *obj, Msg msg)
{
    return FALSE;
}

// WBIM_Put_Away
static IPTR wbIconPutAway(Class *cl, Object *obj, Msg msg)
{
    return FALSE;
}

// WBIM_Delete
static IPTR wbIconDelete(Class *cl, Object *obj, Msg msg)
{
    return FALSE;
}

// WBIM_Format
static IPTR wbIconFormat(Class *cl, Object *obj, Msg msg)
{
    return FALSE;
}

// WBIM_Empty_Trash
static IPTR wbIconEmptyTrash(Class *cl, Object *obj, Msg msg)
{
    return FALSE;
}

static IPTR dispatcher(Class *cl, Object *obj, Msg msg)
{
    IPTR rc = 0;

    switch (msg->MethodID) {
    case OM_NEW:           rc = wbIconNew(cl, obj, (APTR)msg); break;
    case OM_DISPOSE:       rc = wbIconDispose(cl, obj, (APTR)msg); break;
    case OM_GET:           rc = wbIconGet(cl, obj, (APTR)msg); break;
    case GM_RENDER:        rc = wbIconRender(cl, obj, (APTR)msg); break;
    case GM_GOACTIVE:      rc = wbIconGoActive(cl, obj, (APTR)msg); break;
    case GM_GOINACTIVE:    rc = wbIconGoInactive(cl, obj, (APTR)msg); break;
    case GM_HANDLEINPUT:   rc = wbIconHandleInput(cl, obj, (APTR)msg); break;
    case WBIM_Open:        rc = wbIconOpen(cl, obj, msg); break;
    case WBIM_Copy:        rc = wbIconCopy(cl, obj, msg); break;
    case WBIM_Rename:      rc = wbIconRename(cl, obj, msg); break;
    case WBIM_Info:        rc = wbIconInfo(cl, obj, msg); break;
    case WBIM_Snapshot:    rc = wbIconSnapshot(cl, obj, msg); break;
    case WBIM_Unsnapshot:  rc = wbIconUnsnapshot(cl, obj, msg); break;
    case WBIM_Leave_Out:   rc = wbIconLeaveOut(cl, obj, msg); break;
    case WBIM_Put_Away:    rc = wbIconPutAway(cl, obj, msg); break;
    case WBIM_Delete:      rc = wbIconDelete(cl, obj, msg); break;
    case WBIM_Format:      rc = wbIconFormat(cl, obj, msg); break;
    case WBIM_Empty_Trash: rc = wbIconEmptyTrash(cl, obj, msg); break;
    default:               rc = DoSuperMethodA(cl, obj, msg); break;
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
