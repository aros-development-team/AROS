/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Graphics bitmap class implementation.
    Lang: english
*/

#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/utility.h>
#include <proto/gfxhidd.h>

#include <exec/libraries.h>

#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/intuition.h>

#include <utility/tagitem.h>
#include <hidd/graphics.h>

#include "gfxhidd_intern.h"

#include <proto/intuition.h>

#define DEBUG 1
#include <aros/debug.h>

#undef GfxHiddBase
#define GfxHiddBase ((struct GfxHiddBase_intern *)(cl->cl_UserData))

/* macros for easier access to obj, msg */
#define BM(obj) ((struct hGfx_BitMap *)(obj))
#define OPS(x) ((struct opSet *)(x))
#define OPG(x) ((struct opGet *)(x))


AROS_UFH3(static IPTR, dispatch_gfxhiddbitmapclass,
          AROS_UFHA(Class *, cl, A0),
          AROS_UFHA(Object *, obj, A2),
          AROS_UFHA(Msg, msg, A1)
)
{
    IPTR   retval = 0UL;
    struct TagItem *tag;
    struct GfxHiddBitMapData *data;
    ULONG  width = 0, height = 0, depth = 0, flags = 0;

    switch(msg->MethodID)
    {
        case OM_NEW:
            D(bug("BM_BitMapClass - OM_NEW:\n"));

            obj  = (Object *) DoSuperMethodA(cl, obj, msg);
            data = INST_DATA(cl, obj);

            if(obj)
            {
                while((tag = NextTagItem(&OPS(msg)->ops_AttrList)))
                {
                    switch(tag->ti_Tag)
                    {
                        case HIDDA_BitMap_Width      : width    = tag->ti_Data; break;
                        case HIDDA_BitMap_Height     : height   = tag->ti_Data; break;
                        case HIDDA_BitMap_Depth      : depth    = tag->ti_Data; break;
                        case HIDDA_BitMap_Displayable: if(tag->ti_Data) flags = flags | HIDD_Graphics_BitMap_Flags_Displayable; break;

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
            break;


        case OM_DISPOSE:
            D(bug("BM_BitMapClass - OM_DISPOSE:\n"));

            data = INST_DATA(cl, obj);

            HIDD_Graphics_DeleteBitMap(data->bitMap, NULL); /* NULL Ptr is allowded */

            retval = DoSuperMethodA(cl, obj, msg);
            break;


        case OM_UPDATE:
        case OM_SET:
            D(bug("BM_BitMapClass - OM_UPDATE/OM_SET:\n"));
            D(bug("  sorry, not implemented yet\n"));

            retval = NULL;
            break;
    
    
        case OM_GET:
            D(bug("BM_BitMapClass - OM_GET:\n"));
            /*D(bug("  sorry, not implemented yet\n"));*/

            data = INST_DATA(cl, obj);
            switch(OPG(msg)->opg_AttrID)
            {
                case HIDDA_BitMap_BitMap:
                    *(OPG(msg)->opg_Storage) = (ULONG) data->bitMap;
                    retval = TRUE;
                    break;

                default:
                    retval = DoSuperMethodA(cl, obj, msg);
            }
            break;


        default:
            retval = DoSuperMethodA(cl, obj, msg);
            break;
    }

    return retval;
}

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
