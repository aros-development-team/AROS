/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics gc class implementation.
    Lang: english
*/

/****************************************************************************************/

#include <string.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <exec/memory.h>
#include <graphics/text.h>
#include <utility/tagitem.h>
#include <oop/oop.h>

#include <hidd/graphics.h>

#include "graphics_intern.h"

#undef  SDEBUG
#define SDEBUG 0
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************************

    NAME
        aoHidd_GC_UserData

    SYNOPSIS
        [.SG]

    LOCATION
        hidd.graphics.gc

    FUNCTION
        User data

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
    aoHidd_GC_BitMap

    SYNOPSIS
        [I.G]

    LOCATION
        hidd.graphics.gc

    FUNCTION
        Bitmap which this gc uses.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_GC_Foreground

    SYNOPSIS
        [.SG]

    LOCATION
        hidd.graphics.gc

    FUNCTION
        Foreground color

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_GC_Background

    SYNOPSIS
        [.SG]

    LOCATION
        hidd.graphics.gc

    FUNCTION
        Background color

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_GC_DrawMode

    SYNOPSIS
        [.SG]

    LOCATION
        hidd.graphics.gc

    FUNCTION
        Draw mode

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_GC_Font

    SYNOPSIS
        [.SG]

    LOCATION
        hidd.graphics.gc

    FUNCTION
        Current font

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_GC_ColorMask

    SYNOPSIS
        [.SG]

    LOCATION
        hidd.graphics.gc

    FUNCTION
        Prevents some color bits from changing.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_GC_LinePattern

    SYNOPSIS
        [.SG]

    LOCATION
        hidd.graphics.gc

    FUNCTION
        Pattern for line drawing
    
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_GC_LinePatternCnt

    SYNOPSIS
        [.SG]

    LOCATION
        hidd.graphics.gc

    FUNCTION
        Pattern start bit for line drawing.
    
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_GC_PlaneMask

    SYNOPSIS
        [.SG]

    LOCATION
        hidd.graphics.gc

    FUNCTION
        Shape bitmap 

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_GC_ColorExpansionMode

    SYNOPSIS
        [.SG]

    LOCATION
        hidd.graphics.gc

    FUNCTION
        Mode for color expansion
    
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/


VOID GC__Root__Set(OOP_Class *cl, OOP_Object *obj, struct pRoot_Set *msg);

#define IS_GC_ATTR(attr, idx) ( ( (idx) = (attr) - HiddGCAttrBase) < num_Hidd_GC_Attrs)

/****************************************************************************************/

OOP_Object *GC__Root__New(OOP_Class *cl, OOP_Object *obj, struct pRoot_New *msg)
{
    HIDDT_GC_Intern *data;

    EnterFunc(bug("GC::New()\n"));

    obj  = (OOP_Object *) OOP_DoSuperMethod(cl, obj, (OOP_Msg) msg);

    if(obj)
    {
        data = OOP_INST_DATA(cl, obj);
    
        /* clear all data and set some default values */

        memset(data, 0, sizeof (*data));

        data->fg        = 1;        /* foreground color                        */
        data->bg        = 0;        /* background color                        */
        data->drMode    = vHidd_GC_DrawMode_Copy;    /* drawmode               */
        data->colExp    = vHidd_GC_ColExp_Opaque;    /* color expansion mode   */
        data->font      = NULL;     /* current fonts                           */
        data->colMask   = ~0;       /* ColorMask prevents some color bits from changing*/
        data->linePat   = ~0;       /* LinePattern                             */
        data->planeMask = NULL;     /* Pointer to a shape bitMap               */

        /* Override defaults with user suplied attrs */

	OOP_SetAttrs(obj, msg->attrList);
    	/* GC__Root__Set(cl, obj, &set_msg); */

    } /* if(obj) */

    ReturnPtr("GC::New", OOP_Object *, obj);
}

/****************************************************************************************/

VOID GC__Root__Set(OOP_Class *cl, OOP_Object *obj, struct pRoot_Set *msg)
{
    HIDDT_GC_Intern *data = OOP_INST_DATA(cl, obj);
    struct TagItem  *tag, *tstate;
    ULONG   	    idx;

    EnterFunc(bug("GC::Set()\n"));

    tstate = msg->attrList;
    while((tag = NextTagItem((const struct TagItem **)&tstate)))
    {
        if(IS_GC_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
                case aoHidd_GC_Foreground:
		    data->fg = tag->ti_Data;
		    break;
		    
                case aoHidd_GC_Background:
		    data->bg = tag->ti_Data;
		    break;
		    
                case aoHidd_GC_DrawMode:
		    data->drMode = tag->ti_Data;
		    break;
		    
                case aoHidd_GC_Font:
		    data->font = (APTR) tag->ti_Data;
		    break;
		    
                case aoHidd_GC_ColorMask:
		    data->colMask = tag->ti_Data;
		    break;
		    
                case aoHidd_GC_LinePattern: 
		    data->linePat = (UWORD) tag->ti_Data;
		    break;

                case aoHidd_GC_LinePatternCnt: 
		    data->linePatCnt = (UWORD) tag->ti_Data;
		    break;
		    
                case aoHidd_GC_PlaneMask:
		    data->planeMask = (APTR) tag->ti_Data;
		    break;

                case aoHidd_GC_ColorExpansionMode:
		    data->colExp = tag->ti_Data;
		    break;
            }
        }
    }

    ReturnVoid("GC::Set");
}

/****************************************************************************************/

VOID GC__Root__Get(OOP_Class *cl, OOP_Object *obj, struct pRoot_Get *msg)
{
    HIDDT_GC_Intern *data = OOP_INST_DATA(cl, obj);
    ULONG   	    idx;

    EnterFunc(bug("GC::Get() attrID: %i  storage: %p\n", msg->attrID, msg->storage));

    if(IS_GC_ATTR(msg->attrID, idx))
    {
        switch(idx)
        {
            case aoHidd_GC_Foreground:
	    	 *msg->storage = data->fg;
		 break;
		 
            case aoHidd_GC_Background:
	    	 *msg->storage = data->bg;
		 break;
		 
            case aoHidd_GC_DrawMode:
	    	*msg->storage = data->drMode;
		break;
		
            case aoHidd_GC_Font:
	    	*msg->storage = (IPTR)data->font;
		break;
		
            case aoHidd_GC_ColorMask:
	    	*msg->storage = data->colMask;
		break;
		
            case aoHidd_GC_LinePattern:
	    	*msg->storage = data->linePat;
		break;

            case aoHidd_GC_LinePatternCnt:
	    	*msg->storage = data->linePatCnt;
		break;
		
            case aoHidd_GC_PlaneMask:
	    	*msg->storage = (IPTR)data->planeMask;
		break;
		
            case aoHidd_GC_ColorExpansionMode:
	    	*msg->storage = data->colExp;
		break;
		
	    default:
	    	OOP_DoSuperMethod(cl, obj, (OOP_Msg) msg);
		break;
        }
    }
    else
    {
        OOP_DoSuperMethod(cl, obj, (OOP_Msg) msg);
    }

}

/*****************************************************************************************

    NAME
        moHidd_GC_SetClipRect

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_GC_SetClipRect *msg);

        VOID HIDD_GC_SetClipRect(OOP_Object *obj, LONG x1, LONG y1, LONG x2, LONG y2);

    LOCATION
        hidd.graphics.gc

    FUNCTION

    INPUTS
        obj -
        x1  -
        y1  -
        x2  -
        y2  -

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

VOID GC__Hidd_GC__SetClipRect(OOP_Class *cl, OOP_Object *o, struct pHidd_GC_SetClipRect *msg)
{
     HIDDT_GC_Intern *data;
     
     data = OOP_INST_DATA(cl, o);
     
     data->clipX1 = msg->x1;
     data->clipY1 = msg->y1;
     data->clipX2 = msg->x2;
     data->clipY2 = msg->y2;
     
     data->doClip = TRUE;
}

/*****************************************************************************************

    NAME
        moHidd_GC_UnsetClipRect

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_GC_UnsetClipRect *msg);

        VOID HIDD_GC_UnsetClipRect(OOP_Object *obj);

    LOCATION
        hidd.graphics.gc

    FUNCTION

    INPUTS
        obj -

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

VOID GC__Hidd_GC__UnsetClipRect(OOP_Class *cl, OOP_Object *o, struct pHidd_GC_UnsetClipRect *msg)
{
     HIDDT_GC_Intern *data;
     
     data = OOP_INST_DATA(cl, o);
     
     data->doClip = FALSE;
    
}
