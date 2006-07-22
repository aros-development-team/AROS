/* 
    Copyright © 2002-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>
#include <stdlib.h>

#include <exec/types.h>

#include <clib/alib_protos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "prefs.h"
#include "balance_private.h"

/*  #define MYDEBUG 0 */
#include "debug.h"

extern struct Library *MUIMasterBase;

/*
 *  [FirstBound .... <- balance -> .... SecondBound]
 */


IPTR Balance__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Balance_DATA *data;
    struct TagItem *tags,*tag;

    obj = (Object *)DoSuperMethodA(cl, obj, msg);

    if (!obj)
	return NULL;

    /* Initial local instance data */
    data = INST_DATA(cl, obj);

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	case MUIA_Balance_Quiet:
	    break;
	}
    }

    data->ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = cl;

    D(bug("Balance_New(0x%lx)\n",obj));

    return (IPTR)obj;
}

IPTR Balance__MUIM_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Balance_DATA *data = INST_DATA(cl, obj);

    if (!(DoSuperMethodA(cl, obj, (Msg) msg)))
	return FALSE;

    if (_parent(obj))
	get(_parent(obj), MUIA_Group_Horiz, &data->horizgroup);

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);

    return TRUE;
}

IPTR Balance__MUIM_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Balance_DATA *data = INST_DATA(cl, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
IPTR Balance__MUIM_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct Balance_DATA *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg)msg);

    if (data->horizgroup)
    {
	msg->MinMaxInfo->MinWidth  += 3;
	msg->MinMaxInfo->DefWidth  = msg->MinMaxInfo->MinWidth;
	msg->MinMaxInfo->MaxWidth  = msg->MinMaxInfo->DefWidth;
	msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;
    }
    else
    {
	msg->MinMaxInfo->MinHeight += 3;
	msg->MinMaxInfo->DefHeight = msg->MinMaxInfo->MinHeight;
	msg->MinMaxInfo->MaxHeight = msg->MinMaxInfo->DefHeight;
	msg->MinMaxInfo->MaxWidth  = MUI_MAXMAX;
    }

    return TRUE;
}


IPTR Balance__MUIM_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct Balance_DATA *data = INST_DATA(cl, obj);
    struct MUI_RenderInfo *mri;
    LONG col1, col2;

    DoSuperMethodA(cl, obj, (Msg)msg);

    if (!(msg->flags & MADF_DRAWOBJECT))
	return 0;

    if (_mwidth(obj) < 1 || _mheight(obj) < 1)
	return TRUE;

    mri = muiRenderInfo(obj);

    if (data->state == NOT_CLICKED)
    {
	col1 = MPEN_TEXT;
	col2 = MPEN_SHINE;
    }
    else
    {
	col2 = MPEN_TEXT;
	col1 = MPEN_SHINE;
    }

    if (data->horizgroup)
    {
	SetAPen(_rp(obj), _pens(obj)[col1]);
	Move(_rp(obj), _mleft(obj), _mtop(obj));
	Draw(_rp(obj), _mleft(obj), _mbottom(obj));
	SetAPen(_rp(obj), _pens(obj)[MPEN_FILL]);
	Move(_rp(obj), _mleft(obj) + 1, _mtop(obj));
	Draw(_rp(obj), _mleft(obj) + 1, _mbottom(obj));
	SetAPen(_rp(obj), _pens(obj)[col2]);
	Move(_rp(obj), _mleft(obj) + 2, _mtop(obj));
	Draw(_rp(obj), _mleft(obj) + 2, _mbottom(obj));
    }
    else
    {
	SetAPen(_rp(obj), _pens(obj)[col1]);
	Move(_rp(obj), _mleft(obj), _mtop(obj));
	Draw(_rp(obj), _mright(obj), _mtop(obj));
	SetAPen(_rp(obj), _pens(obj)[MPEN_FILL]);
	Move(_rp(obj), _mleft(obj), _mtop(obj) + 1);
	Draw(_rp(obj), _mright(obj), _mtop(obj) + 1);
	SetAPen(_rp(obj), _pens(obj)[col2]);
	Move(_rp(obj), _mleft(obj), _mtop(obj) + 2);
	Draw(_rp(obj), _mright(obj), _mtop(obj) + 2);
    }

    return TRUE;
}

static void draw_object_frame (Object *obj, Object *o, BOOL fixed)
{
    SetAPen(_rp(obj), _pens(obj)[MPEN_TEXT]);
    Move(_rp(obj), _left(o), _top(o));
    Draw(_rp(obj), _left(o), _bottom(o));
    Draw(_rp(obj), _right(o), _bottom(o));
    Draw(_rp(obj), _right(o), _top(o));
    Draw(_rp(obj), _left(o), _top(o));
    if (!fixed)
    {
	Draw(_rp(obj), _right(o), _bottom(o));
	Move(_rp(obj), _right(o), _top(o));
	Draw(_rp(obj), _left(o), _bottom(o));
    }
}

static LONG get_first_bound (struct Balance_DATA *data, Object *obj)
{
    ULONG spacing;

    if (data->horizgroup)
    {
	get(_parent(obj), MUIA_Group_HorizSpacing, &spacing);
	return _left(obj) + _minwidth(obj) + _subwidth(obj) + spacing;
    }
    else
    {
	get(_parent(obj), MUIA_Group_VertSpacing, &spacing);
	return _top(obj) + _minheight(obj) + _subheight(obj) + spacing;
    }
}

static LONG get_second_bound (struct Balance_DATA *data, Object *obj)
{
    ULONG spacing;

    if (data->horizgroup)
    {
	get(_parent(obj), MUIA_Group_HorizSpacing, &spacing);
	return _right(obj) - _minwidth(obj) - _subwidth(obj) - spacing;
    }
    else
    {
	get(_parent(obj), MUIA_Group_VertSpacing, &spacing);
	return _bottom(obj) - _minheight(obj) - _subheight(obj) - spacing;
    }
}


static LONG get_first_bound_multi (struct Balance_DATA *data, Object *obj)
{
    if (data->horizgroup)
    {
	return _mleft(_parent(obj));
    }
    else
    {
	return _mtop(_parent(obj));
    }
}

static LONG get_second_bound_multi (struct Balance_DATA *data, Object *obj)
{
    if (data->horizgroup)
    {
	return _mright(_parent(obj));
    }
    else
    {
	return _mbottom(_parent(obj));
    }
}

static BOOL is_fixed_size (struct Balance_DATA *data, Object *obj)
{
    if (data->horizgroup)
    {
	return (_minwidth(obj) == _maxwidth(obj));
    }
    else
    {
	return (_minheight(obj) == _maxheight(obj));
    }
}


static ULONG get_total_weight_2(struct Balance_DATA *data, Object *objA, Object *objB)
{
    if (data->horizgroup)
    {
	return _hweight(objA) + _hweight(objB);
    }
    else
    {
	return _vweight(objA) + _vweight(objB);
    }
}


static void set_weight_2 (struct Balance_DATA *data, WORD current)
{
    LONG weightB;
    LONG weightA;

    if (current >= data->second_bound)
    {
	weightB = 0;
	weightA = data->total_weight;
	D(bug("weightB = 0\n"));
    }
    else if (current <= data->first_bound)
    {
	weightA = 0;
	weightB = data->total_weight;
	D(bug("weightA = 0\n"));
    }
    else
    {
	D(bug("L=%ld, mid=%ld, R=%ld || M=%d\n",
	      data->first_bound, data->clickpos, data->second_bound,
	      current));
	weightA = (current - data->first_bound + 1) * data->total_weight
	    / (data->second_bound - data->first_bound + 1);

	D(bug("found wA = %ld\n", weightA));
	if (weightA > data->total_weight)
	{
	    D(bug("*** weightA > data->total_weight\n"));
	    weightA = data->total_weight;
	}
	weightB = data->total_weight - weightA;
    }

    if (data->horizgroup)
    {
	_hweight(data->obj_before) = weightA;
	_hweight(data->obj_after) = weightB;
    }
    else
    {
	_vweight(data->obj_before) = weightA;
	_vweight(data->obj_after) = weightB;
    }
}


static void recalc_weights_neighbours (struct IClass *cl, Object *obj, WORD mouse)
{
    struct Balance_DATA *data = INST_DATA(cl, obj);

    if ((data->total_weight == -1) || (data->first_bound == -1) || (data->second_bound == -1))
    {
	Object *sibling;
	Object *object_state;

	object_state = (Object *)data->objs->lh_Head;
	while ((sibling = NextObject(&object_state)))
	{
	    if (!(_flags(sibling) & MADF_SHOWME))
		continue;
/*  	D(bug("sibling %lx\n", sibling)); */
	    if (sibling == obj)
	    {
		while ((sibling = NextObject(&object_state)))
		{
		    if (!(_flags(sibling) & MADF_SHOWME))
			continue;
		    data->obj_after = sibling;
		    break;
		}
		break;
	    }
	    data->obj_before = sibling;
	}
	if (!(data->obj_before && data->obj_after))
	{
	    D(bug("Balance(%0xlx): missing siblings; before=%lx, after=%lx\n",
		  obj, data->obj_before, data->obj_after));
	    return;
	}
	if (data->total_weight == -1)
	    data->total_weight = get_total_weight_2(data, data->obj_before, data->obj_after);
	if (data->first_bound == -1)
	    data->first_bound = get_first_bound(data, data->obj_before);
	if (data->second_bound == -1)
	    data->second_bound = get_second_bound(data, data->obj_after);
    }
	
    set_weight_2(data, mouse);
}

static LONG get_weight (struct Balance_DATA *data, Object *obj)
{
    if (data->horizgroup)
    {
	return _hweight(obj);
    }
    else
    {
	return _vweight(obj);
    }
}

static void set_weight (struct Balance_DATA *data, Object *obj, LONG w)
{
    if (data->horizgroup)
    {
	_hweight(obj) = w;
    }
    else
    {
	_vweight(obj) = w;
    }
}

static LONG get_size (struct Balance_DATA *data, Object *obj)
{
    if (data->horizgroup)
    {
	return _width(obj);
    }
    else
    {
	return _height(obj);
    }
}

#if 0
static void set_interpolated_weight (struct Balance_DATA *data, Object *obj,
				     LONG oldw, LONG neww)
{
    if (data->horizgroup)
    {
	if (oldw)
	    _hweight(obj) = _hweight(obj) * neww / oldw;
	else
	    _hweight(obj) = 0;
    }
    else
    {
	if (oldw)
	    _vweight(obj) = _vweight(obj) * neww / oldw;
	else
	    _vweight(obj) = 0;
    }
  
}
#endif

static void set_weight_all (struct Balance_DATA *data, Object *obj, WORD current)
{
    LONG weightB;
    LONG weightA;
    LONG ldelta, rdelta, lwbygad, rwbygad, lbonus, rbonus, lneg, rneg, lzero, rzero, count;
    int lfirst, rfirst;

    {
	if (data->lsize && data->rsize)
	{
	    weightA = data->lsum
		+ ((current - data->clickpos)
		   * ((data->lsum / (double)data->lsize) + (data->rsum / (double)data->rsize))
		   / 2.0);
	}
	else
	    weightA = data->lsum;

	if (weightA > data->total_weight)
	{
	    D(bug("*** weightA > data->total_weight\n"));
	    weightA = data->total_weight;
	}
	if (weightA < 0)
	{
	    D(bug("*** weightA < 0n"));
	    weightA = 0;
	}
	weightB = data->total_weight - weightA;
	D(bug("normal : weights = %ld/%ld\n", weightA, weightB));
    }

    ldelta = weightA - data->oldWeightA;
    rdelta = weightB - data->oldWeightB;
    lwbygad = ldelta / data->lsiblings;
    rwbygad = rdelta / data->rsiblings;
    lbonus = ldelta % data->lsiblings;
    if (lbonus < 0)
	lbonus = -lbonus;
    rbonus = rdelta % data->rsiblings;
    if (rbonus < 0)
	rbonus = -rbonus;
    lfirst = (int) ((double)data->lsiblings*rand()/(RAND_MAX+1.0));
    rfirst = (int) ((double)data->rsiblings*rand()/(RAND_MAX+1.0));

    count = 0;
    do
    {
	Object *sibling;
	Object *object_state;
	WORD left = data->lsiblings;

	D(bug("delta=%ld/%ld; wbygad=%ld/%ld; bonus=%ld/%ld; first=%d/%d\n", ldelta, rdelta,
	      lwbygad, rwbygad, lbonus, rbonus, lfirst, rfirst));

	if (count++ == 4)
	{
	    D(bug("avoiding deadloop\n"));
	    break;
	}
	lneg = 0;
	rneg = 0;
	lzero = 0;
	rzero = 0;

/*  	D(bug("left weight : from %d to %d\n", data->lsum, weightA)); */
/*  	D(bug("right weight : from %d to %d\n", data->rsum, weightB)); */
	object_state = (Object *)data->objs->lh_Head;
	while ((sibling = NextObject(&object_state)))
	{
	    if (!(_flags(sibling) & MADF_SHOWME))
		continue;
	    if (is_fixed_size(data, sibling))
		continue;

/*  	    D(bug(" B %d\n", left)); */
	    if (left > 0)
	    {
		WORD w1, w2;
		w1 = get_weight(data, sibling);
		w2 = w1;
		if (w2 || (count < 2))
		{
		    w2 += lwbygad;
		    if ((lfirst-- <= 0) && lbonus-- > 0)
			w2 += ((ldelta > 0) ? 1 : -1);
		    if (w2 < 0)
		    {
			lzero++;
			lneg += w2;
			w2 = 0;
		    }
		    set_weight(data, sibling, w2);
		    /*  D(bug(" w (left) from %d to %d (%ld -> %ld)\n", w1, w2, data->oldWeightA, weightA)); */
		}
	    }
	    else
	    {
		WORD w1, w2;
		w1 = get_weight(data, sibling);
		w2 = w1;
		if (w2 || (count < 2))
		{
		    w2 += rwbygad;
		    if ((rfirst-- <= 0) && rbonus-- > 0)
			w2 += ((rdelta > 0) ? 1 : -1);
		    if (w2 < 0)
		    {
			rzero++;
			rneg += w2;
			w2 = 0;
		    }
		    set_weight(data, sibling, w2);
		   /*   D(bug(" w (right) from %d to %d (%ld -> %ld)\n", w1, w2, data->oldWeightB, weightB)); */
		}
	    }
	    left--;
	}
	if (lzero == data->lsiblings)
	    break;
	if (rzero == data->rsiblings)
	    break;
	
	lwbygad = lneg / (data->lsiblings - lzero);
	lbonus = -(lneg % (data->lsiblings - lzero)) + lbonus;
	rwbygad = rneg / (data->rsiblings - rzero);
	rbonus = -(rneg % (data->rsiblings - rzero)) + rbonus;
    } while (lneg || rneg || (lbonus > 0) || (rbonus > 0));

    data->oldWeightA = weightA;
    data->oldWeightB = weightB;
}


static void recalc_weights_all (struct IClass *cl, Object *obj, WORD mouse)
{
    struct Balance_DATA *data = INST_DATA(cl, obj);
    Object *first = NULL;
    Object *last = NULL;

    D(bug("recalc_weights_all\n"));

    if ((data->total_weight == -1) || (data->lsiblings == -1) || (data->rsiblings == -1)
	|| (data->first_bound == -1) || (data->second_bound == -1))
    {
	Object *sibling;
	Object *object_state;
	Object *next;
	BOOL mid = FALSE;

	data->lsum = 0;
	data->rsum = 0;
	data->lsize = 0;
	data->rsize = 0;

	object_state = (Object *)data->objs->lh_Head;
	for (next = NextObject(&object_state); next ; next = NextObject(&object_state))
	{
	    sibling = next;
	    if (!(_flags(sibling) & MADF_SHOWME))
		continue;

	    if (sibling == obj)
	    {
		mid = TRUE;
		data->rsiblings = 0;
		continue;
	    }

	    if (is_fixed_size(data, sibling))
		continue;

	    if (!first)
	    {
		first = sibling;
		data->lsiblings = 1;
		data->lsum += get_weight(data, sibling);
		data->lsize += get_size(data, sibling);
	    }
	    else
	    {
		if (!mid)
		{
		    data->lsiblings++;
		    data->lsum += get_weight(data, sibling);
		    data->lsize += get_size(data, sibling);
		}
		else
		{
		    data->rsiblings++;
		    data->rsum += get_weight(data, sibling);
		    data->rsize += get_size(data, sibling);
		    last = sibling;
		}
	    }
	}

	if (!first || !mid || !last)
	    return;
	if (data->total_weight == -1)
	    data->total_weight = data->lsum + data->rsum;

	if (data->first_bound == -1)
	    data->first_bound = get_first_bound_multi(data, first);
    
	if (data->second_bound == -1)
	    data->second_bound = get_second_bound_multi(data, last);

	data->oldWeightA = data->lsum;
	data->oldWeightB = data->rsum;
    }
    D(bug("Total Weight = %ld, left = %ld, right = %ld\n", data->total_weight, data->lsum, data->rsum));
    D(bug("bound 1 = %ld, bound 2 = %ld\n", data->first_bound, data->second_bound));
    set_weight_all(data, obj, mouse);
}


static void handle_move (struct IClass *cl, Object *obj, WORD mouse)
{
    struct Balance_DATA *data = INST_DATA(cl, obj);
    
    if (data->state == CLICKED)
	recalc_weights_all(cl, obj, mouse);
    else if (data->state == SHIFT_CLICKED)
	recalc_weights_neighbours(cl, obj, mouse);
    else
	return;

    /* relayout with new weights */
    DoMethod(_parent(obj), MUIM_Layout);

    /* full drawing, or sketch */
    if (muiGlobalInfo(obj)->mgi_Prefs->balancing_look == BALANCING_SHOW_OBJECTS)
    {
	MUI_Redraw(_parent(obj), MADF_DRAWALL);
    }
    else
    {
	Object *sibling;
	Object *object_state;

	DoMethod(_parent(obj), MUIM_DrawBackground, _mleft(_parent(obj)),
		 _mtop(_parent(obj)), _mwidth(_parent(obj)), _mheight(_parent(obj)),
		 0, 0, 0);
	/* for each child, draw a black frame */
	object_state = (Object *)data->objs->lh_Head;
	while ((sibling = NextObject(&object_state)))
	{
	    if (!(_flags(sibling) & MADF_SHOWME))
		continue;
/*  	D(bug("sibling %lx\n", sibling)); */
	    
	    draw_object_frame(obj, sibling, is_fixed_size(data, sibling));
	}
    }
}


/**************************************************************************
 MUIM_HandleEvent
**************************************************************************/
IPTR Balance__MUIM_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct Balance_DATA *data = INST_DATA(cl, obj);

    if (msg->imsg)
    {
	switch (msg->imsg->Class)
	{
	    case IDCMP_MOUSEBUTTONS:
	        if (msg->imsg->Code == SELECTDOWN)
	        {
	            if (_isinobject(msg->imsg->MouseX, msg->imsg->MouseY))
	            {
			get(_parent(obj), MUIA_Group_ChildList, &data->objs);
		        data->clickpos = data->horizgroup ? msg->imsg->MouseX : msg->imsg->MouseY;
			data->lastpos = data->clickpos;
			DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
			data->ehn.ehn_Events |= IDCMP_MOUSEMOVE;
			DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
			if (msg->imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
			{
			    data->state = SHIFT_CLICKED;
			    data->obj_before = NULL;
			    data->obj_after = NULL;
			}
			else
			{
			    data->state = CLICKED;
			    data->lsiblings = -1;
			    data->rsiblings = -1;
			}
			data->total_weight = -1;
			data->first_bound = -1;
			data->second_bound = -1;
			srand(1);
			MUI_Redraw(obj, MADF_DRAWOBJECT);
		    }
	        }
		else /* msg->imsg->Code != SELECTDOWN */
   	        {
		    if (data->state != NOT_CLICKED)
		    {
			DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
			data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
			DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
			data->state = NOT_CLICKED;
			if (data->total_weight != -1)
			    MUI_Redraw(_parent(obj), MADF_DRAWALL);
			else
			    MUI_Redraw(obj, MADF_DRAWALL);
	  	    }
		}
		break;

	    case IDCMP_MOUSEMOVE:
	    {
		if ((data->horizgroup) && (msg->imsg->MouseX == data->lastpos))
		    break;
		if ((!data->horizgroup) && (msg->imsg->MouseY == data->lastpos))
		    break;
		data->lazy ^= 1;
		if (data->lazy)
		    break;
		if (data->horizgroup)
		{
		    handle_move(cl, obj, msg->imsg->MouseX);
		    data->lastpos = msg->imsg->MouseX;
		}
		else
		{
		    handle_move(cl, obj, msg->imsg->MouseY);
		    data->lastpos = msg->imsg->MouseY;
		}
	    }
	    break;
	}
    }

    return 0;
}

#if ZUNE_BUILTIN_BALANCE
BOOPSI_DISPATCHER(IPTR, Balance_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
        case OM_NEW:           return Balance__OM_NEW(cl, obj, (struct opSet *) msg);
        case MUIM_Setup:       return Balance__MUIM_Setup(cl, obj, (APTR)msg);
        case MUIM_Cleanup:     return Balance__MUIM_Cleanup(cl, obj, (APTR)msg);
        case MUIM_AskMinMax:   return Balance__MUIM_AskMinMax(cl, obj, (APTR)msg);
        case MUIM_Draw:        return Balance__MUIM_Draw(cl, obj, (APTR)msg);
        case MUIM_HandleEvent: return Balance__MUIM_HandleEvent(cl, obj, (APTR)msg);
        default:               return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Balance_desc =
{ 
    MUIC_Balance,
    MUIC_Area,
    sizeof(struct Balance_DATA),
    (void*)Balance_Dispatcher
};
#endif
