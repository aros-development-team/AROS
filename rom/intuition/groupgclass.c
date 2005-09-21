/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
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

/****************************************************************************************/

static void recalcgroupsize(Class *cl, struct Gadget *g)
{
    struct GroupGData   *data = INST_DATA(cl, g);
    struct Gadget      	*member, *memberstate;
    WORD            	 w, h, width = 0/*g->Width*/, height = 0/*g->Height*/;

    memberstate = (struct Gadget *)data->memberlist.mlh_Head;
    while((member = NextObject(&memberstate)))
    {
        /* No "width - 1" / "height - 1" here! Coords of members are absolute here! */

        w = member->LeftEdge - g->LeftEdge + member->Width;
        h = member->TopEdge  - g->TopEdge  + member->Height;

        if (w > width)  width  = w;
        if (h > height) height = h;
    }

    g->Width  = width;
    g->Height = height;
}

/****************************************************************************************/

#if SUPPORT_TABCYCLE

/****************************************************************************************/

struct Gadget *next_tabcycleobject(Class *cl, struct Gadget *g)
{
    struct GroupGData   *data = INST_DATA(cl, g);
    struct Gadget      	*member, *memberstate, *actobj;
    struct Gadget      	*rc = NULL;

    actobj = data->activegad;

    memberstate = (struct Gadget *)data->memberlist.mlh_Head;
    while ((member = NextObject(&memberstate)))
    {
        if (member == actobj) break;
    }

    if (member)
    {
        while ((member = NextObject(&memberstate)))
        {
            if ( (member->Flags & (GFLG_TABCYCLE | GFLG_DISABLED)) == GFLG_TABCYCLE )
            {
                rc = member;
                break;
            }
        }

        if (!member)
        {
            memberstate = (struct Gadget *)data->memberlist.mlh_Head;
            while ((member = NextObject(&memberstate)))
            {
                if (member == actobj) break;

                if (member->Flags & GFLG_TABCYCLE)
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

struct Gadget *prev_tabcycleobject(Class *cl, struct Gadget *g)
{
    struct GroupGData   *data = INST_DATA(cl, g);
    struct Gadget      	*member, *memberstate, *actobj;
    struct Gadget      	*prevmember = NULL, *rc = NULL;

    actobj = data->activegad;

    memberstate = (struct Gadget *)data->memberlist.mlh_Head;
    while ((member = NextObject(&memberstate)))
    {
        if (member == actobj)
        {
            if (prevmember) rc = prevmember;
            break;
        }

        if ( (member->Flags & (GFLG_TABCYCLE | GFLG_DISABLED)) == GFLG_TABCYCLE )
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

IPTR GroupGClass__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    struct GroupGData *data;

    struct Gadget *g = (struct Gadget *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (g)
    {
        data = INST_DATA(cl, g);

        NEWLIST(&data->memberlist);

        /* Width and height of group gadget is determined by members. No members -> 0 size */

        g->Width  = 0;
        g->Height = 0;
    }

    return (IPTR)g;
}

/****************************************************************************************/

IPTR GroupGClass__OM_SET(Class *cl, struct Gadget *g, struct opSet *msg)
{
    struct GroupGData   *data = INST_DATA(cl, g);
    struct Gadget      	*member, *memberstate;
    WORD            	 dx, new_groupleft, old_groupleft = g->LeftEdge;
    WORD            	 dy, new_grouptop , old_grouptop  = g->TopEdge;
    IPTR            	 rc;

    rc = DoSuperMethodA(cl, (Object *)g, (Msg)msg);

    new_groupleft = g->LeftEdge;
    new_grouptop  = g->TopEdge;

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

        memberstate = (struct Gadget *)data->memberlist.mlh_Head;
        while((member = NextObject(&memberstate)))
        {
            tags[0].ti_Data = member->LeftEdge + dx;
            tags[1].ti_Data = member->TopEdge  + dy;
            DoMethodA((Object *)member, (Msg)&m);
        }

    } /* if (dx || dy) */

    return rc;

}

/****************************************************************************************/

IPTR GroupGClass__OM_DISPOSE(Class *cl, struct Gadget *g, Msg msg)
{
    struct GroupGData *data = INST_DATA(cl, g);

    /* Free all members */

    for(;;)
    {
        struct Gadget *member, *memberstate;
        ULONG method;

        memberstate = (struct Gadget *)data->memberlist.mlh_Head;
        member = NextObject(&memberstate);
        if (!member) break;

        method = OM_REMOVE;
        DoMethodA((Object *)member, (Msg)&method);
        DisposeObject(member);
    }

    DoSuperMethodA(cl, (Object *)g, msg);

    return 0;
}

/****************************************************************************************/

IPTR GroupGClass__OM_ADDMEMBER(Class *cl, struct Gadget *g, struct opMember *msg)
{
    struct GroupGData   *data = INST_DATA(cl, g);
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
    tags[0].ti_Data = ((struct Gadget *)msg->opam_Object)->LeftEdge + g->LeftEdge;
    tags[1].ti_Tag  = GA_Top;
    tags[1].ti_Data = ((struct Gadget *)msg->opam_Object)->TopEdge + g->TopEdge;
    tags[2].ti_Tag  = TAG_DONE;

    DoMethodA(msg->opam_Object, (Msg)&m2);

    recalcgroupsize(cl, g);

    return rc;
}

/****************************************************************************************/

IPTR GroupGClass__OM_REMMEMBER(Class *cl, struct Gadget *g, struct opMember *msg)
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
    tags[0].ti_Data = ((struct Gadget *)msg->opam_Object)->LeftEdge - g->LeftEdge;
    tags[1].ti_Tag  = GA_Top;
    tags[1].ti_Data = ((struct Gadget *)msg->opam_Object)->TopEdge - g->TopEdge;
    tags[2].ti_Tag  = TAG_DONE;

    DoMethodA(msg->opam_Object, (Msg)&m2);

    recalcgroupsize(cl, g);

    return rc;
}

/****************************************************************************************/

IPTR GroupGClass__GM_HITTEST(Class *cl, struct Gadget *g, struct gpHitTest *msg)
{
    struct GroupGData   *data = INST_DATA(cl, g);
    struct gpHitTest     m;
    struct Gadget      	*member, *memberstate;
    WORD            	 x, y;
    IPTR            	 rc = 0;

    m = *msg;

    /* gpht_Mouse.X/Y are relative to (group) gadget */

    x = msg->gpht_Mouse.X + g->LeftEdge;
    y = msg->gpht_Mouse.Y + g->TopEdge;

    memberstate = (struct Gadget *)data->memberlist.mlh_Head;
    while((member = NextObject(&memberstate)))
    {
        if (!(member->Flags & GFLG_DISABLED))
        {
            /* make mouse coords relative to member gadget */

            m.gpht_Mouse.X = x - member->LeftEdge;
            m.gpht_Mouse.Y = y - member->TopEdge;

            if ((m.gpht_Mouse.X >= 0) &&
                (m.gpht_Mouse.Y >= 0) &&
                (m.gpht_Mouse.X < member->Width) &&
                (m.gpht_Mouse.Y < member->Height))
            {
                rc = DoMethodA((Object *)member, (Msg)&m);
                if (rc == GMR_GADGETHIT)
                {
                    data->activegad = member;
                    break;
                }

            }

        } /* if (!(member->Flags & GFLG_DISABLED)) */

    } /* while((member = NextObject(&memberstate))) */

    return rc;
}

/****************************************************************************************/

IPTR GroupGClass__GM_HANDLEINPUT(Class *cl, struct Gadget *g, struct gpInput *msg)
{
    struct GroupGData   *data = INST_DATA(cl, g);
    struct gpInput  	 m;
    IPTR            	 rc;

    /* If someone activates us with ActivateGadget(), activegad will be NULL.
     * In that case, activate the first object.
     */
    if (!data->activegad)
    {
        struct Gadget *memberstate = (struct Gadget *)data->memberlist.mlh_Head;
	
        data->activegad = NextObject(&memberstate);
    }

    m = *msg;

    /* gpi_Mouse coords are relative to group gadget. Make them relative
       to activate object */

    m.gpi_Mouse.X = g->LeftEdge + msg->gpi_Mouse.X - data->activegad->LeftEdge;
    m.gpi_Mouse.Y = g->TopEdge  + msg->gpi_Mouse.Y - data->activegad->TopEdge;

    rc = DoMethodA((Object *)data->activegad, (Msg)&m);

#if SUPPORT_TABCYCLE
    {
        struct Gadget *newgad = NULL;

        if (rc & GMR_NEXTACTIVE) newgad = next_tabcycleobject(cl, g);
        if (rc & GMR_PREVACTIVE) newgad = prev_tabcycleobject(cl, g);

        if (newgad && (newgad != data->activegad))
        {
            struct gpGoInactive im;

            /* Make old member gadget inactive */

            im.MethodID   = GM_GOINACTIVE;
            im.gpgi_GInfo = msg->gpi_GInfo;
            im.gpgi_Abort = 0;          /* The gadget itself wanted to be deactivated */

            DoMethodA((Object *)data->activegad, (Msg)&im);

            /* Make new member gadget active */

            data->activegad = newgad;

            m.MethodID    = GM_GOACTIVE;
            m.gpi_Mouse.X = g->LeftEdge + msg->gpi_Mouse.X - newgad->LeftEdge;
            m.gpi_Mouse.Y = g->TopEdge  + msg->gpi_Mouse.Y - newgad->TopEdge;

            rc = DoMethodA((Object *)newgad, (Msg)&m);

        }

    }
#endif

    return rc;
}

/****************************************************************************************/

IPTR GroupGClass__GM_GOINACTIVE(Class *cl, struct Gadget *g, struct gpGoInactive *msg)
{
    struct GroupGData   *data = INST_DATA(cl, g);
    IPTR            	 rc;

    ASSERT_VALID_PTR(data->activegad);

    rc = DoMethodA((Object *)data->activegad, (Msg)msg);
    data->activegad = NULL;

    return rc;
}

/****************************************************************************************/

IPTR GroupGClass__GM_RENDER(Class *cl, struct Gadget *g, struct gpRender *msg)
{
    struct GroupGData	*data = INST_DATA(cl, g);
    struct Gadget	*member, *memberstate;

    memberstate = (struct Gadget *)data->memberlist.mlh_Head;
    while((member = NextObject(&memberstate)))
    {
        DoMethodA((Object *)member, (Msg)msg);
    }

    return 0;
}

/****************************************************************************************/
