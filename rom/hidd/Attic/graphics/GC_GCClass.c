/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Graphics hidd graphics context class implementation.
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
#define OPS(x) ((struct opSet *)(x))
#define OPG(x) ((struct opGet *)(x))
#define PXD(x) ((struct hGfx_PixelDirect *)(x))
#define PT(x)  ((struct hGfx_Point *)(x))

AROS_UFH3(static IPTR, dispatch_gfxhiddgcclass,
          AROS_UFHA(Class *, cl, A0),
          AROS_UFHA(Object *, obj, A2),
          AROS_UFHA(Msg, msg, A1)
)
{
    IPTR   retval = 0UL;
    struct TagItem *tag;
    struct GfxHiddGCData *data;
    APTR   bm;

    switch(msg->MethodID)
    {
        case OM_NEW:
            D(bug("GC_GCClass - OM_NEW:\n"));
            /* D(bug("  sorry, not implemented yet\n")); */

            obj  = (Object *) DoSuperMethodA(cl, obj, msg);
            data = INST_DATA(cl, obj);

            bm = (APTR) GetTagData(HIDDA_GC_BitMap, NULL, OPS(msg)->ops_AttrList);
            data->gc = HIDD_Graphics_CreateGC(bm, NULL);
            if(data->gc)
            {
                while((tag = NextTagItem(&OPS(msg)->ops_AttrList)))
                {
                    switch(tag->ti_Tag)
                    {
                        case HIDDA_GC_BitMap     : /* was set with HIDD_Graphics_CreateGC() */ break;
                        case HIDDA_GC_Foreground : data->gc->fg        = tag->ti_Data; break;
                        case HIDDA_GC_Background : data->gc->bg        = tag->ti_Data; break;
                        case HIDDA_GC_DrawMode   : data->gc->drMode    = tag->ti_Data; break;
                        case HIDDA_GC_Font       : data->gc->font      = (APTR) tag->ti_Data; break;
                        case HIDDA_GC_ColorMask  : data->gc->colMask   = tag->ti_Data; break;
                        case HIDDA_GC_LinePattern: data->gc->linePat   = (UWORD) tag->ti_Data; break;
                        case HIDDA_GC_PlaneMask  : data->gc->planeMask = (APTR) tag->ti_Data; break;
                        case HIDDA_GC_UserData   : data->gc->userData  = (APTR) tag->ti_Data; break;

                        default: D(bug("  unknown attribute %li\n", tag->ti_Data)); break;
                    } /* switch tag */
                } /* while tag  */
            }
            else
            {
                DisposeObject(obj);
                obj = NULL;
            }

            retval = (IPTR) obj;
            break;

        case OM_DISPOSE:
            D(bug("GC_GCClass - OM_DISPOSE:\n"));
            /* D(bug("  sorry, not implemented yet\n")); */

            data = INST_DATA(cl, obj);
            HIDD_Graphics_DeleteGC(data->gc, NULL); /* NULL Ptr is allowded */

            retval = DoSuperMethodA(cl, obj, msg);
            break;

        case OM_UPDATE:
        case OM_SET:
            D(bug("GC_GCClass - OM_UPDATE/OM_SET:\n"));
            D(bug("  sorry, not implemented yet\n"));

            retval = NULL;
            break;
    
        case OM_GET:
            D(bug("GC_GCClass - OM_GET:\n"));
            D(bug("  sorry, not implemented yet\n"));

            data = INST_DATA(cl, obj);

            retval = NULL;
            break;

        case HIDDM_Graphics_GC_WritePixelDirect_Q:
            D(bug("GC_GCClass - HIDDM_Graphics_GC_WritePixelDirect_Q:\n"));

            data = INST_DATA(cl, obj);
            HIDD_Graphics_WritePixelDirect_Q(data->gc,
                                             PXD(msg)->x,
                                             PXD(msg)->y,
                                             PXD(msg)->val
                                            );
            retval = 0;
            break;

        case HIDDM_Graphics_GC_WritePixelDirect:
            D(bug("GC_GCClass - HIDDM_Graphics_GC_WritePixelDirect:\n"));

            data = INST_DATA(cl, obj);
            retval = HIDD_Graphics_WritePixelDirect(data->gc,
                                                    PXD(msg)->x,
                                                    PXD(msg)->y,
                                                    PXD(msg)->val
                                                   );
            break;

        case HIDDM_Graphics_GC_ReadPixel_Q:
            D(bug("GC_GCClass - HIDDM_Graphics_GC_ReadPixel_Q:\n"));

            data = INST_DATA(cl, obj);
            retval = HIDD_Graphics_ReadPixel_Q(data->gc,
                                               PT(msg)->x,
                                               PT(msg)->y
                                              );
            break;

        default:
            retval = DoSuperMethodA(cl, obj, msg);
            break;
    }

    return retval;
}

/*************************** Classes *****************************/

#undef GfxHiddBase

struct IClass *InitGfxHiddGCClass (struct GfxHiddBase_intern * GfxHiddBase)
{
    Class *cl = NULL;

    D(bug("GfxHiddBitMapClass init\n"));

    cl = MakeClass(GRAPHICSHIDDGC, ROOTCLASS, NULL, sizeof(struct GfxHiddGCData), 0);
    if (cl) {

        cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_gfxhiddgcclass);
        cl->cl_Dispatcher.h_SubEntry = NULL;
        cl->cl_UserData              = (IPTR)GfxHiddBase;

        AddClass (cl);
    }

    return (cl);
}
