/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#ifdef _SASC

#define USE_SYSBASE
#define USE_BUILTIN_MATH
#define INTUI_V36_NAMES_ONLY

#endif

#include <exec/types.h>

#include <dos/dos.h>
#include <dos/dosextens.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <intuition/cghooks.h>
#include <intuition/icclass.h>

#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>

#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <clib/macros.h>

#include <string.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#ifndef __MORPHOS__
#include "intuition_intern.h"
#include <aros/asmcall.h>
#include <proto/alib.h>
#endif /* !__MORPHOS__ */

/***********************************************************************************/

/* Set if to 1 to enable kprintf debugging
 */
#if 0
#define D(x) x
#else
#define D(x)
#endif

#define DEBUG_HIT(x)    ;

/***********************************************************************************/

/* Some handy transparent base class object casting defines.
 */
#define G(o)  ((struct Gadget *)o)
#define EG(o) ((struct ExtGadget *)o)
#define IM(o) ((struct Image *)o)

/***********************************************************************************/

#undef IntuitionBase
#define IntuitionBase   ((struct IntuitionBase *)(cl->cl_UserData))

/***********************************************************************************/

AROS_UFH3S(IPTR, dispatch_imageclass,
           AROS_UFHA(Class *,  cl,  A0),
           AROS_UFHA(Object *, o,   A2),
           AROS_UFHA(Msg,      msg, A1)
          )
{
    AROS_USERFUNC_INIT

    IPTR retval = 0UL;

    switch (msg->MethodID)
    {
	case OM_NEW:
            D(kprintf("ImageClass OM_NEW\n"));

            if (cl)
            {
        	retval = (IPTR)DoSuperMethodA(cl, o, msg);

        	if(retval)
        	{
                    /*
                    This is how Intuition knows an image is a boopsi
                    object!
                    */
                    /*
                      The instance object is contains cleared memory!
                      memset ((void *)retval, 0, (cl->cl_InstOffset + cl->cl_InstSize));
                    */
                    IM(retval)->Width = 80;
                    IM(retval)->Height = 40;
                    IM(retval)->Depth = CUSTOMIMAGEDEPTH;
        	}
            }

            o = (Object *)retval;
            /*
            Fall through -> allow the class the set all the initial
            attributes
            */

	case OM_SET:
        {
            struct TagItem *tstate = ((struct opSet *)msg)->ops_AttrList;
            struct TagItem *tag;
            IPTR 	    	tidata;
            BOOL 	    	unsupported = FALSE;

            D( kprintf("ImageClass OM_SET\n") );

            while ((tag = NextTagItem(&tstate)))
            {
                tidata = tag->ti_Data;

                switch (tag->ti_Tag)
                {
                    case IA_Left:
                	IM(o)->LeftEdge = (WORD) tidata;
                	break;

                    case IA_Top:
                	IM(o)->TopEdge = (WORD) tidata;
                	break;

                    case IA_Width:
                	IM(o)->Width = (WORD) tidata;
                	break;

                    case IA_Height:
                	IM(o)->Height = (WORD) tidata;
                	break;

                    case IA_FGPen:
                	IM(o)->PlanePick = (WORD) tidata;
                	break;

                    case IA_BGPen:
                	IM(o)->PlaneOnOff = (WORD) tidata;
                	break;

                    case IA_Data:
                	IM(o)->ImageData = (UWORD *) tidata;
                	break;

                    default:
                	unsupported = TRUE;
                	break;

                } /* switch (Tag) */

            } /* while (Tag) */

            /*
            If all attributes were supported and there is no retval yet,
            set retval to 1.
            */
            if (!unsupported && !retval)
                retval = 1UL;
            /*
            Because we are a direct subclass of rootclass
            which has no settable/gettable attributes we
            we will NOT pass this method to our superclass!
            */
            break;
        }

	case OM_GET:
            D( kprintf("ImageClass OM_GET\n") );

            retval = 1UL;

            switch (((struct opGet *)msg)->opg_AttrID)
            {
        	case IA_Left:
        	    *((struct opGet *)msg)->opg_Storage = (IPTR) IM(o)->LeftEdge;
        	    break;

        	case IA_Top:
        	    *((struct opGet *)msg)->opg_Storage = (IPTR) IM(o)->TopEdge;
        	    break;

        	case IA_Width:
        	    *((struct opGet *)msg)->opg_Storage = (IPTR) IM(o)->Width;
        	    break;

        	case IA_Height:
        	    *((struct opGet *)msg)->opg_Storage = (IPTR) IM(o)->Height;
        	    break;

        	case IA_FGPen:
        	    *((struct opGet *)msg)->opg_Storage = (IPTR) IM(o)->PlanePick;
        	    break;

        	case IA_BGPen:
        	    *((struct opGet *)msg)->opg_Storage = (IPTR) IM(o)->PlaneOnOff;
        	    break;

        	case IA_Data:
        	    *((struct opGet *)msg)->opg_Storage = (IPTR) IM(o)->ImageData;
        	    break;

        	default:
        	    retval = 0UL;
        	    break;

            } /* switch */

            /*
            Because we are a direct subclass of rootclass
            which has no settable/gettable attributes we
            we will NOT pass this method to our superclass!
            */
            break;

	case IM_ERASE:
            /*
            Both erase methods are documented as handled the same
            at this level, so we will fall thru...
            */
	case IM_ERASEFRAME:
        {
            WORD left, top, width, height;

            D(kprintf("ImageClass IM_ERASE(FRAME)\n") );

            left   = IM(o)->LeftEdge + ((struct impErase *)msg)->imp_Offset.X;
            top    = IM(o)->TopEdge + ((struct impErase *)msg)->imp_Offset.Y;
            width  = IM(o)->Width - 1;
            height = IM(o)->Height - 1;

            if (((struct impErase *)msg)->imp_RPort)
	    {
        	EraseRect(((struct impErase *)msg)->imp_RPort,
                	  left, top,
                	  left + width, top + height);
    	    }
            /* Leave retval=0: No further rendering necessary */
            break;
        }

	case IM_HITFRAME:
	case IM_HITTEST:
        {
            struct impHitTest *imp = (struct impHitTest *)msg;

            /*
            Loosing my sanity, better check that I do not
            have my X/Y mixed up here. :)
            */
            if ((imp->imp_Point.X >= IM(o)->LeftEdge && imp->imp_Point.X < IM(o)->LeftEdge + IM(o)->Width) &&
                    (imp->imp_Point.Y >= IM(o)->TopEdge  && imp->imp_Point.Y < IM(o)->TopEdge + IM(o)->Height))
	    {
                retval = 1UL;
	    }
	    
            DEBUG_HIT(dprintf("image: HITTEST %d %d (%d,%d) %d×%d = %d\n", imp->imp_Point.X, imp->imp_Point.Y,
                              IM(o)->LeftEdge, IM(o)->TopEdge, IM(o)->Width, IM(o)->Height, retval));
            break;
        }

    #if 0 /* for imageclass, IM_HITFRAME==IM_HITTEST */
	case IM_HITFRAME:
	{
            struct impHitTest *imp = (struct impHitTest *)msg;

            /*
            Loosing my sanity, better check that I do not
            have my X/Y mixed up here. :)
            */
            if ((imp->imp_Point.X >= IM(o)->LeftEdge && imp->imp_Point.X < IM(o)->LeftEdge + imp->imp_Dimensions.Width) &&
                    (imp->imp_Point.Y >= IM(o)->TopEdge  && imp->imp_Point.Y < IM(o)->TopEdge + imp->imp_Dimensions.Height))
        	retval = 1UL;
            break;
	}
    #endif

            /* case OM_DISPOSE */
	default:
            retval = DoSuperMethodA(cl, o, msg);
            break;

    } /* switch */

    return (retval);

    AROS_USERFUNC_EXIT
} /* dispatch_imageclass */

/***********************************************************************************/

#undef IntuitionBase

/***********************************************************************************/

struct IClass *InitImageClass (struct IntuitionBase * IntuitionBase)
{
    struct IClass *cl = NULL;

    /* This is the code to make the image class...
    */
    if ((cl = MakeClass(IMAGECLASS, ROOTCLASS, NULL, sizeof(struct Image), 0)))
    {
        cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_imageclass);
        cl->cl_Dispatcher.h_SubEntry = NULL;
        cl->cl_UserData              = (IPTR)IntuitionBase;

        AddClass (cl);
    }

    return (cl);
}

/***********************************************************************************/
