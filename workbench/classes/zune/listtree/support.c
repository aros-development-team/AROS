/*
    Copyright (C) 2015, The AROS Development Team. All rights reserved.
*/

#include <proto/muimaster.h>
#include <proto/utility.h>
#include <clib/alib_protos.h>
#include <mui/NListtree_mcc.h>
#include <mui/NList_mcc.h>

#include "support.h"

#include <aros/debug.h>

/* Internal version of NListtree that enables controling the dispatcher */
#include <aros/symbolsets.h>
#define ADD2INITCLASSES(symbol, pri) ADD2SET(symbol, CLASSESINIT, pri)
#define ADD2EXPUNGECLASSES(symbol, pri) ADD2SET(symbol, CLASSESEXPUNGE, pri)

/* Routing of inherited methods calls:
 * Some methods are often overriden in child classes of Listree, for example DragReport.
 * On the other hand, those methods get called as interaction on the NListtree object.
 * To allow using overriden methods, the following call sequence is implemented:
 *      NListtreeInt.A          -> Listreee.A
 *      Listtree.A              -> NListtreeInt.SuperA
 * In case user inherited code, the call sequence looks as follows
 *      NListtreeInt.A          -> Listreee-inherited.A
 *      Listreee-inherited.A    -> Listreee.A
 *      Listtree.A              -> NListtreeInt.SuperA
 */

#define MUIM_NListtreeInt_ForwardSuperMethod    0xfec81301UL

struct MUIP_NListtreeInt_ForwardSuperMethod
{
  STACKED ULONG MethodID;
  STACKED Msg msg;
};

struct NListtreeInt_DATA
{
    Object * listtree;
};

struct MUI_CustomClass * CL_NListtreeInt;

/* NListtreeInt__OM_SET
 * Store the borrowed outer Listtree pointer from the private tag and
 * delegate the complete set message. Require the owner to remain live while
 * forwarding is possible; no reference is acquired.
 */
IPTR NListtreeInt__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct NListtreeInt_DATA *data = INST_DATA(cl, obj);
    struct TagItem      *tstate = msg->ops_AttrList;
    struct TagItem      *tag;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case(MUIA_NListtreeInt_Listtree):
            data->listtree = (Object *)tag->ti_Data;
            break;
        }
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/* NListtreeInt__ForwardListree
 * Synchronously send the unchanged borrowed message to the stored live
 * outer Listtree object and return its result. No NULL-owner check or
 * message copy.
 */
IPTR NListtreeInt__ForwardListree(struct IClass *cl, Object *obj, Msg msg)
{
    struct NListtreeInt_DATA *data = INST_DATA(cl, obj);

    return DoMethodA(data->listtree, msg);
}

/* NListtreeInt__ForwardSuperMethod
 * Send the nested borrowed message directly to this internal class
 * superclass. Require a valid message; return superclass result. This
 * private path avoids another outer-object forwarding step.
 */
IPTR NListtreeInt__ForwardSuperMethod(struct IClass *cl, Object *obj, struct MUIP_NListtreeInt_ForwardSuperMethod * msg)
{
    return DoSuperMethodA(cl, obj, msg->msg);
}

/* NListtreeInt__DoDrag
 * Ask the outer owner window to drag the outer Listtree using the supplied
 * touch coordinates and flags. Require an attached live owner/window.
 * Ignore window result and return zero.
 */
IPTR NListtreeInt__DoDrag(struct IClass *cl, Object *obj, struct MUIP_DoDrag * msg)
{
    struct NListtreeInt_DATA *data = INST_DATA(cl, obj);

    /* Use the listtree as the dragged object */
    DoMethod(_win(data->listtree), MUIB_Window | 0x00000003,
            (IPTR)data->listtree, msg->touchx, msg->touchy, msg->flags);
    return 0;
}

/* NListtreeInt__DragQuery
 * For a query naming the outer owner, copy the message and replace that
 * object with the internal child before superclass dispatch. Otherwise
 * delegate unchanged. Return superclass result; no ownership transfer.
 */
IPTR NListtreeInt__DragQuery(struct IClass *cl, Object *obj, struct MUIP_DragQuery *msg)
{
    struct NListtreeInt_DATA *data = INST_DATA(cl, obj);

    if (data->listtree == msg->obj)
    {
        /* Forward to parent class passing "this" */
        struct MUIP_DragQuery temp = *msg;
        temp.obj = obj;
        return DoSuperMethodA(cl, obj, &temp);
    }

    return DoSuperMethodA(cl, obj, msg);
}

/* NListtreeInt_Dispatcher
 * Dispatch private owner assignment, explicit superclass forwarding, drag
 * operations and context-menu forwarding. Delegate other methods to
 * NListtree. Require valid message layouts and connected owner for
 * forwarding.
 */
BOOPSI_DISPATCHER(IPTR, NListtreeInt_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case(OM_SET):  return NListtreeInt__OM_SET(cl, obj, (struct opSet *)msg);
    case(MUIM_NListtreeInt_ForwardSuperMethod):
        return NListtreeInt__ForwardSuperMethod(cl, obj, (struct MUIP_NListtreeInt_ForwardSuperMethod *)msg);
    case(MUIM_DoDrag):
        return NListtreeInt__DoDrag(cl, obj, (struct MUIP_DoDrag *)msg);
    case(MUIM_DragQuery):
        return NListtreeInt__DragQuery(cl, obj, (struct MUIP_DragQuery *)msg);
    case(MUIM_ContextMenuBuild):
    case(MUIM_ContextMenuChoice):
        return NListtreeInt__ForwardListree(cl, obj, msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/* MCC_NListtreeInt_Startup
 * Create the internal NListtree subclass and store its class pointer
 * globally. Return whether creation succeeded. Require class initialization
 * ordering before any outer instance is constructed.
 */
static int MCC_NListtreeInt_Startup(struct Library * lib)
{
    CL_NListtreeInt = MUI_CreateCustomClass(lib, MUIC_NListtree, NULL, sizeof(struct NListtreeInt_DATA), NListtreeInt_Dispatcher);
    return CL_NListtreeInt != NULL;
}

/* MCC_NListtreeInt_Shutdown
 * Delete the stored internal class. Require all users to have finished;
 * ignore deletion result and do not clear the global pointer. This is a
 * class-expunge callback, not instance disposal.
 */
static void MCC_NListtreeInt_Shutdown(struct Library * lib)
{
    MUI_DeleteCustomClass(CL_NListtreeInt);
}

ADD2INITCLASSES(MCC_NListtreeInt_Startup, -1);
ADD2EXPUNGECLASSES(MCC_NListtreeInt_Shutdown, -1);
