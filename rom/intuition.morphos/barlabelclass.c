/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
 
    AROS menubarlabelclass implementation (does not exist in AmigaOS)
*/

#include <exec/types.h>

#include <dos/dos.h>
#include <dos/dosextens.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>

#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <graphics/rpattr.h>

#include <cybergraphx/cybergraphics.h>

#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <clib/macros.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/cybergraphics.h>

#ifndef __MORPHOS__
#include <aros/asmcall.h>
#include <proto/alib.h>
#include "intuition_intern.h"
#endif /* !__MORPHOS__ */

#ifdef SKINS
#   include "intuition_customize.h"
#   include "intuition_extend.h"
#endif

#ifdef IntuitionBase
#    undef IntuitionBase
#endif

#define IntuitionBase   ((struct IntuitionBase *)(cl->cl_UserData))

/*************************** BarLabelClass *****************************/

struct MenuBarLabelData
{
    struct DrawInfo *dri;
};

AROS_UFH3S(IPTR, dispatch_menubarlabelclass,
           AROS_UFHA(Class *, cl, A0),
           AROS_UFHA(Object *, obj, A2),
           AROS_UFHA(Msg, msg, A1)
          )
{
    AROS_USERFUNC_INIT

    struct MenuBarLabelData *data;
    struct RastPort         *rp;
    struct TagItem          *ti;
    WORD            	     x1, y1, x2, y2;

    IPTR            	     retval = 0UL;

    switch (msg->MethodID)
    {
	case OM_NEW:
            obj = (Object *)DoSuperMethodA(cl, obj, msg);
            if (obj)
            {
        	data = INST_DATA(cl, obj);
        	data->dri = NULL;

        	retval = (IPTR)obj;
            }
            break;

	case OM_SET:
            if ((ti = FindTagItem(SYSIA_DrawInfo, ((struct opSet *)msg)->ops_AttrList)))
            {
        	data = INST_DATA(cl, obj);

        	data->dri = (struct DrawInfo *)ti->ti_Data;
            }
            retval = DoSuperMethodA(cl, obj, msg);
            break;

	case OM_GET:
            switch(((struct opGet *)msg)->opg_AttrID)
            {
        	case IA_SupportsDisable:
        	    if (MENUS_AMIGALOOK)
        	    {
                	*(((struct opGet *)msg)->opg_Storage) = 0;
        	    }
        	    else
        	    {
                	*(((struct opGet *)msg)->opg_Storage) = 1;
        	    }

        	    retval = 1;
        	    break;

        	default:
        	    retval = DoSuperMethodA(cl, obj, msg);
        	    break;
            }
            break;

	case IM_DRAW:
            data = INST_DATA(cl, obj);

            if (data->dri)
            {
        	rp = ((struct impDraw *)msg)->imp_RPort;

        	if (!rp) break;

        	SetDrMd(rp, JAM1);

        	x1 = ((struct Image *)obj)->LeftEdge + ((struct impDraw *)msg)->imp_Offset.X;
        	y1 = ((struct Image *)obj)->TopEdge  + ((struct impDraw *)msg)->imp_Offset.Y;
        	x2 = x1 + ((struct Image *)obj)->Width  - 1;
        	y2 = y1 + ((struct Image *)obj)->Height - 1;


        	if (MENUS_AMIGALOOK)
        	{
                    SetAPen(rp, data->dri->dri_Pens[BARDETAILPEN]);
                    RectFill(rp, x1, y1, x2, y2);
        	}
        	else
        	{
                    /* Will only work if imageheight = 2 */
                    SetAPen(rp, data->dri->dri_Pens[SHADOWPEN]);
                    RectFill(rp, x1, y1, x2 - 1, y1);
                    WritePixel(rp, x1, y2);

                    SetAPen(rp, data->dri->dri_Pens[SHINEPEN]);
                    RectFill(rp, x1 + 1, y2, x2, y2);
                    WritePixel(rp, x2, y1);
        	}
		
            } /* if (data->dri) */
            break;

	default:
            retval = DoSuperMethodA(cl, obj, msg);
            break;

    } /* switch (msg->MethodID) */

    return retval;

    AROS_USERFUNC_EXIT
}

/****************************************************************************/

#undef IntuitionBase

/****************************************************************************/

struct IClass *InitMenuBarLabelClass (struct IntuitionBase * IntuitionBase)
{
    struct IClass *cl;

    if ( (cl = MakeClass(MENUBARLABELCLASS, IMAGECLASS, NULL, sizeof(struct MenuBarLabelData), 0)) )
    {
        cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_menubarlabelclass);
        cl->cl_Dispatcher.h_SubEntry = NULL;
        cl->cl_UserData              = (IPTR)IntuitionBase;

        AddClass (cl);
    }

    return (cl);
}

