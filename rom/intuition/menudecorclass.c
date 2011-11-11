/*
    Copyright  1995-2011, The AROS Development Team. All rights reserved.
    Copyright  2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/


#include <dos/dos.h>
#include <dos/dosextens.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/menudecorclass.h>
#include <intuition/cghooks.h>
#include <intuition/icclass.h>
#include <intuition/extensions.h>

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

#include <proto/alib.h>

#include "intuition_intern.h"
#include "gadgets.h"


/**************************************************************************************************/

#ifdef __AROS__
#define USE_AROS_DEFSIZE 1
#else
#define USE_AROS_DEFSIZE 0
#endif

#define DEFSIZE_WIDTH  14
#define DEFSIZE_HEIGHT 14

#define HSPACING 3
#define VSPACING 3
#define REFHEIGHT (msg->mdp_ReferenceFont->tf_YSize)
#define REFWIDTH  REFHEIGHT

/* Ralph Schmidt
 * heuristics for smaller arrows used in apps
 * like filer
 */
#define HSPACING_MIDDLE 2
#define VSPACING_MIDDLE 2
#define HSPACING_SMALL 1
#define VSPACING_SMALL 1

#define DRI(dri) ((struct DrawInfo *)(dri))

/**************************************************************************************************/

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>
/**************************************************************************************************/

void menu_draw_thick_line(Class *cl, struct RastPort *rport,
                     LONG x1, LONG y1, LONG x2, LONG y2,
                     UWORD thickness)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;

    Move(rport, x1, y1);
    Draw(rport, x2, y2);
    /* Georg Steger */
    Move(rport, x1 + 1, y1);
    Draw(rport, x2 + 1, y2);
}

IPTR MenuDecorClass__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct Library *UtilityBase = GetPrivIBase(IntuitionBase)->UtilityBase;
    struct scrdecor_data *data;
    
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (obj)
    {
    	data = INST_DATA(cl, obj);

	data->userbuffersize = (ULONG) GetTagData(MDA_UserBuffer, 0, msg->ops_AttrList);
	
    }
    
    return (IPTR)obj;
}

/**************************************************************************************************/

IPTR MenuDecorClass__OM_GET(Class *cl, Object *obj, struct opGet *msg)
{
    struct scrdecor_data *data = INST_DATA(cl, obj);

    switch(msg->opg_AttrID)
    {
    	case MDA_UserBuffer:
	    *msg->opg_Storage = (IPTR) data->userbuffersize;
	    break;

	case MDA_TrueColorOnly:
	    *msg->opg_Storage = FALSE;
	    break;
	    
	default:
	    return DoSuperMethodA(cl, obj, (Msg)msg);
    }
    
    return 1;    
}
    

/**************************************************************************************************/

IPTR MenuDecorClass__MDM_GETDEFSIZE_SYSIMAGE(Class *cl, Object *obj, struct mdpGetDefSizeSysImage *msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;

    switch(msg->mdp_Which)
    {
    	case SUBMENUIMAGE:
    	    *msg->mdp_Width = 0;
    	    *msg->mdp_Height = 0;
            struct  RastPort *rp = CreateRastPort();
            if (rp)
            {
                struct  TextExtent TextExt;
                SetFont(rp, msg->mdp_ReferenceFont);
                TextExtent(rp, ">>", 2, &TextExt);
                *msg->mdp_Width = TextExt.te_Width;
                *msg->mdp_Height = TextExt.te_Height;
                FreeRastPort(rp);
            }
	       break;

        case MENUCHECK:
            *msg->mdp_Width = REFWIDTH / 2 + 4; // reffont->tf_XSize * 3 / 2;
            *msg->mdp_Height= REFHEIGHT;
            break;

    	default:
	       *msg->mdp_Width = DEFSIZE_WIDTH;
	       *msg->mdp_Height = DEFSIZE_HEIGHT;
	       break;
    }

    return TRUE;
}

IPTR MenuDecorClass__MDM_DRAW_SYSIMAGE(Class *cl, Object *obj, struct mdpDrawSysImage *msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct GfxBase       *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct RastPort 	 *rport = msg->mdp_RPort;
    UWORD   	    	 *pens = DRI(msg->mdp_Dri)->dri_Pens;
    LONG    	     	  left = msg->mdp_X;
    LONG    	     	  top = msg->mdp_Y;
    LONG   	          width = msg->mdp_Width;
    LONG   	    	  height = msg->mdp_Height;
    LONG    	    	  right = left + width - 1;
    LONG    	    	  bottom = top + height - 1;
    
    SetDrMd(rport, JAM1);
    
    switch(msg->mdp_Which)
    {
    	case SUBMENUIMAGE:
        {
            if (MENUS_AMIGALOOK(IntuitionBase))
            {
                SetAPen(rport, pens[BARBLOCKPEN]);
            }
            else
            {
                SetAPen(rport, pens[(msg->mdp_State == IDS_SELECTED) ? FILLPEN : BACKGROUNDPEN]);
            }
            RectFill(rport, left, top, right, bottom);
            SetAPen(rport, pens[BARDETAILPEN]);
            SetDrMd(rport, JAM1);
            WORD x = left;

    	    Move(rport, x, top + rport->Font->tf_Baseline);
            Text(rport, ">>", 2);
            break;
        }

    	case MENUCHECK:
        {
            if (MENUS_AMIGALOOK(IntuitionBase))
            {
                SetAPen(rport, pens[BARBLOCKPEN]);
            }
            else
            {
                SetAPen(rport, pens[(msg->mdp_State == IDS_SELECTED) ? FILLPEN : BACKGROUNDPEN]);
            }

            RectFill(rport, left, top, right, bottom);

            SetAPen(rport, pens[BARDETAILPEN]);
            menu_draw_thick_line(cl, rport, left + 1, top + height / 3 , left + 1, bottom, 0);
            menu_draw_thick_line(cl, rport, left + 2, bottom, right - 2, top, 0);

            break;
        }

	case AMIGAKEY:
	{
            struct TextFont *oldfont;
            UBYTE   	     oldstyle;
            
            if (MENUS_AMIGALOOK(IntuitionBase))
            {
                SetAPen(rport, pens[BARDETAILPEN]);
            }
            else
            {
                SetAPen(rport, pens[SHINEPEN]);
            }

            RectFill(rport, left, top, right, bottom);

            if (MENUS_AMIGALOOK(IntuitionBase))
            {
                SetAPen(rport, pens[BARBLOCKPEN]);
    
                oldfont = rport->Font;
                oldstyle = rport->AlgoStyle;
    
                SetFont(rport, GfxBase->DefaultFont);
                SetSoftStyle(rport, FSF_ITALIC, AskSoftStyle(rport));
    
                Move(rport, left + (width - rport->TxWidth) / 2,
                     top  + (height - rport->TxHeight) / 2 + rport->TxBaseline);
                Text(rport, "A", 1);
    
                SetSoftStyle(rport, oldstyle, AskSoftStyle(rport));
                SetFont(rport, oldfont);
    
                SetAPen(rport, pens[BARBLOCKPEN]);
            }
            else
            {
                SetAPen(rport, pens[SHADOWPEN]);
    
                RectFill(rport, left + 1, top, right - 1, top);
                RectFill(rport, right, top + 1, right, bottom - 1);
                RectFill(rport, left + 1, bottom, right - 1, bottom);
                RectFill(rport, left, top + 1, left, bottom - 1);
    
                SetAPen(rport, pens[BACKGROUNDPEN]);
                RectFill(rport, left + 1, bottom - 1, right - 1, bottom - 1);
                RectFill(rport, right - 1, top + 1, right - 1, bottom - 2);
    
                RectFill(rport, left + 2, top + 2, left + 4, top + 2);
                RectFill(rport, left + 2, top + 3, left + 2, top + 4);
    
                SetAPen(rport, pens[SHADOWPEN]);
                RectFill(rport, left + 2, bottom - 2, right - 2, bottom - 2);
                RectFill(rport, right - 2, top + 2, right - 2, bottom - 4);
    
                {
                    WORD a_size   = height - 7;
                    WORD a_left   = left + 5;
                    WORD a_top    = top + 2;
                    WORD a_right  = a_left + a_size;
                    WORD a_bottom = a_top + a_size;
    
                    Move(rport, a_left, a_bottom);
                    Draw(rport, a_right, a_top);
                    Draw(rport, a_right, a_bottom);
                    Move(rport, a_right - 1, a_top + 1);
                    Draw(rport, a_right - 1, a_bottom);
                }
    
                SetAPen(rport, pens[(msg->mdp_State == IDS_SELECTED) ? FILLPEN : BACKGROUNDPEN]);
            }
            
            WritePixel(rport, left, top);
            WritePixel(rport, right, top);
            WritePixel(rport, right, bottom);
            WritePixel(rport, left, bottom);
	}
    }
    return TRUE;
}

IPTR MenuDecorClass__MDM_GETMENUSPACES(Class *cl, Object *obj, struct mdpGetMenuSpaces *msg)
{
    return FALSE;
}

/**************************************************************************************************/

IPTR MenuDecorClass__MDM_DRAWBACKGROUND(Class *cl, Object *obj, struct mdpDrawBackground *msg)
{
    return FALSE;
}

IPTR MenuDecorClass__MDM_INITMENU(Class *cl, Object *obj, struct mdpInitMenu *msg)
{
    return FALSE;
}

IPTR MenuDecorClass__MDM_EXITMENU(Class *cl, Object *obj, struct mdpExitMenu *msg)
{
    return FALSE;
}

/**************************************************************************************************/
