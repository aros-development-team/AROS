/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Graphics bitmap class implementation.
    Lang: english
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <exec/libraries.h>


#include <utility/tagitem.h>
#include <hidd/graphics.h>

#include "gfxhidd_intern.h"


#define DEBUG 1
#include <aros/debug.h>



/********************
**  BitMap::New()  **
********************/
static Object *bitmap_new(Class *cl, Object *obj, struct P_Root_New *msg)
{
    EnterFunc(bug("BM_BitMapClass - OM_NEW:\n"));

    obj  = (Object *) DoSuperMethodA(cl, obj, msg);
    data = INST_DATA(cl, obj);

    if(obj)
    {
	while((tag = NextTagItem(&OPS(msg)->ops_AttrList)))
	{
	    switch(tag->ti_Tag)
	    {
		case aHidd_BitMap_Width      : width    = tag->ti_Data; break;
		case aHidd_BitMap_Height     : height   = tag->ti_Data; break;
		case aHidd_BitMap_Depth      : depth    = tag->ti_Data; break;
		case aHidd_BitMap_Displayable: if(tag->ti_Data) flags = flags | HIDD_Graphics_BitMap_Flags_Displayable; break;

		default: D(bug("  unknown attribute %li\n", tag->ti_Data)); break;
	    } /* switch tag */
	} /* while tag  */

	data->bitMap = HIDD_Graphics_CreateBitMap(width, height, depth,
                                                         flags, NULL, NULL);
	if(!data->bitMap)
	{
	     DisposeObject(obj);
	    obj = NULL;
	}
    }

            retval = (IPTR) obj;

}

/*static VOID bitmap_get(Class *cl, Object *o, struct P_Root_Get *msg)
{
    struct hGfx

            data = INST_DATA(cl, obj);
            switch(OPG(msg)->opg_AttrID)
            {
                case aHidd_BitMap_BitMap:
                    *(OPG(msg)->opg_Storage) = (ULONG) data->bitMap;
                    retval = TRUE;
                    break;

                default:
                    retval = DoSuperMethodA(cl, obj, msg);
            }
            break;

}
*/
/*************************** Classes *****************************/

#undef GfxHiddBase

struct IClass *InitGfxHiddBitMapClass (struct GfxHiddBase_intern * GfxHiddBase)
{
    Class *cl = NULL;

    D(bug("GfxHiddBitMapClass init\n"));

    cl = MakeClass(GRAPHICSHIDDBITMAP, ROOTCLASS, NULL, sizeof(struct GfxHiddBitMapData), 0);
    if (cl) {

        cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_gfxhiddbitmapclass);
        cl->cl_Dispatcher.h_SubEntry = NULL;
        cl->cl_UserData              = (IPTR)GfxHiddBase;

        AddClass (cl);
    }

    return (cl);
}
