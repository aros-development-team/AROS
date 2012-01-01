/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    HIDDT_GC_Intern *data;

    EnterFunc(bug("GC::New()\n"));

    obj  = (OOP_Object *) OOP_DoSuperMethod(cl, obj, (OOP_Msg) msg);

    if(obj)
    {
        data = OOP_INST_DATA(cl, obj);
    
        /* clear all data and set some default values */
        data->fg        = 1;        /* foreground color                        */
        data->bg        = 0;        /* background color                        */
        data->drMode    = vHidd_GC_DrawMode_Copy;    /* drawmode               */
        data->colExp    = vHidd_GC_ColExp_Opaque;    /* color expansion mode   */
        data->colMask   = ~0;       /* ColorMask prevents some color bits from changing*/
        data->linePat   = ~0;       /* LinePattern                             */

        /* Override defaults with user suplied attrs */

	OOP_SetAttrs(obj, msg->attrList);
    	/* GC__Root__Set(cl, obj, &set_msg); */

    } /* if(obj) */

    ReturnPtr("GC::New", OOP_Object *, obj);
}

/****************************************************************************************/

VOID GC__Root__Set(OOP_Class *cl, OOP_Object *obj, struct pRoot_Set *msg)
{
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    HIDDT_GC_Intern *data = OOP_INST_DATA(cl, obj);
    struct TagItem  *tag, *tstate;
    ULONG   	    idx;

    EnterFunc(bug("GC::Set()\n"));

    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
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
		    
                case aoHidd_GC_ColorMask:
		    data->colMask = tag->ti_Data;
		    break;
		    
                case aoHidd_GC_LinePattern: 
		    data->linePat = (UWORD) tag->ti_Data;
		    break;

                case aoHidd_GC_LinePatternCnt: 
		    data->linePatCnt = (UWORD) tag->ti_Data;
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

            case aoHidd_GC_ColorMask:
	    	*msg->storage = data->colMask;
		break;

            case aoHidd_GC_LinePattern:
	    	*msg->storage = data->linePat;
		break;

            case aoHidd_GC_LinePatternCnt:
	    	*msg->storage = data->linePatCnt;
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
    	Install a clipping rectangle on a GC.

    INPUTS
        obj    - a GC object
        x1, y1 - top-left coordinate of the clipping rectangle
        x2, y2 - bottom-right coordinate of the clipping rectangle

    RESULT
    	None

    NOTES
    	Since the GC is just a data container, installing clipping rectangle doesn't magically
    	applies it to all operations. Graphics driver method which uses the GC needs to support
    	it explicitly. Currently clipping is supported only by Draw and DrawEllipse methods.

	Use this method if and only if the GC object was created by you. graphics.library
	internally operates on temporary GC objects, which are allocated only partially. They
	don't have storage space for clipping rectangle data, and attempt to use this
	method on such a GC will result in memory trashing.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

VOID GC__Hidd_GC__SetClipRect(OOP_Class *cl, OOP_Object *o, struct pHidd_GC_SetClipRect *msg)
{
     struct gc_data *data = OOP_INST_DATA(cl, o);

     /* A space for struct Rectangle has been allocated together with the object */
     data->cr.MinX = msg->x1;
     data->cr.MinY = msg->y1;
     data->cr.MaxX = msg->x2;
     data->cr.MaxY = msg->y2;

     /*
      * Set clipRect pointer to our own rectangle.
      * clipRect is intentionally a pointer, not embedded structure. This is done
      * in order to support temporary GCs embedded in a RastPort for graphics.library.
      * There's not enough space to hold struct Rectangle in embedded GC.
      */
     data->prot.clipRect = &data->cr;
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
    	Uninstalls the clipping rectangle (whatever it is) from the GC.

    INPUTS
        obj - a GC object

    RESULT
    	None

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

VOID GC__Hidd_GC__UnsetClipRect(OOP_Class *cl, OOP_Object *o, struct pHidd_GC_UnsetClipRect *msg)
{
     HIDDT_GC_Intern *data = OOP_INST_DATA(cl, o);

     /* Reset clipRect pointer */
     data->clipRect = NULL;
}
