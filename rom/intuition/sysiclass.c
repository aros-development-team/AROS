/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Implementation of SYSICLASS
    Lang: english
*/

#include <exec/types.h>

#include <proto/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>

#include <proto/graphics.h>
#include <graphics/rastport.h>

#include <proto/utility.h>
#include <utility/tagitem.h>

#include <proto/alib.h>

#include <aros/asmcall.h>
#include "intuition_intern.h"

/****************************************************************************/

/* Some handy transparent base class object casting defines.
 */
#define IM(o) ((struct Image *)o)

#undef IntuitionBase
#define IntuitionBase	((struct IntuitionBase *)(cl->cl_UserData))

/****************************************************************************/

struct SysIData
{
    ULONG type;
    struct DrawInfo *dri;
    struct Image *frame;
};

/****************************************************************************/

BOOL sysi_setnew(Class *cl, Object *obj, struct opSet *msg)
{
    struct SysIData *data = INST_DATA(cl, obj);
    struct TagItem *taglist, *tag;

    taglist = msg->ops_AttrList;
    while ((tag = NextTagItem(&taglist)))
    {
	switch(tag->ti_Tag)
	{
	case SYSIA_DrawInfo:
	    data->dri = (struct DrawInfo *)tag->ti_Data;
	    break;
	case SYSIA_Which:
	    data->type = tag->ti_Data;
	    break;
	case SYSIA_ReferenceFont:
	    /* !!! */
	    break;
	case SYSIA_Size:
	    /* !!! */
	    break;
	}
    }

    if ((!data->dri) || ((data->type != CHECKIMAGE) && (data->type != MXIMAGE)))
	return FALSE;
    return TRUE;
}


Object *sysi_new(Class *cl, Class *rootcl, struct opSet *msg)
{
    struct SysIData *data;
    Object *obj;

    obj = (Object *)DoSuperMethodA(cl, (Object *)rootcl, (Msg)msg);
    if (!obj)
        return NULL;

    data = INST_DATA(cl, obj);
    data->type = 0L;
    data->dri = NULL;
    data->frame = NULL;
    if (!sysi_setnew(cl, obj, (struct opSet *)msg))
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return NULL;
    }

    switch (data->type)
    {
    case CHECKIMAGE:
    case MXIMAGE:
    {
        struct TagItem tags[] = {
          {IA_FrameType, FRAME_BUTTON},
          {IA_EdgesOnly, FALSE},
          {TAG_MORE, 0L}
        };

        tags[2].ti_Data = (IPTR)msg->ops_AttrList;
        data->frame = NewObjectA(NULL, FRAMEICLASS, tags);
        if (!data->frame)
        {
            CoerceMethod(cl, obj, OM_DISPOSE);
            obj = NULL;
        }
        break;
    }

    default:
        CoerceMethod(cl, obj, OM_DISPOSE);
        obj = NULL;
        break;
    }

    return obj;
}


ULONG CHECKIMAGEDEF[] =
{ 0x00000000,
  0x00000000,
  0x000000d0,
  0x00000180,
  0x00000300,
  0x00068600,
  0x0001cc00,
  0x0000f100,
  0x00007000,
  0x00000000,
  0x00000000
};

void sysi_draw(Class *cl, Object *obj, struct impDraw *msg)
{
    struct SysIData *data = INST_DATA(cl, obj);
    struct RastPort *rport = msg->imp_RPort;
    WORD left = IM(obj)->LeftEdge + msg->imp_Offset.X;
    WORD top = IM(obj)->TopEdge + msg->imp_Offset.Y;
    UWORD width = IM(obj)->Width;
    UWORD height = IM(obj)->Height;

    EraseRect(rport, left, top, left + width - 1, top + height - 1);

    switch(data->type)
    {
    case CHECKIMAGE:
    {
        /* Draw frame */
        DrawImageState(rport, data->frame,
                       msg->imp_Offset.X, msg->imp_Offset.Y,
                       IDS_NORMAL, data->dri);

        /* Draw checkmark (only if image is in selected state) */
        if (msg->imp_State == IDS_SELECTED)
        {
            int x, y;

            SetAPen(rport, data->dri->dri_Pens[TEXTPEN]);
       	    SetDrMd(rport, JAM1);

            for (y=0; y<height; y++)
            {
                for (x=0; x<width; x++)
                {
                    WORD currentx = x * 26 / width, currenty = y * 11 / height;
                    if ((CHECKIMAGEDEF[currenty] & (2<<(25-currentx))))
                    {
                        Move(rport, left + currentx, top + currenty);
                        Draw(rport, left + currentx, top + currenty);
                    }
                }
            }
        }
        break;
    }
    case MXIMAGE:
    {
        struct TagItem tags[] = {
          { IA_Recessed, FALSE },
          { TAG_DONE, 0UL }
        };

        /* Draw frame */
        if (msg->imp_State == IDS_SELECTED)
            tags[0].ti_Data = TRUE;
        SetAttrsA(data->frame, tags);
        DrawImageState(rport, data->frame,
                       msg->imp_Offset.X, msg->imp_Offset.Y,
                       IDS_NORMAL, data->dri);

        if (msg->imp_State == IDS_SELECTED)
        {
            SetABPenDrMd(rport, data->dri->dri_Pens[FILLPEN], 0, JAM1);
            RectFill(rport, left + 4, top + 2, left + width - 5, top + height - 3);
        }
        break;
    }
    }
    return;
}

/****************************************************************************/

AROS_UFH3(static IPTR, dispatch_sysiclass,
    AROS_UFHA(Class *,  cl,  A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(Msg,      msg, A1)
)
{
    IPTR retval = 0UL;
    struct SysIData *data;

    switch (msg->MethodID)
    {
    case OM_NEW:
        retval = (IPTR)sysi_new(cl, (Class *)obj, (struct opSet *)msg);
        break;

    case OM_DISPOSE:
        data = INST_DATA(cl, obj);
        DisposeObject(data->frame);
        retval = DoSuperMethodA(cl, obj, msg);
        break;

    case OM_SET:
        data = INST_DATA(cl, obj);
        if (data->frame)
            DoMethodA((Object *)data->frame, msg);
        retval = DoSuperMethodA(cl, obj, msg);

    case IM_DRAW:
        sysi_draw(cl, obj, (struct impDraw *)msg);
        break;

    default:
        retval = DoSuperMethodA(cl, obj, msg);
        break;
    }

    return retval;
}

#undef IntuitionBase

/****************************************************************************/

/* Initialize our class. */
struct IClass *InitSysIClass (struct IntuitionBase * IntuitionBase)
{
    struct IClass *cl = NULL;

    cl = MakeClass(SYSICLASS, IMAGECLASS, NULL, sizeof(struct SysIData), 0);
    if (cl == NULL)
        return NULL;

    cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_sysiclass);
    cl->cl_Dispatcher.h_SubEntry = NULL;
    cl->cl_UserData              = (IPTR)IntuitionBase;

    AddClass (cl);

    return (cl);
}
