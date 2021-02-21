/*
    Copyright © 2016, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DEBUG
#   define DEBUG 0
#endif
#include <aros/debug.h>

#define USE_BOOPSI_STUBS
#include <exec/libraries.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <proto/graphics.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <devices/inputevent.h>
#include <proto/alib.h>

#include <gadgets/tapedeck.h>

#include "tapedeck_intern.h"

/***********************************************************************************/

#define IM(o) ((struct Image *)(o))
#define EG(o) ((struct Gadget *)(o))

#define POSPROP_HEIGHT   5
#define TDGADGET_HEIGHT  20

#include <clib/boopsistubs.h>

#define TAPEDECK_FLAG_INIT      (TDECK_Dummy + 100-1)

static UBYTE templateRewind[] =
{
    0x03, 0x03,         // 0000001100000011
    0x0F, 0x0f,         // 0000111100001111
    0x3F, 0x3F,         // 0011111100011111
    0xFF, 0xFF,         // 1111111111111111
    0x3F, 0x3F,         // 0011111100111111
    0x0F, 0x0F,         // 0000111100001111
    0x03, 0x03,         // 0000001100000011
};
static UBYTE templatePlay[] =
{
    0xC0,               // 11000000
    0xF0,               // 11110000
    0xFC,               // 11111100
    0xFF,               // 11111111
    0xFC,               // 11111100
    0xF0,               // 11110000
    0xC0                // 11000000
};
static UBYTE templatePause[] =
{
    0xE7,               // 11100111
    0xE7,               // 11100111
    0xE7,               // 11100111
    0xE7,               // 11100111
    0xE7,               // 11100111
    0xE7,               // 11100111
    0xE7                // 11100111
};
static UBYTE templateFForward[] =
{
    0xC0, 0xC0,         // 1100000011000000
    0xF0, 0xF0,         // 1111000011110000
    0xFC, 0xFC,         // 1111110011111100
    0xFF, 0xFF,         // 1111111111111111
    0xFC, 0xFC,         // 1111110011111100
    0xF0, 0xF0,         // 1111000011110000
    0xC0, 0xC0,         // 1100000011000000
};

/***********************************************************************************/

IPTR TapeDeck__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct TapeDeckData *data = INST_DATA(cl, o);
    IPTR    	     retval = 1;

    D(bug("[tapedeck.gadget]: %s()\n", __PRETTY_FUNCTION__));

    switch(msg->opg_AttrID)
    {
        case TDECK_Mode:
            *msg->opg_Storage = (IPTR)data->tdd_Mode;
            break;

        case TDECK_Frames:
            *msg->opg_Storage = (IPTR)data->tdd_FrameCount;
            break;

	case TDECK_CurrentFrame:
            *msg->opg_Storage = (IPTR)data->tdd_FrameCurrent;
            break;

        case GA_Height:
            *msg->opg_Storage = TDGADGET_HEIGHT;
            break;

	default:
	    retval = DoSuperMethodA(cl, o, (Msg)msg);
	    break;
    }
    
    return retval;
}

/***********************************************************************************/

IPTR TapeDeck__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    struct TapeDeckData *data = INST_DATA(cl, o);
    struct TagItem      *tag, *taglist = msg->ops_AttrList;
    BOOL                rerender = FALSE, tdinit = FALSE;
    IPTR                result;

    D(bug("[tapedeck.gadget]: %s()\n", __PRETTY_FUNCTION__));

    result = DoSuperMethodA(cl, o, (Msg)msg);

    while((tag = NextTagItem(&taglist)))
    {
        switch(tag->ti_Tag)
        {
            case TAPEDECK_FLAG_INIT:
                D(bug("[tapedeck.gadget] %s: performing initial setup\n", __PRETTY_FUNCTION__));
                tdinit = TRUE;
                break;

            case TDECK_Mode:
                D(bug("[tapedeck.gadget] %s: TDECK_Mode %08x\n", __PRETTY_FUNCTION__, tag->ti_Data));
                data->tdd_Mode = (ULONG)tag->ti_Data;
		break;

	    case TDECK_Frames:
                D(bug("[tapedeck.gadget] %s: TDECK_Frames - %d\n", __PRETTY_FUNCTION__, tag->ti_Data));
                data->tdd_FrameCount = (ULONG)tag->ti_Data;
		break;

            case TDECK_CurrentFrame:
                D(bug("[tapedeck.gadget] %s: TDECK_CurrentFrame - %d\n", __PRETTY_FUNCTION__, tag->ti_Data));
                data->tdd_FrameCurrent = (ULONG)tag->ti_Data;
		break;

	    case GA_Disabled:
		rerender = TRUE;
		break;

            case GA_Height:
                D(bug("[tapedeck.gadget] %s: GA_Height - %d (ignored)\n", __PRETTY_FUNCTION__, tag->ti_Data));
                tag->ti_Data = TDGADGET_HEIGHT;
                break;

            D(
            case GA_Left:
                bug("[tapedeck.gadget] %s: GA_Left - %d\n", __PRETTY_FUNCTION__, tag->ti_Data);
                break;

            case GA_Top:
                bug("[tapedeck.gadget] %s: GA_Top - %d\n", __PRETTY_FUNCTION__, tag->ti_Data);
                break;

            case GA_Width:
                bug("[tapedeck.gadget] %s: GA_Width - %d\n", __PRETTY_FUNCTION__, tag->ti_Data);
                break;


            )
        }
    }

    if (!(tdinit) && (rerender))
    {
        struct RastPort *rport;

        rport = ObtainGIRPort(msg->ops_GInfo);
        if(rport)
        {
            DoMethod(o, GM_RENDER, (IPTR)msg->ops_GInfo, (IPTR)rport, GREDRAW_REDRAW);
            ReleaseGIRPort(rport);
            result = FALSE;
        }
    }

    return result;
}

/***********************************************************************************/

Object *TapeDeck__OM_NEW(Class *cl, Class *rootcl, struct opSet *msg)
{
    struct TapeDeckData 	*data;
    struct TagItem pproptags[] =
    {
        { GA_ID,                1               },
        { PGA_Visible,          1               },
        { PGA_Freedom,          FREEHORIZ       },
        { PGA_Borderless,       TRUE            },
        { PGA_Total,            1               },
        { PGA_Top,              0               },
        { TAG_DONE,             0               }
    };
    struct TagItem  	frametags[] =
    {
        { IA_EdgesOnly,         FALSE           },
        { IA_FrameType,         FRAME_BUTTON    },
        { TAG_DONE,             0               }
    };
    struct TagItem  	tdinittags[] =
    {
        { TAPEDECK_FLAG_INIT,   0                       },
        { TAG_MORE,             (IPTR)msg->ops_AttrList       },
        { TAG_DONE,             0                       }
    };
    Object  	    	*o;

    D(bug("[tapedeck.gadget]: %s()\n", __PRETTY_FUNCTION__));

    o = (Object *)DoSuperMethodA(cl, (Object *)rootcl, (Msg)msg);
    if (o)
    {
        data = INST_DATA(cl, o);

        EG(o)->GadgetRender = NewObjectA(NULL, FRAMEICLASS, frametags);
        D(bug("[tapedeck.gadget] %s: frame @ 0x%p\n", __PRETTY_FUNCTION__, EG(o)->GadgetRender));

        data->tdd_ButtonPen[0] = SHADOWPEN;
        data->tdd_ButtonPen[1] = SHADOWPEN;
        data->tdd_ButtonPen[2] = SHADOWPEN;

        pproptags[3].ti_Data = data->tdd_FrameCount;
        pproptags[4].ti_Data = data->tdd_FrameCurrent;
        data->tdd_PosProp = NewObjectA(NULL, "propgclass", pproptags);

        D(bug("[tapedeck.gadget] %s: playback position prop @ 0x%p\n", __PRETTY_FUNCTION__, data->tdd_PosProp));

        if (msg->ops_AttrList)
        {
            msg->ops_AttrList = tdinittags;
            TapeDeck__OM_SET(cl, o, msg);
        }
    }

    return o;
}

/***********************************************************************************/

VOID TapeDeck__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct TapeDeckData *data = INST_DATA(cl, o);

    D(bug("[tapedeck.gadget]: %s()\n", __PRETTY_FUNCTION__));

    if (data->tdd_PosProp)
        DisposeObject((Object *)data->tdd_PosProp);

    if (EG(o)->GadgetRender)
        DisposeObject((Object *)EG(o)->GadgetRender);

    DoSuperMethodA(cl,o,msg);
}

/***********************************************************************************/

IPTR TapeDeck__GM_LAYOUT(Class *cl, struct Gadget *g, struct gpLayout *msg)
{
    struct TapeDeckData *data = INST_DATA(cl, g);
    struct TagItem  	frametags[] = {
        { GA_Left,      g->LeftEdge     },
        { GA_Top,       g->TopEdge      },
        { IA_Width,     g->Width        },
        { IA_Height,    g->Height	},
        { TAG_DONE,     0UL 		}
    };
    IPTR RetVal;

    D(bug("[tapedeck.gadget]: %s()\n", __PRETTY_FUNCTION__));

    RetVal = DoSuperMethodA(cl, (Object *)g, (Msg)msg);

    SetAttrsA((Object *)g->GadgetRender, frametags);
    DoMethodA((Object *)g->GadgetRender, msg);

    frametags[0].ti_Data = g->LeftEdge + 1;
    frametags[2].ti_Tag = GA_Width;
    frametags[2].ti_Data = g->Width - 2;
    frametags[3].ti_Tag = GA_Height;
    frametags[3].ti_Data = POSPROP_HEIGHT;

    SetAttrsA((Object *)data->tdd_PosProp, frametags);
    DoMethodA((Object *)data->tdd_PosProp, msg);

    return RetVal;
}

IPTR TapeDeck__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    struct TapeDeckData *data = INST_DATA(cl, o);
    struct TagItem  	proptags[] = {
        { PGA_Total,    data->tdd_FrameCount    },
        { PGA_Top,      data->tdd_FrameCurrent  },
        { TAG_DONE,     0UL 		        }
    };
    LONG rend_x, rend_y;
    IPTR propTop;

    D(bug("[tapedeck.gadget]: %s()\n", __PRETTY_FUNCTION__));

    if ((msg->gpr_Redraw != GREDRAW_UPDATE) && (EG(o)->GadgetRender))
    {
        /* Full redraw: clear and draw border */
        DrawImageState(msg->gpr_RPort, IM(EG(o)->GadgetRender),
                       EG(o)->LeftEdge, EG(o)->TopEdge,
                       IDS_NORMAL,
                       msg->gpr_GInfo->gi_DrInfo);
    }

    GetAttr (PGA_Top, (Object *)data->tdd_PosProp, &propTop);
    if (propTop != data->tdd_FrameCurrent)
    {
        SetAttrsA((Object *)data->tdd_PosProp, proptags);
        SetAPen(msg->gpr_RPort,  msg->gpr_GInfo->gi_DrInfo->dri_Pens[BACKGROUNDPEN]);
        RectFill(msg->gpr_RPort,
            EG(o)->LeftEdge + 2, EG(o)->TopEdge + 1,
            EG(o)->LeftEdge + EG(o)->Width - 2, EG(o)->TopEdge + POSPROP_HEIGHT);
    }
    DoMethodA((Object *)data->tdd_PosProp, (Msg)msg);

    rend_x = EG(o)->LeftEdge + ((EG(o)->Width - 50) >> 1);
    rend_y = EG(o)->TopEdge + 4 + ((EG(o)->Height - 11) >> 1);

    if ((msg->gpr_Redraw != GREDRAW_UPDATE) || (data->tdd_ButtonPen[0] != data->tdd_LastPen[0]))
    {
        SetAPen(msg->gpr_RPort,  msg->gpr_GInfo->gi_DrInfo->dri_Pens[data->tdd_ButtonPen[0]]);

        BltTemplate ((void *) templateRewind,
                 0, 2,
                 msg->gpr_RPort, rend_x, rend_y,
                 16, 7);

        data->tdd_LastPen[0] = data->tdd_ButtonPen[0];
    }

    if ((msg->gpr_Redraw != GREDRAW_UPDATE) || (data->tdd_ButtonPen[1] != data->tdd_LastPen[1]))
    {
        if (data->tdd_Mode != data->tdd_ModeLast)
        {
            SetAPen(msg->gpr_RPort,  msg->gpr_GInfo->gi_DrInfo->dri_Pens[BACKGROUNDPEN]);
            RectFill(msg->gpr_RPort,
                rend_x + 21, rend_y,
                rend_x + 28, rend_y + 6);
        }

        SetAPen(msg->gpr_RPort, msg->gpr_GInfo->gi_DrInfo->dri_Pens[data->tdd_ButtonPen[1]]);

        if (data->tdd_Mode == BUT_PLAY)
        {
            BltTemplate ((void *) templatePause,
                     0, 1,
                     msg->gpr_RPort, rend_x + 21, rend_y,
                     8, 7);
        }
        else
        {
            BltTemplate ((void *) templatePlay,
                     0, 1,
                     msg->gpr_RPort, rend_x + 21, rend_y,
                     8, 7);
        }

        data->tdd_LastPen[1] = data->tdd_ButtonPen[1];
    }

    if ((msg->gpr_Redraw != GREDRAW_UPDATE) || (data->tdd_ButtonPen[2] != data->tdd_LastPen[2]))
    {
        SetAPen(msg->gpr_RPort, msg->gpr_GInfo->gi_DrInfo->dri_Pens[data->tdd_ButtonPen[2]]);

        BltTemplate ((void *) templateFForward,
                 0, 2,
                 msg->gpr_RPort, rend_x + 34, rend_y,
                 16, 7);

        data->tdd_LastPen[2] = data->tdd_ButtonPen[2];
    }

    return (IPTR)TRUE;
}

/***********************************************************************************/

IPTR TapeDeck__GM_HANDLEINPUT(Class *cl, Object *o, struct gpInput *msg)
{
    struct RastPort     *rport;
    struct TapeDeckData *data = INST_DATA(cl, o);
    struct InputEvent   *ie = msg->gpi_IEvent;
    IPTR    	    	retval = GMR_MEACTIVE;
    BOOL                redraw = FALSE;

    D(bug("[tapedeck.gadget]: %s()\n", __PRETTY_FUNCTION__));

    if (ie->ie_Code == SELECTDOWN)
    {
        D(bug("[tapedeck.gadget]: %s: SELECTDOWN\n", __PRETTY_FUNCTION__));
        EG(o)->Flags |= GFLG_SELECTED;
    }

    if ((data->tdd_PosProp) &&
        ((((struct Gadget *)data->tdd_PosProp)->Flags & GFLG_SELECTED) || (msg->gpi_Mouse.Y < POSPROP_HEIGHT)))
    {
        IPTR topCurrent;

        D(bug("[tapedeck.gadget]: %s: input event is for the prop gadget ...\n", __PRETTY_FUNCTION__));

        if (ie->ie_Code == SELECTDOWN)
        {
            GetAttr (PGA_Top, (Object *)data->tdd_PosProp, &data->tdd_TopLast);
            data->tdd_ModeLast = data->tdd_Mode;
            data->tdd_Mode = BUT_FRAME;
        }

        /* pass it to the prop gadget .. */
        DoMethodA((Object *)data->tdd_PosProp, (Msg)msg);

        if (data->tdd_Mode == BUT_FRAME)
        {
            GetAttr (PGA_Top, (Object *)data->tdd_PosProp, &topCurrent);
            if (data->tdd_TopLast != topCurrent)
            {
                D(bug("[tapedeck.gadget]: %s: position moved ...(%d)\n", __PRETTY_FUNCTION__, topCurrent));
                data->tdd_FrameCurrent = topCurrent;
            }
            if (ie->ie_Code == SELECTUP)
                data->tdd_Mode = data->tdd_ModeLast;
        }
    }

    if ((msg->gpi_Mouse.Y > ((EG(o)->Height - 11) >> 1)) &&
            (msg->gpi_Mouse.Y < (EG(o)->Height - ((EG(o)->Height - 11) >> 1))) &&
            (msg->gpi_Mouse.X > ((EG(o)->Width - 50) >> 1)))
    {
        ULONG offset_x = msg->gpi_Mouse.X - ((EG(o)->Width - 50) >> 1);

        if (offset_x <  16)
        {
            D(bug("[tapedeck.gadget]: %s:    <<\n", __PRETTY_FUNCTION__));
            if (ie->ie_Code == SELECTDOWN)
            {
                data->tdd_ButtonActive = 1;
                data->tdd_ButtonPen[0] = SHINEPEN;
                data->tdd_ModeLast = data->tdd_Mode;
                data->tdd_Mode = BUT_REWIND;
                redraw = TRUE;
            }
            else if (ie->ie_Code == SELECTUP)
            {
                data->tdd_ButtonPen[0] = SHADOWPEN;
                data->tdd_Mode = data->tdd_ModeLast;
            }
        }
        else if ((offset_x > 20) && (offset_x < 29))
        {
            D(bug("[tapedeck.gadget]: %s:    PLAY/PAUSE\n", __PRETTY_FUNCTION__));
            if (ie->ie_Code == SELECTDOWN)
            {
                data->tdd_ButtonActive = 2;
                data->tdd_ButtonPen[1] = SHINEPEN;
                redraw = TRUE;
            }
            else if (ie->ie_Code == SELECTUP)
            {
                data->tdd_ButtonPen[1] = SHADOWPEN;
                data->tdd_ModeLast = data->tdd_Mode;
                if (data->tdd_Mode != BUT_PLAY)
                    data->tdd_Mode = BUT_PLAY;
                else
                    data->tdd_Mode = BUT_PAUSE;
            }
        }
        else if ((offset_x > 33) && (offset_x < 50))
        {
            D(bug("[tapedeck.gadget]: %s:    >>\n", __PRETTY_FUNCTION__));
            if (ie->ie_Code == SELECTDOWN)
            {
                data->tdd_ButtonActive = 3;
                data->tdd_ButtonPen[2] = SHINEPEN;
                data->tdd_ModeLast = data->tdd_Mode;
                data->tdd_Mode = BUT_FORWARD;
                redraw = TRUE;
            }
            else if (ie->ie_Code == SELECTUP)
            {
                data->tdd_ButtonPen[2] = SHADOWPEN;
                data->tdd_Mode = data->tdd_ModeLast;
            }
        }
    }

    if (ie->ie_Code == SELECTUP)
    {
        D(bug("[tapedeck.gadget]: %s: SELECTUP\n", __PRETTY_FUNCTION__));
        if (data->tdd_ButtonActive > 0)
        {
            data->tdd_ButtonPen[data->tdd_ButtonActive - 1] = SHADOWPEN;
            redraw = TRUE;
        }
        EG(o)->Flags &= ~GFLG_SELECTED;
        data->tdd_ButtonActive = 0;
        retval = GMR_NOREUSE;
    }

    if ((redraw) &&
        (rport = ObtainGIRPort (msg->gpi_GInfo)))
    {
        struct gpRender gpr;

        gpr.MethodID   = GM_RENDER;
        gpr.gpr_GInfo  = msg->gpi_GInfo;
        gpr.gpr_RPort  = rport;
        gpr.gpr_Redraw = GREDRAW_UPDATE;
        DoMethodA (o, &gpr);

        ReleaseGIRPort (rport);
    }

    return retval;
}

/***********************************************************************************/

IPTR TapeDeck__GM_GOINACTIVE(Class *cl, Object *o, struct gpGoInactive *msg)
{
    struct RastPort *rport;

    D(bug("[tapedeck.gadget]: %s()\n", __PRETTY_FUNCTION__));

    EG(o)->Flags &= ~GFLG_SELECTED;

    rport = ObtainGIRPort(msg->gpgi_GInfo);
    if (rport)
    {
        struct gpRender rmsg = { GM_RENDER, msg->gpgi_GInfo, rport, GREDRAW_UPDATE }, *p_rmsg = &rmsg;
	
        DoMethodA(o, (Msg)p_rmsg);
        ReleaseGIRPort(rport);
    }
    
    return 0;
}
