/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

#include <string.h>
#include <stdio.h>
#include <math.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "prefs.h"
#include "debug.h"
#include "knob_private.h"

extern struct Library *MUIMasterBase;

#define OUTERFRAME_X 	    2
#define OUTERFRAME_Y 	    2
#define OUTERFRAME_W 	    (OUTERFRAME_X * 2)
#define OUTERFRAME_H 	    (OUTERFRAME_Y * 2)

#define INNERFRAME_X 	    2
#define INNERFRAME_Y 	    2
#define INNERFRAME_W 	    (INNERFRAME_X * 2)
#define INNERFRAME_H 	    (INNERFRAME_Y * 2)

#define BORDERSIZE_X 	    2
#define BORDERSIZE_Y 	    2
#define BORDERSIZE_W 	    (BORDERSIZE_X * 2)
#define BORDERSIZE_H 	    (BORDERSIZE_Y * 2)

#define KNOB_LABEL_SPACING  2

#define KNOB_WIDTH  	    25
#define KNOB_HEIGHT 	    25

#define LABEL_HEIGHT	    8

#define TOTAL_WIDTH 	    OUTERFRAME_W + BORDERSIZE_W + KNOB_WIDTH
#define TOTAL_HEIGHT	    OUTERFRAME_H + BORDERSIZE_H + INNERFRAME_H + KNOB_HEIGHT + KNOB_LABEL_SPACING + LABEL_HEIGHT


/* 0 halfshine */
/* 1 halfshadow */
/* 2 shadow */
/* 3 shine */

#define RLE_REP(count, val) (((count - 1) << 4) | val)

static const UBYTE knob_rle[] =  /* hand-encoded, BTW ;-) */
{
    RLE_REP(8,0), RLE_REP(9,1), RLE_REP(8,0),
    RLE_REP(6,0), RLE_REP(2,1), RLE_REP(9,2), RLE_REP(2,1), RLE_REP(6,0),
    RLE_REP(5,0), 1, RLE_REP(2,2), RLE_REP(9,3), RLE_REP(2,2), 1, RLE_REP(5,0),
    RLE_REP(4,0), 1, 2, RLE_REP(13,3), 2, 1, RLE_REP(4,0),
    RLE_REP(3,0), 1, 2, RLE_REP(3,3), RLE_REP(9,0), RLE_REP(3,3), 2, 3, RLE_REP(3,0),
    RLE_REP(2,0), 1, 2, RLE_REP(2,3), RLE_REP(13,0), 3, 1, 2, 3, RLE_REP(2,0),
    0, 1, 2, RLE_REP(2,3), RLE_REP(15,0), RLE_REP(2,1), 2, 3, 0,
    0, 1, 2, RLE_REP(2,3), RLE_REP(15,0), RLE_REP(2,1), 2, 3, 0,
    1, 2, RLE_REP(2,3), 0, RLE_REP(16,0), RLE_REP(2,1), 2, 3,
    1, 2, RLE_REP(2,3), 0, RLE_REP(16,0), RLE_REP(2,1), 2, 3,
    1, 2, RLE_REP(2,3), 0, RLE_REP(16,0), RLE_REP(2,1), 2, 3,
    1, 2, RLE_REP(2,3), 0, RLE_REP(16,0), RLE_REP(2,1), 2, 3,
    1, 2, RLE_REP(2,3), 0, RLE_REP(16,0), RLE_REP(2,1), 2, 3,
    1, 2, RLE_REP(2,3), 0, RLE_REP(16,0), RLE_REP(2,1), 2, 3,
    1, 2, RLE_REP(2,3), 0, RLE_REP(16,0), RLE_REP(2,1), 2, 3,
    1, 2, RLE_REP(2,3), 0, RLE_REP(16,0), RLE_REP(2,1), 2, 3,
    1, 2, RLE_REP(2,3), 0, RLE_REP(16,0), RLE_REP(2,1), 2, 3,
    0, 1, 2, RLE_REP(2,3), RLE_REP(15,0), RLE_REP(2,1), 2, 3, 0,
    0, 1, 2, RLE_REP(2,3), RLE_REP(15,0), RLE_REP(2,1), 2, 3, 0,
    RLE_REP(2,0), 1, 2, RLE_REP(2,3), RLE_REP(13,0), RLE_REP(2,1), 2, 3, RLE_REP(2,0),    
    RLE_REP(3,0), 1, 2, RLE_REP(3,1), RLE_REP(9,0), RLE_REP(3,1), 2, 3, RLE_REP(3,0),
    RLE_REP(4,0), 3, 2, RLE_REP(13,1), 2, 3, RLE_REP(4,0),
    RLE_REP(5,0), 3, RLE_REP(2,2), RLE_REP(9,1), RLE_REP(2,2), 3, RLE_REP(5,0),    
    RLE_REP(6,0), RLE_REP(2,3), RLE_REP(9,2), RLE_REP(2,3), RLE_REP(6,0),
    RLE_REP(8,0), RLE_REP(9,3), RLE_REP(8,0),
    
};

IPTR Knob__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    obj = (Object *)DoSuperNewTags
    (
        cl, obj, NULL,
	MUIA_FillArea, FALSE,
        TAG_MORE, (IPTR) msg->ops_AttrList
    );
    
    if (obj)
    {
    }
    
    return (IPTR)obj;
}

IPTR Knob__MUIM_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    //struct RastPort 	       rp;
    IPTR    	    	       retval;
    
    retval = DoSuperMethodA(cl, obj, (Msg)msg);
    if (retval)
    {
    #if 0
	InitRastPort(&rp);
	SetFont(&rp,_font(obj));

    	DeinitRastPort(&rp);
    #endif
    }
    
    return retval;
}

IPTR Knob__MUIM_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    //struct Knob_DATA *data = INST_DATA(cl, obj);
    
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
IPTR Knob__MUIM_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    //struct Knob_DATA *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg)msg);

    msg->MinMaxInfo->MinWidth  += TOTAL_WIDTH;
    msg->MinMaxInfo->MinHeight += TOTAL_HEIGHT;
    msg->MinMaxInfo->DefWidth  += TOTAL_WIDTH;
    msg->MinMaxInfo->DefHeight += TOTAL_HEIGHT;
    msg->MinMaxInfo->MaxWidth  += TOTAL_WIDTH;
    msg->MinMaxInfo->MaxHeight += TOTAL_HEIGHT;

    return TRUE;
}

static void DrawNeedle(Object *obj, struct RastPort *rp, LONG x1, LONG y1,
    	    	       LONG x2, LONG y2, double angle, BOOL clear)
{
    LONG cx = (x1 + x2) / 2;
    LONG cy = (y1 + y2) / 2;
    LONG rx = cx - x1 - 4;
    LONG ry = cy - y1 - 4;
    LONG a, b;
    
    SetDrMd(rp, JAM1);
   
    if ((angle < 0.0) | (angle > 270.0)) angle = 0.0;
    angle = 270.0 - 45.0 - angle;
    
    a = cx + (LONG)(cos(angle * 3.14159265358979323846 / 180.0) * rx);
    b = cy - (LONG)(sin(angle * 3.14159265358979323846 / 180.0) * ry);

    if (clear)
    {
    	SetAPen(rp, _pens(obj)[MPEN_HALFSHINE]);
	RectFill(rp, a - 1, b - 1, a + 1, b + 1);
    }
    else
    {
    	SetAPen(rp, _pens(obj)[MPEN_SHADOW]);
	Move(rp, a, b - 1); Draw(rp, a - 1, b);
    	SetAPen(rp, _pens(obj)[MPEN_HALFSHADOW]);
	Move(rp, a - 1, b - 1); Draw(rp, a, b);
    	SetAPen(rp, _pens(obj)[MPEN_SHINE]);
	Move(rp, a + 1, b); Draw(rp, a, b + 1);
    	SetAPen(rp, _pens(obj)[MPEN_HALFSHINE]);
	WritePixel(rp, a + 1, b - 1);
	WritePixel(rp, a + 1, b + 1);
	WritePixel(rp, a - 1, b + 1);
    }
   
}


/**************************************************************************
 MUIM_Draw
**************************************************************************/
IPTR Knob__MUIM_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct Knob_DATA *data = INST_DATA(cl, obj);
    struct RastPort *rp;
    WORD x1, y1, x2, y2;

    DoSuperMethodA(cl,obj,(Msg)msg);

    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)))
        return FALSE;

    x1 = _mleft(obj);
    y1 = _mtop(obj);
    x2 = _mright(obj);
    y2 = _mbottom(obj);
    
    rp = _rp(obj);
    
    if (msg->flags & MADF_DRAWOBJECT)
    {
    	/* Transparent edges */
	
	DoMethod(obj, MUIM_DrawParentBackground, x1, y1, 2, 1, x1, y1, 0);
	DoMethod(obj, MUIM_DrawParentBackground, x1, y1 + 1, 1, 1, x1, y1 + 1, 0);
	
	DoMethod(obj, MUIM_DrawParentBackground, x2 - 1, y1, 2, 1, x2 - 1, y1, 0);
    	DoMethod(obj, MUIM_DrawParentBackground, x2, y1 + 1, 1, 1, x2, y1 + 1, 0);

	DoMethod(obj, MUIM_DrawParentBackground, x1, y2, 2, 1, x1, y2, 0);
	DoMethod(obj, MUIM_DrawParentBackground, x1, y2 - 1, 1, 1, x1, y2 - 1, 0);
	
	DoMethod(obj, MUIM_DrawParentBackground, x2 - 1, y2, 2, 1, x2 - 1, y2, 0);
	DoMethod(obj, MUIM_DrawParentBackground, x2, y2 - 1, 1, 1, x2, y2 - 1, 0);
	
    	/* Outer frame */
	
	SetABPenDrMd(rp, _pens(obj)[MPEN_SHINE], 0, JAM1);
	Move(rp, x1 + 1, y2 - 1);
	Draw(rp, x1, y2 - 2);
	Draw(rp, x1, y1 + 2);
	Draw(rp, x1 + 2, y1);
	Draw(rp, x2 - 2, y1);
	Draw(rp, x2 - 1, y1 + 1);
	
	SetAPen(rp, _pens(obj)[MPEN_SHADOW]);
	Move(rp, x2, y1 + 2);
	Draw(rp, x2, y2 - 2);
	Draw(rp, x2 - 2, y2);
	Draw(rp, x1 + 2, y2);
	
	SetAPen(rp, _pens(obj)[MPEN_HALFSHINE]);
	Move(rp, x1 + 1, y2 - 2);
	Draw(rp, x1 + 1, y1 + 2);
	Draw(rp, x1 + 2, y1 + 1);
	Draw(rp, x2 - 2, y1 + 1);
	
	SetAPen(rp, _pens(obj)[MPEN_HALFSHADOW]);
	Move(rp, x2 - 1, y1 + 2);
	Draw(rp, x2 - 1, y2 - 2);
	Draw(rp, x2 - 2, y2 - 1);
	Draw(rp, x1 + 2, y2 - 1);
	
	/* Border */
	
	x1 += OUTERFRAME_X; x2 -= OUTERFRAME_X;
	y1 += OUTERFRAME_X; y2 -= OUTERFRAME_Y;
	
	SetAPen(rp, _pens(obj)[MPEN_HALFSHINE]);
	RectFill(rp, x1, y1, x2, y1 + BORDERSIZE_Y - 1);
	RectFill(rp, x1, y1 + BORDERSIZE_Y, x1 + BORDERSIZE_X - 1, y2);
	RectFill(rp, x1 + BORDERSIZE_X - 1, y2 - BORDERSIZE_Y + 1, x2, y2);
	RectFill(rp, x2 - BORDERSIZE_X + 1, y1 + BORDERSIZE_Y, x2, y2 - BORDERSIZE_Y);
	
	/* Inner Frame */
	
	x1 += BORDERSIZE_X; x2 -= BORDERSIZE_X;
	y1 += BORDERSIZE_Y; y2 = y1 + KNOB_HEIGHT -1;
	
	/* Knob bg */
	
    	{
	    static const UBYTE pen_mapping[] =
	    {
	    	MPEN_HALFSHINE, MPEN_HALFSHADOW, MPEN_SHADOW, MPEN_SHINE
	    };
	    const UBYTE *rleptr;
	    UBYTE rle;
	    WORD x = 0, y = 0, count;
	    
	    for(rleptr = knob_rle; ;)
	    {
	    	rle = *rleptr++;
		count = (rle >> 4) + 1;
		SetAPen(_rp(obj), _pens(obj)[pen_mapping[rle & 15]]);		
		RectFill(_rp(obj), x1 + x, y1 + y, x1 + x + count - 1, y1 + y);
		x += count;
		if (x >= KNOB_WIDTH)
		{
		    x = 0;
		    y++;
		    if (y >= KNOB_HEIGHT) break;
		}
	    }
	}
	
	/* Knob-Label spacing */
	
	y1 = y2 + 1;
	
	SetAPen(rp, _pens(obj)[MPEN_HALFSHINE]);
	RectFill(rp, x1, y1, x2, y1 + KNOB_LABEL_SPACING - 1);
	
	/* Label Frame */

    	y1 += KNOB_LABEL_SPACING;
	y2 = _mbottom(obj) - OUTERFRAME_Y - BORDERSIZE_Y;

	SetAPen(rp, _pens(obj)[MPEN_HALFSHINE]);
	Move(rp, x1, y1); Draw(rp, x1 + 1, y1); Draw(rp, x1, y1 + 1);
	Move(rp, x2, y1); Draw(rp, x2 - 1, y1); Draw(rp, x2, y1 + 1);
	Move(rp, x1, y2); Draw(rp, x1 + 1, y2); Draw(rp, x1, y2 - 1);
	Move(rp, x2, y2); Draw(rp, x2 - 1, y2); Draw(rp, x2, y2 - 1);
	
	SetAPen(rp, _pens(obj)[MPEN_HALFSHADOW]);
	Move(rp, x1 + 1, y2 - 1);
	Draw(rp, x1, y2 - 2);
    	Draw(rp, x1, y1 + 2);
	Draw(rp, x1 + 2, y1);
	Draw(rp, x2 - 2, y1);
	Draw(rp, x2 - 1, y1 + 1);
	
	SetAPen(rp, _pens(obj)[MPEN_SHINE]);
	Move(rp, x2, y1 + 2);
	Draw(rp, x2, y2 - 2);
	Draw(rp, x2 - 2, y2);
	Draw(rp, x1 + 2, y2);
	
	SetAPen(rp, _pens(obj)[MPEN_SHADOW]);
	RectFill(rp, x1 + 1, y1 + 2, x1 + 1, y2 - 2);
	RectFill(rp, x2 - 1, y1 + 2, x2 - 1, y2 - 2);
	RectFill(rp, x1 + 2, y1 + 1, x2 - 2, y1 + 1);
	RectFill(rp, x1 + 2, y2 - 1, x2 - 2, y2 - 1);
	
	/* Label Bg */
	
	RectFill(rp, x1 + 2, y1 +2, x2 - 2, y2 - 2);
		
    }
    
    x1 = _mleft(obj) + OUTERFRAME_X + BORDERSIZE_X;
    x2 = _mright(obj) - OUTERFRAME_X - BORDERSIZE_X;
    y1 = _mtop(obj) + OUTERFRAME_Y + BORDERSIZE_Y;
    y2 = y1 + KNOB_HEIGHT - 1;
    
    if (msg->flags & MADF_DRAWUPDATE)
    {
    	DrawNeedle(obj, rp, x1, y1, x2, y2, data->prevangle, TRUE);
    }
    
    data->prevangle = (double)DoMethod(obj, MUIM_Numeric_ValueToScale, 0, 270);

    DrawNeedle(obj, rp, x1, y1, x2, y2, data->prevangle, FALSE);
    
    return TRUE;
}

#if ZUNE_BUILTIN_KNOB
BOOPSI_DISPATCHER(IPTR, Knob_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Knob__OM_NEW(cl, obj, (struct opSet *)msg);
	case MUIM_Setup: return Knob__MUIM_Setup(cl, obj, (struct MUIP_Setup *)msg);
	case MUIM_Cleanup: return Knob__MUIM_Cleanup(cl, obj, (struct MUIP_Cleanup *)msg);		
	case MUIM_AskMinMax: return Knob__MUIM_AskMinMax(cl, obj, (struct MUIP_AskMinMax *)msg);
	case MUIM_Draw: return Knob__MUIM_Draw(cl, obj, (struct MUIP_Draw *)msg);
        default:     return DoSuperMethodA(cl, obj, msg);
    }
}

const struct __MUIBuiltinClass _MUI_Knob_desc =
{ 
    MUIC_Knob, 
    MUIC_Numeric, 
    sizeof(struct Knob_DATA), 
    (void*)Knob_Dispatcher 
};
#endif /* ZUNE_BUILTIN_KNOB */
