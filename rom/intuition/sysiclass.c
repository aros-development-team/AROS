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

/****************************************************************************/

struct SysIData
{
    ULONG type;
    struct DrawInfo *dri;
    struct Image *frame;
};

/****************************************************************************/

#undef IntuitionBase
#define IntuitionBase	((struct IntuitionBase *)(cl->cl_UserData))

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

    if ((!data->dri) || (data->type != CHECKIMAGE))
	return FALSE;
    return TRUE;
}


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
    {
	obj = (Object *)DoSuperMethodA(cl, obj, msg);
	if (!obj)
	    return NULL;
	data = INST_DATA(cl, obj);
	data->frame = NULL;
	if (!sysi_setnew(cl, obj, (struct opSet *)msg))
	{
	    CoerceMethod(cl, obj, OM_DISPOSE);
	    return NULL;
	}
	switch (data->type)
	{
	case CHECKIMAGE:
	{
	    struct TagItem tags[4] = {
	      {IA_Width, IM(obj)->Width},
	      {IA_Height, IM(obj)->Height},
	      {IA_FrameType, FRAME_BUTTON},
	      {TAG_DONE, 0L}
	    };

	    data->frame = NewObjectA(NULL, FRAMEICLASS, tags);
	    if (!data->frame)
	    {
		CoerceMethod(cl, obj, OM_DISPOSE);
		return NULL;
	    }
	}
	}
	retval = (IPTR)obj;
	break;
    }

    case OM_DISPOSE:
	data = INST_DATA(cl, obj);
	DisposeObject(data->frame);
	retval = DoSuperMethodA(cl, obj, msg);
	break;

#define IMPD(x) ((struct impDraw *)(x))
    case IM_DRAW:
    {
	struct RastPort *rport = IMPD(msg)->imp_RPort;
	WORD left = IM(obj)->LeftEdge + IMPD(msg)->imp_Offset.X;
	WORD top = IM(obj)->TopEdge + IMPD(msg)->imp_Offset.Y;
	UWORD width = IM(obj)->Width, height = IM(obj)->Height;

	data = INST_DATA(cl, obj);
	switch(data->type)
	{
	case CHECKIMAGE:
	{
	    DrawImageState(rport, data->frame, left, top, IDS_NORMAL,
			   data->dri);
	    if (IMPD(msg)->imp_State == IDS_SELECTED)
	    {
	        int x, y;

		SetAPen(rport, data->dri->dri_Pens[TEXTPEN]);
		SetDrMd(rport, JAM1);
		x = left + (width / 4);
		for (y=(height/2); y<(height/2)+(height/3); y++)
		{
		    Move(rport, x, top + y);
		    Draw(rport, x + (width / 10), top + y);
		    x++;
		}
		x++;
		for (y=(height/2)+(height/3)-1; y<(height/5); y--)
		{
		    Move(rport, x, top + y);
		    Draw(rport, x + (width / 15), top + y);
		    x++;
		}
		Draw(rport, x + (width / 10) - 1, top + y + 1);
	    }
	}
	}
	break;
    }
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

    if ((cl = MakeClass(SYSICLASS, IMAGECLASS, NULL, sizeof(struct SysIData), 0)))
    {
	cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_sysiclass);
	cl->cl_Dispatcher.h_SubEntry = NULL;
	cl->cl_UserData 	     = (IPTR)IntuitionBase;

	AddClass (cl);
    }

    return (cl);
}
