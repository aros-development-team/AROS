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
#include "levelmeter_private.h"

extern struct Library *MUIMasterBase;

#define OUTERFRAME_X 	    2
#define OUTERFRAME_Y 	    2
#define OUTERFRAME_W 	    (OUTERFRAME_X * 2)
#define OUTERFRAME_H 	    (OUTERFRAME_Y * 2)

#define INNERFRAME_X 	    2
#define INNERFRAME_Y 	    2
#define INNERFRAME_W 	    (INNERFRAME_X * 2)
#define INNERFRAME_H 	    (INNERFRAME_Y * 2)

#define BORDERSIZE_X 	    3
#define BORDERSIZE_Y 	    2
#define BORDERSIZE_W 	    (BORDERSIZE_X * 2)
#define BORDERSIZE_H 	    (BORDERSIZE_Y * 2)

#define LEVEL_LABEL_SPACING 2

#define LEVEL_WIDTH 	    39
#define LEVEL_HEIGHT 	    20

#define LABEL_HEIGHT	    8

#define TOTAL_WIDTH 	    OUTERFRAME_W + BORDERSIZE_W + INNERFRAME_W + LEVEL_WIDTH
#define TOTAL_HEIGHT	    OUTERFRAME_H + BORDERSIZE_H + INNERFRAME_H * 2 + LEVEL_HEIGHT + LEVEL_LABEL_SPACING + LABEL_HEIGHT

IPTR Levelmeter__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    obj = (Object *)DoSuperNewTags
    (
        cl, obj, NULL,
	MUIA_FillArea, FALSE,
        TAG_MORE, (IPTR) msg->ops_AttrList
    );
    
    if (obj)
    {
    	struct Levelmeter_DATA *data = INST_DATA(cl, obj);
	
	data->levelbgpen = -1;
    }
    
    return (IPTR)obj;
}

IPTR Levelmeter__MUIM_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Levelmeter_DATA *data = INST_DATA(cl, obj);
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
    	data->levelbgpen = ObtainBestPen(_screen(obj)->ViewPort.ColorMap,
	    	    	    	    	 0x4b4b4b4b,
					 0x39393939,
					 0x93939393,
					 OBP_FailIfBad, FALSE,
					 OBP_Precision, PRECISION_GUI,
					 TAG_DONE);
    }
    
    return retval;
}

IPTR Levelmeter__MUIM_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Levelmeter_DATA *data = INST_DATA(cl, obj);
    
    if (data->levelbgpen != -1)
    {
    	ReleasePen(_screen(obj)->ViewPort.ColorMap, data->levelbgpen);
	data->levelbgpen = -1;
    }
    
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
IPTR Levelmeter__MUIM_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    //struct Levelmeter_DATA *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg)msg);

    msg->MinMaxInfo->MinWidth  += TOTAL_WIDTH;
    msg->MinMaxInfo->MinHeight += TOTAL_HEIGHT;
    msg->MinMaxInfo->DefWidth  += TOTAL_WIDTH;
    msg->MinMaxInfo->DefHeight += TOTAL_HEIGHT;
    msg->MinMaxInfo->MaxWidth  += TOTAL_WIDTH;
    msg->MinMaxInfo->MaxHeight += TOTAL_HEIGHT;

    return TRUE;
}

static void DrawScale(struct RastPort *rp, LONG x1, LONG y1, LONG x2, LONG y2, LONG pen)
{
    LONG cx = (x1 + x2 + 1) / 2;
    LONG cy = y2 - 1;
    LONG rx = cx - x1 - 1;
    LONG ry = cy - y1 - 1;
    LONG a, b, g;
    
    SetABPenDrMd(rp, pen, 0, JAM1);
    
    for(g = 0; g <= 180; g += 15)
    {
    	double angle = ((double)g) * 3.14159265358979323846 / 180.0;
    	
    	a = cx + (LONG)(cos(angle) * rx);
    	b = cy - (LONG)(sin(angle) * ry);

    	WritePixel(rp, a, b);
	
	if ((g % 45) == 0)
	{
	    static WORD offtable[][2] =
	    {
	    	{-1,0 },
		{-1,1 },
		{0 ,1 },
		{ 1,1 },
		{ 1,0 }
	    };
	    
	    WritePixel(rp, a + offtable[g / 45][0], b + offtable[g / 45][1]);
	}
	
    }
    
}

static void DrawNeedle(struct RastPort *rp, LONG x1, LONG y1, LONG x2, LONG y2, double angle, LONG pen)
{
    LONG cx = (x1 + x2 + 1) / 2;
    LONG cy = y2 - 1;
    LONG rx = cx - x1 - 4;
    LONG ry = cy - y1 - 4;
    LONG a, b;
    
    SetABPenDrMd(rp, pen, 0, JAM1);
    Move(rp, cx, cy);
    
    if ((angle < 0.0) | (angle > 180.0)) angle = 0.0;
    angle = 180.0 - angle;
    
    a = cx + (LONG)(cos(angle * 3.14159265358979323846 / 180.0) * rx);
    b = cy - (LONG)(sin(angle * 3.14159265358979323846 / 180.0) * ry);

    Draw(rp, a, b);
    
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
IPTR Levelmeter__MUIM_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct Levelmeter_DATA *data = INST_DATA(cl, obj);
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
	y1 += BORDERSIZE_Y; y2 = y1 + LEVEL_HEIGHT + INNERFRAME_H - 1;
	
	Move(rp, x1, y1);
	Draw(rp, x1 + 2, y1);
	Draw(rp, x1, y1 + 2);
	Draw(rp, x1, y1 + 1);
	
	Move(rp, x2, y1);
	Draw(rp, x2 - 2, y1);
	Draw(rp, x2, y1 + 2);
	Draw(rp, x2, y1 + 1);
	
	Move(rp, x1, y2);
	Draw(rp, x1 + 2, y2);
	Draw(rp, x1, y2 - 2);
	Draw(rp, x1, y2 - 1);
	
	Move(rp, x2, y2);
	Draw(rp, x2 - 2, y2);
	Draw(rp, x2, y2 - 2);
	Draw(rp, x2, y2 - 1);
	
	SetAPen(rp, _pens(obj)[MPEN_HALFSHADOW]);
	Move(rp, x1 + 2, y2 - 1);
	Draw(rp, x1, y2 - 3);
	Draw(rp, x1, y1 + 3);
	Draw(rp, x1 + 3, y1);
	Draw(rp, x2 - 3, y1);
	Draw(rp, x2 - 1, y1 + 2);

	SetAPen(rp, _pens(obj)[MPEN_SHINE]);
	Move(rp, x2, y1 + 3);
	Draw(rp, x2, y2 - 3);
	Draw(rp, x2 - 3, y2);
	Draw(rp, x1 + 3, y2);
	
	SetAPen(rp, _pens(obj)[MPEN_SHADOW]);
	Move(rp, x1 + 3, y1 + 1);
	Draw(rp, x2 - 3, y1 + 1);
    	Move(rp, x1 + 1, y1 + 3);
	Draw(rp, x1 + 1, y2 - 3);
	Move(rp, x1 + 3, y2 - 1);
    	Draw(rp, x2 - 3, y2 - 1);
	Move(rp, x2 - 1, y1 + 3),
    	Draw(rp, x2 - 1, y2 - 3);
	
	/* Levelmeter bg */
	
	x1 += INNERFRAME_X; x2 -= INNERFRAME_X;
	y1 += INNERFRAME_Y; y2 -= INNERFRAME_Y;
	
	SetAPen(rp, data->levelbgpen);
	RectFill(rp, x1 + 1, y1, x2 - 1, y1);
	RectFill(rp, x1, y1 + 1, x2, y2 - 1);
	RectFill(rp, x1 + 1, y2, x2 - 1, y2);
	
	SetAPen(rp, _pens(obj)[MPEN_SHADOW]);
	WritePixel(rp, x1, y1);
	WritePixel(rp, x2, y1);
	WritePixel(rp, x1, y2);
	WritePixel(rp, x2, y2);

	/* Levelmeter scale */
	
	DrawScale(rp, x1, y1, x2, y2, _pens(obj)[MPEN_SHINE]);
	
	/* Level-Label spacing */
	
	x1 -= INNERFRAME_X; x2 += INNERFRAME_X;
	y1 = y2 + INNERFRAME_Y + 1;
	
	SetAPen(rp, _pens(obj)[MPEN_HALFSHINE]);
	RectFill(rp, x1, y1, x2, y1 + LEVEL_LABEL_SPACING - 1);
	
	/* Label Frame */

    	y1 += LEVEL_LABEL_SPACING;
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
    
    x1 = _mleft(obj) + OUTERFRAME_X + BORDERSIZE_X + INNERFRAME_X;
    x2 = _mright(obj) - OUTERFRAME_X - BORDERSIZE_X - INNERFRAME_X;
    y1 = _mtop(obj) + OUTERFRAME_Y + BORDERSIZE_Y + INNERFRAME_Y;
    y2 = y1 + LEVEL_HEIGHT - 1;
    
    if (msg->flags & MADF_DRAWUPDATE)
    {
    	DrawNeedle(rp, x1, y1, x2, y2, data->prevangle, data->levelbgpen);
    }
    
    data->prevangle = (double)DoMethod(obj, MUIM_Numeric_ValueToScale, 0, 180);

    DrawNeedle(rp, x1, y1, x2, y2, data->prevangle, _pens(obj)[MPEN_SHINE]);
    
    return TRUE;
}

#if ZUNE_BUILTIN_LEVELMETER
BOOPSI_DISPATCHER(IPTR, Levelmeter_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Levelmeter__OM_NEW(cl, obj, (struct opSet *)msg);
	case MUIM_Setup: return Levelmeter__MUIM_Setup(cl, obj, (struct MUIP_Setup *)msg);
	case MUIM_Cleanup: return Levelmeter__MUIM_Cleanup(cl, obj, (struct MUIP_Cleanup *)msg);		
	case MUIM_AskMinMax: return Levelmeter__MUIM_AskMinMax(cl, obj, (struct MUIP_AskMinMax *)msg);
	case MUIM_Draw: return Levelmeter__MUIM_Draw(cl, obj, (struct MUIP_Draw *)msg);
        default:     return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Levelmeter_desc =
{ 
    MUIC_Levelmeter, 
    MUIC_Numeric, 
    sizeof(struct Levelmeter_DATA), 
    (void*)Levelmeter_Dispatcher 
};
#endif /* ZUNE_BUILTIN_LEVELMETER */
