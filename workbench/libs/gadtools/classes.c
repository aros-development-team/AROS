/*
   (C) 1997 AROS - The Amiga Replacement OS
   $Id$

   Desc: Internal GadTools classes.
   Lang: English
 */
#include <proto/exec.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <intuition/intuition.h>
#include <intuition/cghooks.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <utility/tagitem.h>
#include <devices/inputevent.h>
#include <proto/alib.h>
#include <proto/utility.h>

#define SDEBUG 1
#define DEBUG 1
#include <aros/debug.h>

#include "gadtools_intern.h"

#define G(x) ((struct Gadget *)(x))

#define GadToolsBase ((struct GadToolsBase_intern *)cl->cl_UserData)

/*************************** BUTTON_KIND *****************************/

struct ButtonData {
    struct DrawInfo *dri;
    struct Image *frame;
};


Object *button_new(Class * cl, Object * obj, struct opSet *msg)
{
    struct ButtonData *data;
    struct DrawInfo *dri;
    struct Image *frame;
    struct TagItem tags[] =
    {
	{IA_Width, 0UL},
	{IA_Height, 0UL},
	{IA_Resolution, 0UL},
	{IA_FrameType, FRAME_BUTTON},
	{TAG_DONE, 0UL}
    };

    dri = (struct DrawInfo *) GetTagData(GA_DrawInfo, NULL, msg->ops_AttrList);
    if (!dri)
	return NULL;

    tags[0].ti_Data = GetTagData(GA_Width, 0, msg->ops_AttrList);
    tags[1].ti_Data = GetTagData(GA_Height, 0, msg->ops_AttrList);
    tags[2].ti_Data = (dri->dri_Resolution.X << 16) + dri->dri_Resolution.Y;
    frame = (struct Image *) NewObjectA(NULL, FRAMEICLASS, tags);
    if (!frame)
	return NULL;

    tags[0].ti_Tag = GA_Image;
    tags[0].ti_Data = (IPTR) frame;
    tags[1].ti_Tag = TAG_MORE;
    tags[1].ti_Data = (IPTR) msg->ops_AttrList;
    obj = (Object *) DoSuperMethod(cl, obj, OM_NEW, tags, msg->ops_GInfo);
    if (!obj) {
	DisposeObject(frame);
	return NULL;
    }
    data = INST_DATA(cl, obj);
    data->dri = dri;
    data->frame = frame;

    return obj;
}


IPTR button_set(Class * cl, Object * obj, struct opSet * msg)
{
    IPTR retval = 0UL;
    struct TagItem *tag, tags[2];
    struct RastPort *rport;

    tag = FindTagItem(GA_Disabled, msg->ops_AttrList);
    if (tag) {
	tags[0].ti_Tag = GA_Disabled;
	tags[0].ti_Data = tag->ti_Data;
	tags[1].ti_Tag = TAG_DONE;
	DoSuperMethod(cl, obj, OM_SET, tags, msg->ops_GInfo);
	retval = TRUE;
    }
    if ((retval) && (((Class *) (*(obj - sizeof(Class *)))) == cl)) {
	rport = ObtainGIRPort(msg->ops_GInfo);
	if (rport) {
	    DoMethod(obj, GM_RENDER, msg->ops_GInfo, rport, GREDRAW_UPDATE);
	    ReleaseGIRPort(rport);
	    retval = FALSE;
	}
    }
    return retval;
}


AROS_UFH3(static IPTR, dispatch_buttonclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, obj, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
    struct ButtonData *data;
    IPTR retval = 0UL;

    switch (msg->MethodID) {
    case OM_NEW:
	retval = (IPTR) button_new(cl, obj, (struct opSet *) msg);
	break;

    case OM_DISPOSE:
	data = INST_DATA(cl, obj);
	DisposeObject(data->frame);
	retval = DoSuperMethodA(cl, obj, msg);
	break;

    case OM_SET:
	retval = button_set(cl, obj, (struct opSet *) msg);
	break;

#define OPG(x) ((struct opGet *)(x))
    case OM_GET:
	data = INST_DATA(cl, obj);
	if (OPG(msg)->opg_AttrID == GA_Disabled)
	    retval = DoSuperMethodA(cl, obj, msg);
	else {
	    *(OPG(msg)->opg_Storage) = 0UL;
	    retval = 0UL;
	}
	break;

    default:
	retval = DoSuperMethodA(cl, obj, msg);
	break;
    }

    return retval;
}

/*************************** TEXT_KIND and NUMERIC_KIND *****************************/

#define TEXTF_CLIPPED	(1 << 0)
#define TEXTF_BORDER	(1 << 1)
#define TEXTF_COPYTEXT	(1 << 2)
struct TextData {
    STRPTR format;
    IPTR toprint;
    UBYTE frontpen;
    UBYTE backpen;
    UBYTE justification;
    UBYTE flags;
    struct TextFont *font;
    UWORD maxnumberlength;
    LONG  (*dispfunc)(struct Gadget *, WORD);

    
};

/******************
**  Text::Set()  **
******************/
IPTR text_set(Class * cl, Object * o, struct opSet * msg)
{
    IPTR retval = 0UL;
    struct TagItem *tag, *tstate;
    struct TextData *data = INST_DATA(cl, o);

    EnterFunc(bug("Text::Set()\n"));
    
    tstate = msg->ops_AttrList;
    
    while ((tag = NextTagItem(&tstate)))
    {
    	IPTR tidata = tag->ti_Data;
    	
    	switch (tag->ti_Tag)
    	{
    	    case GTNM_Number:
    	    	data->toprint = tidata;
    	    	D(bug("GTNM_Number: %ld\n", tidata));
    	    	if (data->dispfunc)
    	    	{
    	    	    data->toprint = (ULONG)data->dispfunc((struct Gadget *)o,
    	    	    					(WORD)data->toprint);
    	    	}
    	    	retval = 1UL;
    	    	break;
    	    
    	    case GTTX_Text:
    	    	/* If the user has GT_SetGadgetAttrs() us to a different text,
    	    	** then don't copy it anymore
    	    	*/
    	    	if (msg->MethodID != OM_NEW)
    	    	{
    	    	    if (data->flags & TEXTF_COPYTEXT)
    	    	    {
    	    	    	FreeVec((APTR)data->toprint);
    	    	    	data->flags &= ~TEXTF_COPYTEXT;
    	    	    }
    	    	    data->toprint = tidata;
    	    	    D(bug("GTTX_Text: %s\n", tidata));
    	    	}
    	    	break;
    	    
    	    case GTTX_Border:	/* [I]	*/
    	    case GTNM_Border:	/* [I]	*/
    	    	if (tidata)
    	    	    data->flags |= TEXTF_BORDER;
    	    	    
    	    	D(bug("Border: %d\n", tidata));
    	    	break;
    	    	

    	    /* case GTTX_FrontPen:  [IS]	*/
    	    case GTNM_FrontPen:	/* [IS]	*/
    	    	data->frontpen = (UBYTE)tidata;
    	    	D(bug("FrontPen: %d\n", tidata));
    	    	break;
    	    	
    	    /* case GTTX_BackPen:	 [IS]	*/
    	    case GTNM_BackPen:	/* [IS]	*/
    	    	data->backpen = (UBYTE)tidata;
    	    	D(bug("BackPen: %d\n", tidata));
    	    	break;

    	    /* case GTTX_Justification:	 [I] */
    	    case GTNM_Justification:	/* [I] */
    	    	data->justification = (UBYTE)tidata;
    	    	D(bug("Justification: %d\n", tidata));
    	    	break;
    	    	
    	    case GTNM_Format:	/* [I]	*/
    	    case GTA_Text_Format:
    	    	data->format = (STRPTR)tidata;
    	    	D(bug("Format: %s\n", tidata));
    	    	break;
    	    	
    	    /* case GTTX_Clipped:	 [I]	*/
    	    case GTNM_Clipped:
    	    	if (tidata)
    	    	    data->flags |= TEXTF_CLIPPED;
    	    	D(bug("Clipped: %d\n", tidata));
    	    	break;
    	    	
    	    case GTNM_MaxNumberLen:	/* [I]	*/
    	    	data->maxnumberlength = tidata;
    	    	D(bug("MaxNumberLen: %d\n", tidata));
    	    	break;
    	    
    	} /* switch() */
    
    } /* while (iterate taglist) */
    

    ReturnInt ("Text::Set", IPTR, retval);
}



/******************
**  Text::New()  **
******************/
Object *text_new(Class * cl, Object * o, struct opSet *msg)
{

    EnterFunc(bug("Text::New()\n"));
    o = (Object *) DoSuperMethodA(cl, o, (Msg)msg);
    if (o)
    {
    	struct TextData *data = INST_DATA(cl, o);
    	struct TextAttr *tattr, def_tattr;
    	struct DrawInfo *dri;
    	
   	/* Set some defaults */
    	data->format	= "%ld";
    	data->flags 	= 0;
    	data->frontpen	= TEXTPEN;
    	data->backpen  	= BACKGROUNDPEN;
    	data->toprint 	= NULL;
    	data->font 	= NULL;
    	data->maxnumberlength = 0; /* This means "no limit" */
    	data->dispfunc = (APTR)GetTagData(GTA_Text_DispFunc, NULL, msg->ops_AttrList);
    	
    	/* Open font to use for gadget */
    	
    	/* We will *ALWAYS* have a valid DrawInfo struct */
    	dri = (struct DrawInfo *)GetTagData(GA_DrawInfo, NULL, msg->ops_AttrList);

    	def_tattr.ta_Name  = dri->dri_Font->tf_Message.mn_Node.ln_Name;
    	def_tattr.ta_YSize = dri->dri_Font->tf_YSize;
    	def_tattr.ta_Style = 0;
    	def_tattr.ta_Flags = 0;

    	tattr = (struct TextAttr *)GetTagData(GA_TextAttr, (IPTR)&def_tattr, msg->ops_AttrList);

    	data->font = OpenFont(tattr);
    	if (!data->font)
    	   goto error;
    	
    	
    	if (GetTagData(GTTX_CopyText, (IPTR)FALSE, msg->ops_AttrList))
    	{
    	    STRPTR text;
    	    
    	    D(bug("Got GTTX_CopyText\n"));
    	    text = (STRPTR)GetTagData(GTTX_Text, (IPTR)"Text MUST be passed with OM_NEW", msg->ops_AttrList);
    	     
    	    D(bug("Text: %s\n", text));
    	    /* Allocate copy buffer for the text */
    	    data->toprint = (IPTR)AllocVec(strlen(text) + 1, MEMF_ANY);
    	    if (data->toprint)
    	    {
    	    	data->flags |= TEXTF_COPYTEXT;
    	    	D(bug("Copying text\n"));
    	    	strcpy((STRPTR)data->toprint, text);
    	    }
    	    else
    	    	goto error;
    	}

    	D(bug("calling text_set\n"));
    	text_set(cl, o, msg);
    }
    ReturnPtr ("Text::New", Object *, o);
    
error:
     CoerceMethod(cl, o, OM_DISPOSE);
     ReturnPtr ("Text::New", Object *, NULL);
}

/*******************
**  Text::Render  **
*******************/
#define HBORDER 2
#define VBORDER 2

#undef GadToolsBase

VOID renderframe(struct RastPort *rp, struct Gadget *gad, UWORD *pens,
		struct GadToolsBase_intern *GadToolsBase)
{
    WORD left, top, right, bottom;

    left = gad->LeftEdge; top = gad->TopEdge;
    right = left + gad->Width - 1; bottom = top + gad->Height - 1;
    
    /* Left */
    SetAPen(rp, pens[SHADOWPEN]);
    RectFill(rp, left, top, left + HBORDER - 1, bottom);
    
    /* Right */
    SetAPen(rp, pens[SHINEPEN]);
    RectFill(rp, right - HBORDER + 1, top, right, bottom);

    /* Top */
    SetAPen(rp, pens[SHADOWPEN]);    		
    RectFill(rp, left + HBORDER, top, right - 1, top + HBORDER - 1);
    
    /* Bottom */
    SetAPen(rp, pens[SHINEPEN]);
    RectFill(rp, left + 1, bottom - HBORDER + 1, right, bottom);

    return;
}

AROS_UFH2 (void, puttostr,
	AROS_UFHA(UBYTE, chr, D0),
	AROS_UFHA(STRPTR *,strPtrPtr,A3)
)
{
    AROS_LIBFUNC_INIT
    D(bug("Putting character %c into buffer at adress %p\n",
    	chr, *strPtrPtr));
    *(*strPtrPtr)= chr;
    (*strPtrPtr) ++;
    AROS_LIBFUNC_EXIT
}

#define GadToolsBase ((struct GadToolsBase_intern *)(cl->cl_UserData))

VOID text_render(Class *cl, Object *o, struct gpRender *msg)
{
    UWORD *pens = msg->gpr_GInfo->gi_DrInfo->dri_Pens;
    UBYTE textbuf[256], *str;
    struct TextData *data = INST_DATA(cl, o);
    WORD left, top, width, height, numchars, tlength;
    struct TextFont *oldfont;
    struct RastPort *rp = msg->gpr_RPort;
    
    EnterFunc(bug("Text::Render()\n"));
    
    /* preserve font */
    oldfont = rp->Font;
    SetFont(rp, data->font);
    
    /* Create the text */
    str = textbuf;
    RawDoFmt(data->format, &(data->toprint), (VOID_FUNC)puttostr, &str);
    
    D(bug("Text formatted into: %s\n", textbuf));
    numchars = strlen(textbuf);
    
    left   = G(o)->LeftEdge;
    top    = G(o)->TopEdge;
    width  = G(o)->Width;
    height = G(o)->Height;
    
    if (data->flags & TEXTF_BORDER)
    {
    	/* Render the border */
    	D(bug("Rendering frame\n"));
    	renderframe(rp, (struct Gadget *)o, pens, GadToolsBase);
    	
    	left += VBORDER + 1;
    	top  += HBORDER + 1;
    	width  = G(o)->Width -  (VBORDER + 1) * 2;
    	height = G(o)->Height - (HBORDER + 1) * 2;
    }
    
    if (data->flags & TEXTF_CLIPPED)
    {
    	struct TextExtent te;
    	
    	/* See how many chars fits into the display area */
    	numchars = TextFit(rp, textbuf, numchars, &te, NULL, 1, width, height);
    }
    
    tlength = TextLength(rp, textbuf, numchars);
    
    switch (data->justification)
    {
    	case GTJ_LEFT:
    	    break;
    	case GTJ_RIGHT:
    	    left += (width - tlength);
    	    break;
    	case GTJ_CENTER:
    	    left += ((width - tlength) / 2);
    	    break;
    }
    
    /* Render text */
    D(bug("Rendering text of lenghth %d at (%d, %d)\n", numchars, left, top));
    SetABPenDrMd(rp, pens[data->frontpen], pens[data->backpen], JAM2);
    Move(rp, left, 
    	top + ((height - data->font->tf_YSize) / 2) + data->font->tf_Baseline);
    Text(rp, textbuf, numchars);
    
    SetFont(rp, oldfont);
    
    ReturnVoid("Text::Render");
}

AROS_UFH3(static IPTR, dispatch_textclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, o, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
    struct TextData *data;
    IPTR retval = 0UL;

    switch (msg->MethodID) {
    case OM_NEW:
	retval = (IPTR) text_new(cl, o, (struct opSet *) msg);
	break;

    case OM_DISPOSE:
	data = INST_DATA(cl, o);
	
	if (data->flags & TEXTF_COPYTEXT)
	     FreeVec((APTR)data->toprint);
	     
	if (data->font)
	    CloseFont(data->font);
	  
	retval = DoSuperMethodA(cl, o, msg);
	break;

    case OM_SET:
    case OM_UPDATE:
	retval = DoSuperMethodA(cl, o, msg);
	retval += text_set(cl, o, (struct opSet *) msg);

	/* If we have been subclassed, OM_UPDATE should not cause a GM_RENDER
	 * because it would circumvent the subclass from fully overriding it.
	 * The check of cl == OCLASS(o) should fail if we have been
	 * subclassed, and we have gotten here via DoSuperMethodA().
	 */
	if ( retval && ( msg->MethodID == OM_UPDATE ) && ( cl == OCLASS(o) ) )
	{
	    struct GadgetInfo *gi = ((struct opSet *)msg)->ops_GInfo;
	    if (gi)
	    {
		struct RastPort *rp = ObtainGIRPort(gi);
		if (rp)
		{
		    DoMethod(o, GM_RENDER, gi, rp, GREDRAW_REDRAW);
		    ReleaseGIRPort(rp);
		} /* if */
	    } /* if */
	} /* if */
	break;
	
    case GM_RENDER:
    	text_render(cl, o, (struct gpRender *)msg);
    	break;
    	
    case GM_GOACTIVE:
    	retval = GMR_NOREUSE;
    	break;

#define OPG(x) ((struct opGet *)(x))
    case OM_GET:
	data = INST_DATA(cl, o);
	if (OPG(msg)->opg_AttrID == GA_Disabled)
	    retval = DoSuperMethodA(cl, o, msg);
	else {
	    *(OPG(msg)->opg_Storage) = 0UL;
	    retval = 0UL;
	}
	break;

    default:
	retval = DoSuperMethodA(cl, o, msg);
	break;
    }

    return retval;
}

/*************************** SLIDER_KIND *****************************/

struct SliderData {
    WORD   min;
    WORD   max;
    WORD   level;
    UBYTE labelplace;
    
};

#undef GadToolsBase


STATIC VOID notifylevel(Class *cl, Object *o, WORD level, struct GadgetInfo *ginfo,
			struct GadToolsBase_intern *GadToolsBase)
{			

    struct TagItem ntags[] =
    {
    	{GTSL_Level,	(IPTR)NULL},
    	{GA_ID,	0},
    	{TAG_DONE,}
    };
    	    	    
    ntags[0].ti_Data = (IPTR)level;
    ntags[1].ti_Data = G(o)->GadgetID;
    DoSuperMethod(cl, o, OM_NOTIFY, ntags, ginfo, 0);
    
    return;
}

#define GadToolsBase ((struct GadToolsBase_intern *)(cl->cl_UserData))

/********************
**  Slider::Set()  **
********************/
STATIC IPTR slider_set(Class * cl, Object * o, struct opSet * msg)
{
    IPTR retval = 0UL;
    struct TagItem *tag, *tstate, tags[] =
    {
    	{PGA_Top,	0},
    	{PGA_Total,	0},
    	{TAG_MORE,	(IPTR)NULL}
    };
    struct SliderData *data = INST_DATA(cl, o);
    
    BOOL val_set = FALSE;
    tstate = msg->ops_AttrList;
    
    while ((tag = NextTagItem(&tstate)))
    {
    	IPTR tidata = tag->ti_Data;
    	
    	switch (tag->ti_Tag)
    	{
    	    case GTSL_Min:
    	    	data->min = (WORD)tidata;
		val_set = TRUE;
		break;
    	    case GTSL_Max:
    	    	data->max = (WORD)tidata;
    	    	val_set = TRUE;
    	    	break;
    	    	
    	    case GTSL_Level:	/* [ISN] */
    	    	if (tidata != data->level)
    	    	{
    	    	    data->level = (WORD)tidata;
    	    	    notifylevel(cl, o, data->level, msg->ops_GInfo, GadToolsBase);
		    val_set = TRUE;
		    
    	    	}
    	    	break;
    	    	
    	} /* switch () */
    	
     } /* while (iterate taglist) */
    
    if (val_set)
    {
    	tags[0].ti_Data = data->min - data->level;
    	tags[1].ti_Data = data->max - data->min;
    	tags[2].ti_Data = (IPTR)msg->ops_AttrList;
    	
    	retval = 1UL;
    }

    return (DoSuperMethod(cl, o, msg->MethodID, msg->ops_GInfo, tags));
}



/********************
**  Slider::New()  **
********************/
STATIC Object *slider_new(Class * cl, Object * o, struct opSet *msg)
{
    WORD min, max, level;
    
    struct TagItem tags[] =
    {
     	{PGA_Total,	0},
     	{PGA_Visible,	1},
     	{PGA_Top,	0},
     	{TAG_MORE,	(IPTR)NULL}
    };

    EnterFunc(bug("Slider::New()\n"));

    min   = GetTagData(GTSL_Min,   0,  msg->ops_AttrList);
    D(bug("min=%d\n", min));
    max   = GetTagData(GTSL_Max,   15, msg->ops_AttrList);
    D(bug("max=%d\n", max));    
    level = GetTagData(GTSL_Level, 0,  msg->ops_AttrList);
    D(bug("level=%d\n", level));        

    tags[0].ti_Data = max - min;
    tags[2].ti_Data = level - min;
    tags[3].ti_Data = (IPTR)msg->ops_AttrList;
    
    o = (Object *)DoSuperMethod(cl, o, OM_NEW, tags, NULL);
    if (o)
    {
    	struct SliderData *data = INST_DATA(cl, o);
    	
	data->min   = min;
	data->max   = max;
	data->level = level;
	
    }
    ReturnPtr("Slider::New", Object *, o);
    
}

/*************************
**  Slider::GoActive()  **
*************************/
STATIC IPTR slider_goactive(Class *cl, Object *o, struct gpInput *msg)
{
    IPTR retval;
    struct SliderData *data = INST_DATA(cl, o);

    EnterFunc(bug("Slider::GoActive()\n"));
    retval = DoSuperMethodA(cl, o, (Msg)msg);
    
    if (retval != GMR_MEACTIVE)
    {
    	data->level = (WORD)*(msg->gpi_Termination);
	notifylevel(cl, o, data->level, msg->gpi_GInfo, GadToolsBase);    	
    }
    ReturnInt("Slider::Goactive", IPTR, retval);
}

/****************************
**  Slider::HandleInput()  **
****************************/
STATIC IPTR slider_handleinput(Class *cl, Object *o, struct gpInput *msg)
{
    struct InputEvent *ie = msg->gpi_IEvent;
    IPTR retval;
    struct SliderData *data = INST_DATA(cl ,o);

    EnterFunc(bug("Slider::HandleInput()\n"));
    retval = DoSuperMethodA(cl, o, (Msg)msg);
    /* Mousemove ? */
    if ((ie->ie_Class == IECLASS_RAWMOUSE) && (ie->ie_Code == IECODE_NOBUTTON))
    {
    	LONG top;
    	
    	/* Get the top attribute */
    	DoSuperMethod(cl, o, OM_GET, PGA_Top, &top);
    	
    	/* Level changed ? */
    	if (data->level - data->min != top)
    	{
    	    data->level = data->min + top;
    	    retval = GMR_INTERIMUPDATE;
   	    *(msg->gpi_Termination) = data->level;
	    notifylevel(cl, o, data->level, msg->gpi_GInfo, GadToolsBase);    	

    	}
    }
    else
    {
    	if (retval != GMR_MEACTIVE)
    	{
    	    data->level =(WORD)*(msg->gpi_Termination);
	    notifylevel(cl, o, data->level, msg->gpi_GInfo, GadToolsBase);    	
    	}
    }
    
    ReturnInt("Slider::HandleInput", IPTR, retval);
}

AROS_UFH3(static IPTR, dispatch_sliderclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, o, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{

    IPTR retval = 0UL;

    switch (msg->MethodID) {
    case OM_NEW:
	retval = (IPTR) slider_new(cl, o, (struct opSet *) msg);
	break;

    case OM_SET:
	retval = slider_set(cl, o, (struct opSet *) msg);
	break;

    case GM_GOACTIVE:
    	retval = slider_goactive(cl, o, (struct gpInput *)msg);
    	break;
	
    case GM_HANDLEINPUT:
    	retval = slider_handleinput(cl, o, (struct gpInput *)msg);
    	break;

    default:
	retval = DoSuperMethodA(cl, o, msg);
	break;
    }

    return retval;
}

/*************************** SCROLLER_KIND *****************************/

struct ScrollerData {
	UBYTE dummy;
};

#undef GadToolsBase

#define GadToolsBase ((struct GadToolsBase_intern *)(cl->cl_UserData))

/**********************
**  Scroller::Set()  **
**********************/
IPTR scroller_set(Class * cl, Object * o, struct opSet * msg)
{
    IPTR retval = 0UL;
    struct TagItem *tag, *tstate;
    struct ScrollerData *data = INST_DATA(cl, o);
    
    tstate = msg->ops_AttrList;
    
    while ((tag = NextTagItem(&tstate)))
    {
    	IPTR tidata = tag->ti_Data;
    	
/*    	switch (tag->ti_Tag)
    	{
    	    
    	}
*/    	
    
    }

    return (retval);
}



/**********************
**  Scroller::New()  **
**********************/
Object *scroller_new(Class * cl, Object * o, struct opSet *msg)
{

/*    struct TagItem *tag, *tstate, tags[] =
    {
    	
    }
*/
    return( (Object *) DoSuperMethodA(cl, o, (Msg)msg));
    
}


AROS_UFH3(static IPTR, dispatch_scrollerclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, o, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{

    IPTR retval = 0UL;

    switch (msg->MethodID) {
    case OM_NEW:
	retval = (IPTR) scroller_new(cl, o, (struct opSet *) msg);
	break;

    case OM_SET:
	retval = scroller_set(cl, o, (struct opSet *) msg);
	break;

    default:
	retval = DoSuperMethodA(cl, o, msg);
	break;
    }

    return retval;
}

/*************************** Classes *****************************/

#undef GadToolsBase

Class *makebuttonclass(struct GadToolsBase_intern * GadToolsBase)
{
    Class *cl;

    if (GadToolsBase->buttonclass)
	return GadToolsBase->buttonclass;

    cl = MakeClass(NULL, FRBUTTONCLASS, NULL, sizeof(struct ButtonData), 0UL);
    if (!cl)
	return NULL;
    cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_buttonclass);
    cl->cl_Dispatcher.h_SubEntry = NULL;
    cl->cl_UserData = (IPTR) GadToolsBase;

    GadToolsBase->buttonclass = cl;

    return cl;
}

Class *maketextclass(struct GadToolsBase_intern * GadToolsBase)
{
    Class *cl;

    if (GadToolsBase->textclass)
	return GadToolsBase->textclass;

    cl = MakeClass(NULL, GADGETCLASS, NULL, sizeof(struct TextData), 0UL);
    if (!cl)
	return NULL;
    cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_textclass);
    cl->cl_Dispatcher.h_SubEntry = NULL;
    cl->cl_UserData = (IPTR) GadToolsBase;

    GadToolsBase->textclass = cl;

    return cl;
}

Class *makesliderclass(struct GadToolsBase_intern * GadToolsBase)
{
    Class *cl;

    if (GadToolsBase->sliderclass)
	return GadToolsBase->sliderclass;

    cl = MakeClass(NULL, PROPGCLASS, NULL, sizeof(struct SliderData), 0UL);
    if (!cl)
	return NULL;
    cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_sliderclass);
    cl->cl_Dispatcher.h_SubEntry = NULL;
    cl->cl_UserData = (IPTR) GadToolsBase;

    GadToolsBase->sliderclass = cl;

    return cl;
}

Class *makescrollerclass(struct GadToolsBase_intern * GadToolsBase)
{
    Class *cl;

    if (GadToolsBase->scrollerclass)
	return GadToolsBase->scrollerclass;

    cl = MakeClass(NULL, PROPGCLASS, NULL, sizeof(struct ScrollerData), 0UL);
    if (!cl)
	return NULL;
    cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_scrollerclass);
    cl->cl_Dispatcher.h_SubEntry = NULL;
    cl->cl_UserData = (IPTR) GadToolsBase;

    GadToolsBase->scrollerclass = cl;

    return cl;
}
