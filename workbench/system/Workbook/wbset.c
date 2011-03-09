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

struct wbSet {
    LONG           NumItems;
};

static void wbGABox(struct WorkbookBase *wb, Object *obj, struct IBox *box)
{
    struct Gadget *gadget = (struct Gadget *)obj;

    box->Top = gadget->TopEdge;
    box->Left = gadget->LeftEdge;
    box->Width = gadget->Width;
    box->Height = gadget->Height;
}

static IPTR WBSetAddMember(Class *cl, Object *obj, struct opMember *opm)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbSet *my = INST_DATA(cl, obj);
    struct IBox box, ibox;
    Object *iobj = opm->opam_Object;
    IPTR rc;
    struct TagItem tags[3] = {
    	{ GA_Width, },
    	{ GA_Height, },
    	{ TAG_END, },
    };

    wbGABox(wb, obj, &box);
    wbGABox(wb, iobj, &ibox);

    if (1 || ibox.Left == NO_ICON_POSITION ||
    	ibox.Top == NO_ICON_POSITION) {

    	ibox.Top  = (my->NumItems / 8) * 80;
    	ibox.Left = (my->NumItems % 8) * 80;

    	SetAttrs(iobj, GA_Left, ibox.Left, GA_Top, ibox.Top, TAG_END);
    }

    D(bug("My  box: %dx%d @%d,%d\n", box.Width, box.Height, box.Left, box.Top));
    D(bug("My ibox: %dx%d @%d,%d\n", ibox.Width, ibox.Height, ibox.Left, ibox.Top));

    my->NumItems++;
    rc = DoSuperMethodA(cl, obj, (Msg)opm);

    /* Notify when our size changes */
    GetAttr(GA_Width, obj, &tags[0].ti_Data);
    GetAttr(GA_Height, obj, &tags[1].ti_Data);
    DoMethod(obj, OM_NOTIFY, tags, NULL, 0);

    return rc;
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
    case OM_ADDMEMBER:  rc = WBSetAddMember(cl, obj, (APTR)msg); break;
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
