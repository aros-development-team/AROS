/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

/****************************************************************************************/

#include <exec/types.h>

#include <dos/dos.h>
#include <dos/dosextens.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>

#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <clib/macros.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#ifndef __MORPHOS__
#include <aros/asmcall.h>
#include <proto/alib.h>
#include "intuition_intern.h"
#endif /* !__MORPHOS__ */

#ifdef IntuitionBase
#    undef IntuitionBase
#endif

/****************************************************************************************/

/* On the Amiga tabcycling between member (string) gadgets of a group
   gadget does not work at all -> freeze */

#define SUPPORT_TABCYCLE    1

#define IntuitionBase       ((struct IntuitionBase *)(cl->cl_UserData))
#define G(x)            ((struct ExtGadget *)(x))

/****************************************************************************************/

struct GroupGData
{
    struct MinList   memberlist;
    Object          *activeobj;
};

/****************************************************************************************/

static void recalcgroupsize(Class *cl, Object *obj)
{
    struct GroupGData   *data = INST_DATA(cl, obj);
    Object          	*member, *memberstate;
    WORD            	 w, h, width = 0/*G(obj)->Width*/, height = 0/*G(obj)->Height*/;

    memberstate = (Object *)data->memberlist.mlh_Head;
    while((member = NextObject(&memberstate)))
    {
        /* No "width - 1" / "height - 1" here! Coords of members are absolute here! */

        w = G(member)->LeftEdge - G(obj)->LeftEdge + G(member)->Width;
        h = G(member)->TopEdge  - G(obj)->TopEdge  + G(member)->Height;

        if (w > width)  width  = w;
        if (h > height) height = h;
    }

    G(obj)->Width  = width;
    G(obj)->Height = height;
}

/****************************************************************************************/

#if SUPPORT_TABCYCLE

/****************************************************************************************/

Object *next_tabcycleobject(Class *cl, Object *obj)
{
    struct GroupGData   *data = INST_DATA(cl, obj);
    Object          	*member, *memberstate, *actobj;
    Object          	*rc = NULL;

    actobj = data->activeobj;

    memberstate = (Object *)data->memberlist.mlh_Head;
    while ((member = NextObject(&memberstate)))
    {
        if (member == actobj) break;
    }

    if (member)
    {
        while ((member = NextObject(&memberstate)))
        {
            if ( (G(member)->Flags & (GFLG_TABCYCLE | GFLG_DISABLED)) == GFLG_TABCYCLE )
            {
                rc = member;
                break;
            }
        }

        if (!member)
        {
            memberstate = (Object *)data->memberlist.mlh_Head;
            while ((member = NextObject(&memberstate)))
            {
                if (member == actobj) break;

                if (G(member)->Flags & GFLG_TABCYCLE)
                {
                    rc = member;
                    break;
                }
            }

        } /* if (!rc) */

    } /* if (member) */

    return rc;
}

/****************************************************************************************/

Object *prev_tabcycleobject(Class *cl, Object *obj)
{
    struct GroupGData   *data = INST_DATA(cl, obj);
    Object          	*member, *memberstate, *actobj;
    Object          	*prevmember = NULL, *rc = NULL;

    actobj = data->activeobj;

    memberstate = (Object *)data->memberlist.mlh_Head;
    while ((member = NextObject(&memberstate)))
    {
        if (member == actobj)
        {
            if (prevmember) rc = prevmember;
            break;
        }

        if ( (G(member)->Flags & (GFLG_TABCYCLE | GFLG_DISABLED)) == GFLG_TABCYCLE )
        {
            prevmember = member;
        }
    }

    if (member && !rc)
    {
        prevmember = NULL;
        while ((member = NextObject(&memberstate)))
        {
            if (((struct Gadget *)member)->Flags & GFLG_TABCYCLE)
            {
                prevmember = member;
            }
        }

        rc = prevmember;

    } /* if (member && !rc) */

    return rc;

}

/****************************************************************************************/

#endif /* SUPPORT_TABCYCLE */

/****************************************************************************************/

static IPTR groupg_new(Class *cl, Object *obj, struct opSet *msg)
{
    struct GroupGData *data;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (obj)
    {
        data = INST_DATA(cl, obj);

        NEWLIST(&data->memberlist);

        /* Width and height of group gadget is determined by members. No members -> 0 size */

        G(obj)->Width  = 0;
        G(obj)->Height = 0;
    }

    return (IPTR)obj;
}

/****************************************************************************************/

static IPTR groupg_set(Class *cl, Object *obj, struct opSet *msg)
{
    struct GroupGData   *data = INST_DATA(cl, obj);
    Object          	*member, *memberstate;
    WORD            	 dx, new_groupleft, old_groupleft = G(obj)->LeftEdge;
    WORD            	 dy, new_grouptop , old_grouptop  = G(obj)->TopEdge;
    IPTR            	 rc;

    rc = DoSuperMethodA(cl, obj, (Msg)msg);

    new_groupleft = G(obj)->LeftEdge;
    new_grouptop  = G(obj)->TopEdge;

    dx = new_groupleft - old_groupleft;
    dy = new_grouptop  - old_grouptop;

    if (dx || dy)
    {
        struct opSet   m;
        struct TagItem tags[3];

        m.MethodID     = OM_SET;
        m.ops_AttrList = tags;
        m.ops_GInfo    = msg->ops_GInfo;

        tags[0].ti_Tag = GA_Left;
        tags[1].ti_Tag = GA_Top;
        tags[2].ti_Tag = TAG_DONE;

        memberstate = (Object *)data->memberlist.mlh_Head;
        while((member = NextObject(&memberstate)))
        {
            tags[0].ti_Data = G(member)->LeftEdge + dx;
            tags[1].ti_Data = G(member)->TopEdge  + dy;
            DoMethodA(member, (Msg)&m);
        }

    } /* if (dx || dy) */

    return rc;

}

/****************************************************************************************/

static IPTR groupg_dispose(Class *cl, Object *obj, Msg msg)
{
    struct GroupGData *data = INST_DATA(cl, obj);

    /* Free all members */

    for(;;)
    {
        Object *member, *memberstate;
        ULONG method;

        memberstate = (Object *)data->memberlist.mlh_Head;
        member = NextObject(&memberstate);
        if (!member) break;

        method = OM_REMOVE;
        DoMethodA(member, (Msg)&method);
        DisposeObject(member);
    }

    DoSuperMethodA(cl, obj, msg);

    return 0;
}

/****************************************************************************************/

static IPTR groupg_addmember(Class *cl, Object *obj, struct opMember *msg)
{
    struct GroupGData   *data = INST_DATA(cl, obj);
    struct opAddTail     m;
    struct opSet         m2;
    struct TagItem  	 tags[3];
    IPTR            	 rc;

    m.MethodID  = OM_ADDTAIL;
    m.opat_List = (struct List *)&data->memberlist;

    rc = DoMethodA(msg->opam_Object, (Msg)&m);

    /* Member gadget had its coords relative to group gadget.
       Convert the coords to absolute coords. */

    m2.MethodID = OM_SET;
    m2.ops_AttrList = tags;
    m2.ops_GInfo = NULL;

    tags[0].ti_Tag  = GA_Left;
    tags[0].ti_Data = G(msg->opam_Object)->LeftEdge + G(obj)->LeftEdge;
    tags[1].ti_Tag  = GA_Top;
    tags[1].ti_Data = G(msg->opam_Object)->TopEdge + G(obj)->TopEdge;
    tags[2].ti_Tag  = TAG_DONE;

    DoMethodA(msg->opam_Object, (Msg)&m2);

    recalcgroupsize(cl, obj);

    return rc;
}

/****************************************************************************************/

static IPTR groupg_remmember(Class *cl, Object *obj, struct opMember *msg)
{
    struct opSet    m2;
    struct TagItem  tags[3];
    IPTR    	    rc;
    STACKULONG      method = OM_REMOVE;

    rc = DoMethodA(msg->opam_Object, (Msg)&method);

    /* Member gadget had its coords absolute here.
       Convert the coords back to relative coords. */

    m2.MethodID     = OM_SET;
    m2.ops_AttrList = tags;
    m2.ops_GInfo    = NULL;

    tags[0].ti_Tag  = GA_Left;
    tags[0].ti_Data = G(msg->opam_Object)->LeftEdge - G(obj)->LeftEdge;
    tags[1].ti_Tag  = GA_Top;
    tags[1].ti_Data = G(msg->opam_Object)->TopEdge - G(obj)->TopEdge;
    tags[2].ti_Tag  = TAG_DONE;

    DoMethodA(msg->opam_Object, (Msg)&m2);

    recalcgroupsize(cl, obj);

    return rc;
}

/****************************************************************************************/

static IPTR groupg_hittest(Class *cl, Object *obj, struct gpHitTest *msg)
{
    struct GroupGData   *data = INST_DATA(cl, obj);
    struct gpHitTest     m;
    Object          	*member, *memberstate;
    WORD            	 x, y;
    IPTR            	 rc = 0;

    m = *msg;

    /* gpht_Mouse.X/Y are relative to (group) gadget */

    x = msg->gpht_Mouse.X + G(obj)->LeftEdge;
    y = msg->gpht_Mouse.Y + G(obj)->TopEdge;

    memberstate = (Object *)data->memberlist.mlh_Head;
    while((member = NextObject(&memberstate)))
    {
        if (!(G(member)->Flags & GFLG_DISABLED))
        {
            /* make mouse coords relative to member gadget */

            m.gpht_Mouse.X = x - G(member)->LeftEdge;
            m.gpht_Mouse.Y = y - G(member)->TopEdge;

            if ((m.gpht_Mouse.X >= 0) &&
                (m.gpht_Mouse.Y >= 0) &&
                (m.gpht_Mouse.X < G(member)->Width) &&
                (m.gpht_Mouse.Y < G(member)->Height))
            {
                rc = DoMethodA(member, (Msg)&m);
                if (rc == GMR_GADGETHIT)
                {
                    data->activeobj = member;
                    break;
                }

            }

        } /* if (!(G(member)->Flags & GFLG_DISABLED)) */

    } /* while((member = NextObject(&memberstate))) */

    return rc;
}

/****************************************************************************************/

static IPTR groupg_input(Class *cl, Object *obj, struct gpInput *msg)
{
    struct GroupGData   *data = INST_DATA(cl, obj);
    struct gpInput  	 m;
    IPTR            	 rc;

    /* If someone activates us with ActivateGadget(), activeobj will be NULL.
     * In that case, activate the first object.
     */
    if (!data->activeobj)
    {
        Object *memberstate = (Object *)data->memberlist.mlh_Head;
	
        data->activeobj = NextObject(&memberstate);
    }

    m = *msg;

    /* gpi_Mouse coords are relative to group gadget. Make them relative
       to activate object */

    m.gpi_Mouse.X = G(obj)->LeftEdge + msg->gpi_Mouse.X - G(data->activeobj)->LeftEdge;
    m.gpi_Mouse.Y = G(obj)->TopEdge  + msg->gpi_Mouse.Y - G(data->activeobj)->TopEdge;

    rc = DoMethodA(data->activeobj, (Msg)&m);

#if SUPPORT_TABCYCLE
    {
        Object *newobj = NULL;

        if (rc & GMR_NEXTACTIVE) newobj = next_tabcycleobject(cl, obj);
        if (rc & GMR_PREVACTIVE) newobj = prev_tabcycleobject(cl, obj);

        if (newobj && (newobj != data->activeobj))
        {
            struct gpGoInactive im;

            /* Make old member gadget inactive */

            im.MethodID   = GM_GOINACTIVE;
            im.gpgi_GInfo = msg->gpi_GInfo;
            im.gpgi_Abort = 0;          /* The gadget itself wanted to be deactivated */

            DoMethodA(data->activeobj, (Msg)&im);

            /* Make new member gadget active */

            data->activeobj = newobj;

            m.MethodID    = GM_GOACTIVE;
            m.gpi_Mouse.X = G(obj)->LeftEdge + msg->gpi_Mouse.X - G(newobj)->LeftEdge;
            m.gpi_Mouse.Y = G(obj)->TopEdge  + msg->gpi_Mouse.Y - G(newobj)->TopEdge;

            rc = DoMethodA(newobj, (Msg)&m);

        }

    }
#endif

    return rc;
}

/****************************************************************************************/

static IPTR groupg_goinactive(Class *cl, Object *obj, struct gpGoInactive *msg)
{
    struct GroupGData   *data = INST_DATA(cl, obj);
    IPTR            	 rc;

    ASSERT_VALID_PTR(data->activeobj);

    rc = DoMethodA(data->activeobj, (Msg)msg);
    data->activeobj = NULL;

    return rc;
}

/****************************************************************************************/

static IPTR groupg_render(Class *cl, Object *obj, struct gpRender *msg)
{
    struct GroupGData   *data = INST_DATA(cl, obj);
    Object      *member, *memberstate;

    memberstate = (Object *)data->memberlist.mlh_Head;
    while((member = NextObject(&memberstate)))
    {
        DoMethodA(member, (Msg)msg);
    }

    return 0;
}

/****************************************************************************************/

AROS_UFH3S(IPTR, dispatch_groupgclass,
           AROS_UFHA(Class *, cl, A0),
           AROS_UFHA(Object *, obj, A2),
           AROS_UFHA(Msg, msg, A1)
          )
{
    AROS_USERFUNC_INIT

    IPTR retval;

    switch (msg->MethodID)
    {
	case OM_NEW:
            retval = groupg_new(cl, obj, (struct opSet *)msg);
            break;

	case OM_SET:
	case OM_UPDATE:
            retval = groupg_set(cl, obj, (struct opSet *)msg);
            break;

	case OM_DISPOSE:
            retval = groupg_dispose(cl, obj, msg);
            break;

	case OM_ADDMEMBER:
            retval = groupg_addmember(cl, obj, (struct opMember *)msg);
            break;

	case OM_REMMEMBER:
            retval = groupg_remmember(cl, obj, (struct opMember *)msg);
            break;

	case GM_HITTEST:
            retval = groupg_hittest(cl, obj, (struct gpHitTest *)msg);
            break;

	case GM_GOACTIVE:
	case GM_HANDLEINPUT:
            retval = groupg_input(cl, obj, (struct gpInput *)msg);
            break;

	case GM_GOINACTIVE:
            retval = groupg_goinactive(cl, obj, (struct gpGoInactive *)msg);
            break;

	case GM_RENDER:
            retval = groupg_render(cl, obj, (struct gpRender *)msg);
            break;

	default:
            retval = DoSuperMethodA(cl, obj, msg);
            break;

    } /* switch (msg->MethodID) */

    return retval;

    AROS_USERFUNC_EXIT
}

/****************************************************************************************/

#undef IntuitionBase

/****************************************************************************************/

struct IClass *InitGroupGClass (struct IntuitionBase * IntuitionBase)
{
    struct IClass *cl;

    if ( (cl = MakeClass(GROUPGCLASS, GADGETCLASS, NULL, sizeof(struct GroupGData), 0)) )
    {
        cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_groupgclass);
        cl->cl_Dispatcher.h_SubEntry = NULL;
        cl->cl_UserData              = (IPTR)IntuitionBase;

        AddClass (cl);
    }

    return (cl);
}

/****************************************************************************************/
