/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Graphics hidd class implementation.
    Lang: english
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/alib.h>

#include <exec/libraries.h>

#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/intuition.h>

#include <utility/tagitem.h>
#include <hidd/graphics.h>

#include "gfxhidd_intern.h"

#include <proto/intuition.h>

#define DEBUG 0
#include <aros/debug.h>

#undef GfxHiddBase
#define GfxHiddBase ((struct GfxHiddBase_intern *)(cl->cl_UserData))


AROS_UFH3(static IPTR, dispatch_gfxhiddclass,
          AROS_UFHA(Class *, cl, A0),
          AROS_UFHA(Object *, obj, A2),
          AROS_UFHA(Msg, msg, A1)
)
{
    IPTR retval = 0UL;
    struct GfxHiddData *data;
    UWORD  cmdLen; /* length of a command */
    ULONG  *ulPtr; /* pointer to ULONG for "parsing" commands */
    UWORD  *uwPtr; /* pointer to UWORD for "parsing" commands */

    switch (msg->MethodID) {
    case OM_NEW:
        retval = DoSuperMethodA(cl, obj, msg);
        break;

    case OM_DISPOSE:
        data = INST_DATA(cl, obj);
        retval = DoSuperMethodA(cl, obj, msg);
        break;

    case OM_UPDATE:
    case OM_SET:
        break;

    case HIDDM_Graphics_Cmd:
        /* IPTR HIDDM_Graphics_Cmd,UWORD Length,UWORD Command,...) */
        D(bug("GfxHiddClass - HIDDM_Graphics_Cmd:\n"));

        ulPtr  = (ULONG *) msg;
        ulPtr  = ulPtr + 1; /* skip MessageID */
        cmdLen = *ulPtr++;

        switch(*ulPtr++)
        {
            case HIDDV_Graphics_Cmd_CreateBitMap:
                /* HIDDT_BitMap HIDDV_Graphics_Cmd_CreateBitMap (Tag tag, ...) */
                D(bug("HIDDV_Graphics_Cmd_CreateBitMap:\n"));

                /*
                   The paramters are tags, so only the pointer to
                   the first tag must passed.
                */
                retval = (IPTR) NewObjectA(NULL,
                                           GRAPHICSHIDDBITMAP,
                                           (struct TagItem *) ulPtr
                                          );
                break;

            case HIDDV_Graphics_Cmd_DeleteBitMap:
                /* void HIDDV_Graphics_Cmd_DeleteBitMap (HIDDT_BitMap bm) */
                D(bug("HIDDV_Graphics_Cmd_DeleteBitMap:\n"));

                /*
                   The first parameter is the objectpointer just dispose
                   the object.
                */
                DisposeObject((struct Object *) ulPtr);
                retval = 0;
                break;

            default:
                D(bug("Error: unknown command\n"));
                break;
        }
        break;

    case HIDDM_SpeedTest:
        data = INST_DATA(cl, obj);
        D(bug("Speedtest method:\n"));
        D(bug("Val1: %x\n", ((struct hGfx_SpeeTest *)msg)->val1));
        D(bug("Val2: %x\n", ((struct hGfx_SpeeTest *)msg)->val2));
        D(bug("Val3: %x\n", ((struct hGfx_SpeeTest *)msg)->val3));
        break;


#define OPG(x) ((struct opGet *)(x))
    case OM_GET:
        data = INST_DATA(cl, obj);
        break;


    default:
        retval = DoSuperMethodA(cl, obj, msg);
        break;
    }

    return retval;
}

/*************************** Classes *****************************/

#undef GfxHiddBase

struct IClass *InitGfxHiddClass (struct GfxHiddBase_intern * GfxHiddBase)
{
    Class *cl = NULL;

    D(bug("GfxHiddClass init3\n"));

    cl = MakeClass(GRAPHICSHIDD, ROOTCLASS, NULL, sizeof(struct GfxHiddData), 0);
    if (cl) {
    D(bug("GfxHiddClass ok\n"));

        cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_gfxhiddclass);
        cl->cl_Dispatcher.h_SubEntry = NULL;
        cl->cl_UserData              = (IPTR)GfxHiddBase;

        AddClass (cl);
    }

    D(bug("GfxHiddClass init - exit\n"));

    return (cl);
}
