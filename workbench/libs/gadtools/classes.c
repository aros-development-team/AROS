/*
   (C) 1997 AROS - The Amiga Research OS
   $Id$

   Desc: Internal GadTools classes.
   Lang: English
 */
 
#undef AROS_ALMOST_COMPATIBLE
#define AROS_ALMOST_COMPATIBLE 1

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
#include <gadgets/aroscheckbox.h>
#include <gadgets/aroscycle.h>
#include <gadgets/arosmx.h>
#include <gadgets/arospalette.h>

#include <string.h> /* memset() */

#define SDEBUG 0
#define DEBUG 1
#include <aros/debug.h>

#include "gadtools_intern.h"

#define G(x) ((struct Gadget *)(x))
#define EG(X) ((struct ExtGadget *)(x))

#define GadToolsBase ((struct GadToolsBase_intern *)cl->cl_UserData)

IPTR scroller_set(Class * cl, Object * o, struct opSet * msg);

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

    /* Catch everything, but GA_Disabled. */
    tag = FindTagItem(GA_Disabled, msg->ops_AttrList);
    if (tag) {
	tags[0].ti_Tag = GA_Disabled;
	tags[0].ti_Data = tag->ti_Data;
	tags[1].ti_Tag = TAG_DONE;
	DoSuperMethod(cl, obj, OM_SET, tags, msg->ops_GInfo);
	retval = TRUE;
    }

    /* Redraw the gadget, if an attribute was changed and if this is the
       objects' base-class. */
    if ((retval) && (OCLASS(obj) == cl)) {
	rport = ObtainGIRPort(msg->ops_GInfo);
	if (rport) {
	    DoMethod(obj, GM_RENDER, msg->ops_GInfo, rport, GREDRAW_UPDATE);
	    ReleaseGIRPort(rport);
	    retval = FALSE;
	}
    }

    return retval;
}

IPTR button_render(Class * cl, Object * obj, struct gpRender * msg)
{
    IPTR retval = 0UL;
    UWORD old_gadgetflags;
    struct IntuiText *old_gadgettext;
    
    /* Georg Steger: Hack, because IntuiTexts are not centered
       by button gadget class */
       
    old_gadgetflags = G(obj)->Flags;
    old_gadgettext = G(obj)->GadgetText;
    
    G(obj)->Flags &= ~GFLG_LABELMASK;
    G(obj)->Flags |= GFLG_LABELITEXT;
    G(obj)->GadgetText = 0;
    
    retval = DoSuperMethodA(cl, obj, (Msg)msg);
 
    G(obj)->GadgetText  = old_gadgettext;
    G(obj)->Flags = old_gadgetflags;
 
    renderlabel(GadToolsBase, (struct Gadget *)obj, msg->gpr_RPort, GV_LabelPlace_In);

   
    return retval;
}

AROS_UFH3S(IPTR, dispatch_buttonclass,
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
	switch (OPG(msg)->opg_AttrID)
	{
	    case GA_Disabled:
	    	retval = DoSuperMethodA(cl, obj, msg);
		break;
	
	    case GTA_GadgetKind:
	    case GTA_ChildGadgetKind:
	    	*(OPG(msg)->opg_Storage) = BUTTON_KIND;
	    	retval = 1UL;
		break;
		
	    default:
	    	*(OPG(msg)->opg_Storage) = 0UL;
	    	retval = 0UL;
		break;
	}
	break;

    case GM_RENDER:
    	retval = button_render(cl, obj, (struct gpRender *) msg);
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
    struct DrawInfo *dri;
    Object *frame;
    
    STRPTR format;
    IPTR toprint;
    UBYTE frontpen;
    UBYTE backpen;
    UBYTE justification;
    UBYTE flags;
    struct TextFont *font;
    UWORD maxnumberlength;
    WORD gadgetkind;
    LONG  (*dispfunc)(struct Gadget *, WORD);
    UBYTE labelplace;
    
};

/******************
**  Text::Set()  **
******************/
IPTR text_set(Class * cl, Object * o, struct opSet * msg)
{
    IPTR retval = 0UL;
    struct TagItem *tag, *tstate;
    struct TextData *data = INST_DATA(cl, o);
    struct RastPort *rport;
    
    EnterFunc(bug("Text::Set()\n"));
    
    tstate = msg->ops_AttrList;
    
    while ((tag = NextTagItem((const struct TagItem **)&tstate)))
    {
    	IPTR tidata = tag->ti_Data;
    	
    	switch (tag->ti_Tag)
    	{
	    case GTA_GadgetKind:
	    	data->gadgetkind = (WORD)tidata;
		break;

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
		retval = 1UL;
    	    	break;
    	    
    	    case GTTX_Border:	/* [I]	*/
    	    case GTNM_Border:	/* [I]	*/
    	    	if (tidata)
    	    	    data->flags |= TEXTF_BORDER;
    	    	    
    	    	D(bug("Border: %d\n", tidata));
    	    	break;
    	    	

    	    /*case GTTX_FrontPen:  [IS]	*/
    	    case GTNM_FrontPen:	/* [IS]	*/
    	    	data->frontpen = (UBYTE)tidata;
    	    	D(bug("FrontPen: %d\n", tidata));
		retval = 1UL;
    	    	break;
    	    	
    	    /* case GTTX_BackPen: [IS]	*/
    	    case GTNM_BackPen:	/* [IS]	*/
    	    	data->backpen = (UBYTE)tidata;
    	    	D(bug("BackPen: %d\n", tidata));
		retval = 1UL;
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
    
    /* Redraw the gadget, if an attribute was changed and if this is the
       objects' base-class. */
    if ((retval) && (OCLASS(o) == cl)) {
	rport = ObtainGIRPort(msg->ops_GInfo);
	if (rport) {
	    DoMethod(o, GM_RENDER, msg->ops_GInfo, rport, GREDRAW_UPDATE);
	    ReleaseGIRPort(rport);
	    retval = FALSE;
	}
    }

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
   	
   	/* Set some defaults */
    	data->format	= "%ld";
    	data->flags 	= 0;
    	data->frontpen	= TEXTPEN;
    	data->backpen  	= BACKGROUNDPEN;
    	data->toprint 	= NULL;
    	data->font 	= NULL;
    	data->maxnumberlength = 0; /* This means "no limit" */
    	data->dispfunc = (APTR)GetTagData(GTA_Text_DispFunc, NULL, msg->ops_AttrList);
    	data->labelplace = GetTagData(GA_LabelPlace, GV_LabelPlace_Left, msg->ops_AttrList);
	
    	/* Open font to use for gadget */
    	
    	/* We will *ALWAYS* have a valid DrawInfo struct */
    	data->dri = (struct DrawInfo *)GetTagData(GA_DrawInfo, NULL, msg->ops_AttrList);

    	def_tattr.ta_Name  = data->dri->dri_Font->tf_Message.mn_Node.ln_Name;
    	def_tattr.ta_YSize = data->dri->dri_Font->tf_YSize;
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
    	} else {
	    STRPTR text;
	    
	    if ((text = (STRPTR)GetTagData(GTTX_Text, NULL, msg->ops_AttrList)))
	    {
	        data->toprint = (IPTR)text;
	    }
	}

    	D(bug("calling text_set\n"));
    	text_set(cl, o, msg);
	
	if (data->flags & TEXTF_BORDER)
	{
	    struct TagItem frame_tags[] =
	    {
		{IA_Width	, GetTagData(GA_Width, 0, msg->ops_AttrList)				},
		{IA_Height	, GetTagData(GA_Height, 0, msg->ops_AttrList)				},
		{IA_Resolution	, (data->dri->dri_Resolution.X << 16) + data->dri->dri_Resolution.Y	},
		{IA_FrameType	, FRAME_BUTTON								},
		{IA_Recessed	, TRUE									},
		{TAG_DONE	, 0UL									}
	    };

	    data->frame = NewObjectA(NULL, FRAMEICLASS, frame_tags);
	}
	
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
    WORD left, left2, top, width, height, numchars, tlength;
    struct TextFont *oldfont;
    struct RastPort *rp = msg->gpr_RPort;
    
    EnterFunc(bug("Text::Render()\n"));

    left   = G(o)->LeftEdge;
    top    = G(o)->TopEdge;
    width  = G(o)->Width;
    height = G(o)->Height;

    if (msg->gpr_Redraw == GREDRAW_REDRAW)
    {
        if (data->frame)
	{
	    DrawImageState(msg->gpr_RPort,
		    (struct Image *)data->frame,
		    left,
		    top,
		    IDS_NORMAL,
		    msg->gpr_GInfo->gi_DrInfo);
	}
	
	renderlabel(GadToolsBase, (struct Gadget *)o, msg->gpr_RPort, data->labelplace);
    }
    
    if (data->toprint || (data->gadgetkind != TEXT_KIND))
    {
	/* preserve font */
	oldfont = rp->Font;
	SetFont(rp, data->font);

	/* Create the text */
	str = textbuf;

	RawDoFmt(data->format, &(data->toprint), (VOID_FUNC)puttostr, &str);

	D(bug("Text formatted into: %s\n", textbuf));
	numchars = strlen(textbuf);

	if (data->flags & TEXTF_BORDER)
	{
    	    left += VBORDER + 1;
    	    top  += HBORDER + 1;
    	    width  = G(o)->Width -  (VBORDER + 1) * 2;
    	    height = G(o)->Height - (HBORDER + 1) * 2;
	}

	if (data->flags & TEXTF_CLIPPED)
	{
    	    struct TextExtent te;

    	    /* See how many chars fits into the display area */
    	    numchars = TextFit(rp, textbuf, numchars, &te, NULL, 1, width, G(o)->Height);
	}

	tlength = TextLength(rp, textbuf, numchars);

	left2 = left;
	switch (data->justification)
	{
    	    case GTJ_LEFT:
    		break;
    	    case GTJ_RIGHT:
    		left2 += (width - tlength);
    		break;
    	    case GTJ_CENTER:
    		left2 += ((width - tlength) / 2);
    		break;
	}

	/* Render text */
	D(bug("Rendering text of lenghth %d at (%d, %d)\n", numchars, left, top));
	SetABPenDrMd(rp, pens[data->backpen], pens[data->backpen], JAM2);
	RectFill(rp, left, top, left + width - 1, top + height - 1);
	SetAPen(rp, pens[data->frontpen]);

	Move(rp, left2, 
    	    top + ((height - data->font->tf_YSize) / 2) + data->font->tf_Baseline);
	Text(rp, textbuf, numchars);

	SetFont(rp, oldfont);

    } /* if (data->toprint || (data->gadgetkind != TEXT_KIND)) */
    
    ReturnVoid("Text::Render");
}

AROS_UFH3S(IPTR, dispatch_textclass,
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
	switch (OPG(msg)->opg_AttrID)
	{
	    case GA_Disabled:
	    	retval = DoSuperMethodA(cl, o, msg);
		break;
		
	    case GTA_GadgetKind:
	    case GTA_ChildGadgetKind:
	    	*(OPG(msg)->opg_Storage) = data->gadgetkind;
		retval = 1UL;
		break;
		
	    default:
	  	*(OPG(msg)->opg_Storage) = 0UL;
	   	retval = 0UL;
		break;
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
    Object *frame;
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
    	{TAG_DONE,}
    };
    
    ntags[0].ti_Data = (IPTR)level;
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
    struct TagItem *tag, *tstate, *dosuper_tags, tags[] =
    {
    	{PGA_Total,	0},
    	{PGA_Top,	0},
    	{TAG_MORE,	(IPTR)NULL}
    };
    struct SliderData *data = INST_DATA(cl, o);
    
    BOOL val_set = FALSE;

    EnterFunc(bug("Slider::Set(attrlist=%p)\n", msg->ops_AttrList));
    dosuper_tags = msg->ops_AttrList;
        
    tstate = msg->ops_AttrList;
    while ((tag = NextTagItem((const struct TagItem **)&tstate)))
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
    	tags[0].ti_Data = data->max - data->min + 1;
    	tags[1].ti_Data = data->level - data->min;
    	tags[2].ti_Data = (IPTR)msg->ops_AttrList;
    	
	/* kprintf("min = %d   man = %d   level = %d\n",data->min,data->max,data->level); */
    	dosuper_tags = tags;
    	
    	retval = 1UL;
    }
    
    ReturnInt ("Slider::Set", IPTR, DoSuperMethod(cl, o, msg->MethodID, dosuper_tags, msg->ops_GInfo));
}



/********************
**  Slider::New()  **
********************/
STATIC Object *slider_new(Class * cl, Object * o, struct opSet *msg)
{
    struct DrawInfo *dri;
    WORD min, max, level;
    
    struct TagItem tags[] =
    {
     	{PGA_Total,	0},
     	{PGA_Visible,	1},
     	{PGA_Top,	0},
     	{TAG_MORE,	(IPTR)NULL}
    };
   struct TagItem fitags[] =
    {
	{IA_Width, 0UL},
	{IA_Height, 0UL},
	{IA_Resolution, 0UL},
	{IA_FrameType, FRAME_BUTTON},
	{IA_EdgesOnly, TRUE},
	{TAG_DONE, 0UL}
    };
    
    EnterFunc(bug("Slider::New()\n"));

    min   = GetTagData(GTSL_Min,   0,  msg->ops_AttrList);
    max   = GetTagData(GTSL_Max,   15, msg->ops_AttrList);
    level = GetTagData(GTSL_Level, 0,  msg->ops_AttrList);

    tags[0].ti_Data = max - min + 1;
    tags[2].ti_Data = level - min;
    tags[3].ti_Data = (IPTR)msg->ops_AttrList;
    
    o = (Object *)DoSuperMethod(cl, o, OM_NEW, tags, NULL);
    if (o)
    {
    	struct SliderData *data = INST_DATA(cl, o);
  
	dri = (struct DrawInfo *)GetTagData(GA_DrawInfo, NULL, msg->ops_AttrList);
	
	fitags[0].ti_Data = GetTagData(GA_Width, 0, msg->ops_AttrList) + BORDERPROPSPACINGX * 2;
	fitags[1].ti_Data = GetTagData(GA_Height, 0, msg->ops_AttrList) + BORDERPROPSPACINGY * 2;
	fitags[2].ti_Data = (dri->dri_Resolution.X << 16) + dri->dri_Resolution.Y;

	data->frame = NewObjectA(NULL, FRAMEICLASS, fitags);
	if (data->frame)
	{
	    scroller_set(cl, o, msg);
	    
	    data->min   = min;
	    data->max   = max;
	    data->level = level;
	    data->labelplace = GetTagData(GA_LabelPlace, GV_LabelPlace_Left, msg->ops_AttrList);
	} else {
	    CoerceMethod(cl, o, OM_DISPOSE);
	    o = NULL;
	}
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
    	data->level = data->min + (WORD)*(msg->gpi_Termination);
	notifylevel(cl, o, data->level, msg->gpi_GInfo, GadToolsBase);    	
    }
    ReturnInt("Slider::Goactive", IPTR, retval);
}

/********************
**  Slider::Get()  **
********************/
STATIC IPTR slider_get(Class *cl, Object *o, struct opGet *msg)
{
    struct SliderData *data;
    IPTR retval = 1UL;
    
    data = INST_DATA(cl,o);
    switch (msg->opg_AttrID)
    {
	case GTA_GadgetKind:
	case GTA_ChildGadgetKind:
	    *msg->opg_Storage = SLIDER_KIND;
	    break;
	
	case GTSL_Level:
	    *msg->opg_Storage = data->level;
	    break;
	
	case GTSL_Max:
	    *msg->opg_Storage = data->max;
	    break;
	
	case GTSL_Min:
	    *msg->opg_Storage = data->min;
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, o, (Msg)msg);
	    break;
    }
    
    return retval;
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
    	
    	/* Get the PGA_Top attribute */
    	DoSuperMethod(cl, o, OM_GET, PGA_Top, &top);
    	
    	/* Level changed ? */
    	if (data->level - data->min != top)
    	{
    	    data->level = data->min + top;
#if 0
    	    retval = GMR_INTERIMUPDATE;
   	    *(msg->gpi_Termination) = data->level;
#endif
	    notifylevel(cl, o, data->level, msg->gpi_GInfo, GadToolsBase);    	

    	}
    }
    else
    {
    	if (retval != GMR_MEACTIVE)
    	{
    	
    	    data->level = data->min + (WORD)*(msg->gpi_Termination);
	    notifylevel(cl, o, data->level, msg->gpi_GInfo, GadToolsBase);
    	}
    }
    
    ReturnInt("Slider::HandleInput", IPTR, retval);
}


/****************************
**  Slider::Render()       **
****************************/
STATIC IPTR slider_render(Class *cl, Object *o, struct gpRender *msg)
{
    struct SliderData *data;
    IPTR retval;
    
    data = INST_DATA(cl, o);
    
    if (msg->gpr_Redraw == GREDRAW_REDRAW)
    {
	DrawImageState(msg->gpr_RPort,
		(struct Image *)data->frame,
		G(o)->LeftEdge - BORDERPROPSPACINGX,
		G(o)->TopEdge  - BORDERPROPSPACINGY,
		IDS_NORMAL,
		msg->gpr_GInfo->gi_DrInfo);
   
    }
    
    retval = DoSuperMethodA(cl, o, (Msg)msg);

    renderlabel(GadToolsBase, (struct Gadget *)o, msg->gpr_RPort, data->labelplace);

    ReturnInt("Slider::Render", IPTR, retval);
}


AROS_UFH3S(IPTR, dispatch_sliderclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, o, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
    struct SliderData *data;
    IPTR retval = 0UL;

    switch (msg->MethodID) {
    case OM_NEW:
	retval = (IPTR) slider_new(cl, o, (struct opSet *) msg);
	break;

    case OM_DISPOSE:
	data = INST_DATA(cl, o);
	DisposeObject(data->frame);
	retval = DoSuperMethodA(cl, o, msg);
	break;

    case OM_SET:
	retval = slider_set(cl, o, (struct opSet *) msg);
	break;

    case OM_GET:
	retval = slider_get(cl, o, (struct opGet *) msg);
	break;

    case GM_GOACTIVE:
    	retval = slider_goactive(cl, o, (struct gpInput *)msg);
    	break;
	
    case GM_HANDLEINPUT:
    	retval = slider_handleinput(cl, o, (struct gpInput *)msg);
    	break;
    	
    case GM_RENDER:
    	retval = slider_render(cl, o, (struct gpRender *)msg);
	break;
	
    default:
	retval = DoSuperMethodA(cl, o, msg);
	break;
    }

    return retval;
}

/*************************** SCROLLER_KIND *****************************/

#ifdef SDEBUG
#   undef SDEBUG
#endif
#ifdef DEBUG
#   undef DEBUG
#endif
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

struct ArrowData {
	Object *arrowimage;
	Object *frame;
	Object *scroller;
	WORD gadgetkind;
	WORD arrowtype;
};


/*******************
**  Arrow::New()  **
*******************/
STATIC Object *arrow_new(Class * cl, Object * o, struct opSet *msg)
{
    struct DrawInfo *dri = (struct DrawInfo *)GetTagData(GA_DrawInfo, NULL, msg->ops_AttrList);
    Object *frame = NULL, *arrowimage = NULL;
    struct TagItem fitags[] =
    {
	{IA_Width, 0UL},
	{IA_Height, 0UL},
	{IA_Resolution, 0UL},
	{IA_FrameType, FRAME_BUTTON},
	{TAG_DONE, 0UL}
    };

   struct TagItem itags[] =
   {
	{SYSIA_Which,	 0},
    	{SYSIA_DrawInfo, 0},
    	{IA_Left,	0},
    	{IA_Top,	0},
	{IA_Width,	0},
	{IA_Height,	0},
	{SYSIA_WithBorder, FALSE},
	{SYSIA_Style, SYSISTYLE_GADTOOLS},    	
    	{TAG_DONE,}
   };
   struct TagItem atags[] =
   {
   	{GA_LabelImage,	0UL},
   	{GA_Image,	0UL},
   	{TAG_MORE,	0UL}
   };
   WORD arrowtype;
 
    EnterFunc(bug("Arrow::New()\n"));
    
    fitags[0].ti_Data = itags[4].ti_Data = GetTagData(GA_Width, 0, msg->ops_AttrList);
    fitags[1].ti_Data = itags[5].ti_Data = GetTagData(GA_Height, 0, msg->ops_AttrList);
    fitags[2].ti_Data = (dri->dri_Resolution.X << 16) + dri->dri_Resolution.Y;
    
    frame = NewObjectA(NULL, FRAMEICLASS, fitags);
    if (!frame)
	return NULL;
	
    itags[0].ti_Data = arrowtype = GetTagData(GTA_Arrow_Type, LEFTIMAGE, msg->ops_AttrList);
    itags[1].ti_Data = (IPTR)dri;
    
    arrowimage = NewObjectA(NULL, SYSICLASS, itags);
    if (!arrowimage)
    	goto failure;
    
    #define IM(o) ((struct Image *)o)	
    D(bug("Created Arrowimage: %p, dims=(%d, %d, %d, %d)\n",
    	arrowimage, IM(arrowimage)->LeftEdge, IM(arrowimage)->TopEdge, IM(arrowimage)->Width, IM(arrowimage)->Height));
    	
    atags[0].ti_Data = (IPTR)arrowimage;
    atags[1].ti_Data = (IPTR)frame;
    atags[2].ti_Data = (IPTR)msg->ops_AttrList;
    
    o = (Object *)DoSuperMethod(cl, o, OM_NEW, atags, NULL);
    if (o)
    {
    	struct ArrowData *data = INST_DATA(cl, o);
    	
    	D(bug("Got object from superclass: %p\n", o));
	data->gadgetkind = GetTagData(GTA_GadgetKind, 0, msg->ops_AttrList);
	data->arrowtype = arrowtype;
    	data->scroller = (Object *)GetTagData(GTA_Arrow_Scroller, NULL,  msg->ops_AttrList);
    	if (!data->scroller)
     	    goto failure;
     	    
     	data->arrowimage = arrowimage;
     	data->frame      = frame;
    	
    }
    ReturnPtr("Arrow::New", Object *, o);
    
failure:
    if (frame)
    	DisposeObject(frame);
    if (arrowimage)
    	DisposeObject(arrowimage);
    if (o)
    	CoerceMethod(cl, o, OM_DISPOSE);
    
    ReturnPtr("Arrow::New", Object *, NULL);
    
}

/***************************
**  Arrow::HandleInput()  **
***************************/
STATIC IPTR arrow_handleinput(Class *cl, Object *o, struct gpInput *msg)
{
    IPTR retval;
#if 0
    struct InputEvent *ie = msg->gpi_IEvent;
#endif

    EnterFunc(bug("Arrow::HandleInput\n()\n"));
    /* Let superclass tell what it thinks about the event */
    retval = DoSuperMethodA(cl, o, (Msg)msg);
#if 0
    if (ie->ie_Class == IECLASS_RAWMOUSE)
    {
    	if ((ie->ie_Code == SELECTUP) || (ie->ie_Code == MENUDOWN))
    	{
    	    struct ArrowData *data = INST_DATA(cl, o);
    	    
    	    /* Fill in the code field with the current scroller level */
    	    GetAttr(PGA_Top, data->scroller, msg->gpi_Termination);
    	}
    }    
    else if (ie->ie_Class == IECLASS_TIMER)
    {
    	struct ArrowData *data = INST_DATA(cl, o);

	GetAttr(PGA_Top, data->scroller, msg->gpi_Termination);
	retval = GMR_INTERIMUPDATE;
    }
#endif
    ReturnInt ("Arrow::HandleInput", IPTR, retval);
}

AROS_UFH3S(IPTR, dispatch_arrowclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, o, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{

    IPTR retval = 0UL;

    switch (msg->MethodID) {
    case OM_NEW:
	retval = (IPTR)arrow_new(cl, o, (struct opSet *) msg);
	break;

    case OM_GET: {
    	struct ArrowData *data = INST_DATA(cl, o);
	
	switch(OPG(msg)->opg_AttrID)
	{
	    case GTA_GadgetKind:
	    	*(OPG(msg)->opg_Storage) = data->gadgetkind;
		retval = 1UL;
		break;
	
	    case GTA_ChildGadgetKind:
	    	*(OPG(msg)->opg_Storage) = _ARROW_KIND;
		retval = 1UL;
		break;

	    case GTA_Arrow_Type:
	    	*(OPG(msg)->opg_Storage) = data->arrowtype;
		retval = 1UL;
		break;

	    case GTA_Arrow_Scroller:
	    	*(OPG(msg)->opg_Storage) = (IPTR)data->scroller;
		retval = 1UL;
		break;
		
	    default:
	    	retval = DoSuperMethodA(cl, o, msg);
		break;
	}
	} break;
	
    case OM_DISPOSE: {
    	struct ArrowData *data = INST_DATA(cl, o);

   	if (data->frame)
    	    DisposeObject(data->frame);
     	
    	if (data->arrowimage)
    	    DisposeObject(data->arrowimage);
	    
	retval = DoSuperMethodA(cl, o, msg);
   	} break;
    
    case GM_HANDLEINPUT:
    	retval = arrow_handleinput(cl, o, (struct gpInput *)msg);
    	break;

    default:
	retval = DoSuperMethodA(cl, o, msg);
	break;
    }

    return retval;
}


/*********** Scroller class ************/

struct ScrollerData
{
    Object *frame;
    WORD gadgetkind;
    UBYTE labelplace;
};

/**********************
**  Scroller::Set()  **
**********************/

#define opU(x) ((struct opUpdate *)msg)
IPTR scroller_set(Class * cl, Object * o, struct opSet * msg)
{
    IPTR retval = 0UL;
    struct TagItem *tag, *tstate, tags[] =
    {
    	{PGA_Total,	0},
    	{PGA_Top,	0},
    	{PGA_Visible,	0},
    	{TAG_MORE,	(IPTR)NULL}
    };

    struct ScrollerData *data;

    tags[3].ti_Data = (IPTR)msg->ops_AttrList;
    
    /* Get old values */
    DoSuperMethod(cl, o, OM_GET, PGA_Total, 	&(tags[0].ti_Data));
    DoSuperMethod(cl, o, OM_GET, PGA_Top, 	&(tags[1].ti_Data));
    DoSuperMethod(cl, o, OM_GET, PGA_Visible, 	&(tags[2].ti_Data));

    tstate = msg->ops_AttrList;
    
    while ((tag = NextTagItem((const struct TagItem **)&tstate)))
    {
    	
    	switch (tag->ti_Tag)
    	{
	     case GTA_GadgetKind:
	     	data = INST_DATA(cl, o);
		data->gadgetkind = tag->ti_Data;
		break;

    	     case GTSC_Total:
    	     	tags[0].ti_Data  = tag->ti_Data;
    	     	break;

    	     case GTSC_Top:
		tags[1].ti_Data  = tag->ti_Data;
		break;
    	     	
    	     case GTSC_Visible:
            	tags[2].ti_Data  = tag->ti_Data;
            	break;
            	
             case GTA_Scroller_Dec:
#if 0
                /* buttong_class gives -GA_ID if mouse outside arrow */
                if ((tag->ti_Data > 0) && (opU(msg)->opu_Flags & OPUF_INTERIM))
                {
#endif

            	    if (tags[1].ti_Data > 0)
            	    {
            	    	((ULONG)tags[1].ti_Data) --;
            	    	retval = 1UL;
            	    }
#if 0
                }
#endif
                break;
            
            case GTA_Scroller_Inc:
#if 0
                /* buttong_class gives -GA_ID if mouse outside arrow */
                if ((tag->ti_Data > 0) && (opU(msg)->opu_Flags & OPUF_INTERIM))
                {
#endif
            	    /* Top < (Total - Visible) ? */
            	    if (tags[1].ti_Data < (tags[0].ti_Data - tags[2].ti_Data))
            	    {
            	    	((ULONG)tags[1].ti_Data) ++;
           	    	retval = 1UL;
           	    }
#if 0
            	}
#endif
            	break;
            
	     	
    	}
    	
    }

    DoSuperMethod(cl, o, OM_SET, tags, msg->ops_GInfo);

    return (retval);
}



/**********************
**  Scroller::Get()  **
**********************/
IPTR scroller_get(Class * cl, Object * o, struct opGet *msg)
{
    struct ScrollerData *data = INST_DATA(cl, o);
    struct opGet cloned_msg = *msg;
    IPTR retval = 1UL;

    switch (msg->opg_AttrID)
    {
	case GTA_GadgetKind:
	    *msg->opg_Storage = data->gadgetkind;
	    break;
		
	case GTA_ChildGadgetKind:
	    *msg->opg_Storage = SCROLLER_KIND;
	    break;
	
	case GTSC_Top:
	    cloned_msg.opg_AttrID = PGA_Top;
	    retval = DoSuperMethodA(cl, o, (Msg)&cloned_msg);
	    break;
	    
	case GTSC_Total:
	    cloned_msg.opg_AttrID = PGA_Total;
	    retval = DoSuperMethodA(cl, o, (Msg)&cloned_msg);
	    break;
	    
	case GTSC_Visible:
	    cloned_msg.opg_AttrID = PGA_Visible;
	    retval = DoSuperMethodA(cl, o, (Msg)&cloned_msg);
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, o, (Msg)msg);
	    break;
    }
    
    return retval;
}

/**********************
**  Scroller::New()  **
**********************/
Object *scroller_new(Class * cl, Object * o, struct opSet *msg)
{
    struct ScrollerData *data;
    struct DrawInfo *dri;
    struct TagItem fitags[] =
    {
	{IA_Width, 0UL},
	{IA_Height, 0UL},
	{IA_Resolution, 0UL},
	{IA_FrameType, FRAME_BUTTON},
	{IA_EdgesOnly, TRUE},
	{TAG_DONE, 0UL}
    };
    
    EnterFunc(bug("Scroller::New()\n"));
    
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);
    if (o)
    {
    	data = INST_DATA(cl, o);
	
	dri = (struct DrawInfo *)GetTagData(GA_DrawInfo, NULL, msg->ops_AttrList);
	
	fitags[0].ti_Data = GetTagData(GA_Width, 0, msg->ops_AttrList) + BORDERPROPSPACINGX * 2;
	fitags[1].ti_Data = GetTagData(GA_Height, 0, msg->ops_AttrList) + BORDERPROPSPACINGY * 2;
	fitags[2].ti_Data = (dri->dri_Resolution.X << 16) + dri->dri_Resolution.Y;

	data->frame = NewObjectA(NULL, FRAMEICLASS, fitags);
	if (data->frame)
	{
	    scroller_set(cl, o, msg);
	    data->labelplace = GetTagData(GA_LabelPlace, GV_LabelPlace_Left, ((struct opSet *)msg)->ops_AttrList);
	} else {
	    CoerceMethod(cl, o, OM_DISPOSE);
	    o = NULL;
	}
    }
    ReturnPtr("Scroller::New", Object *, o);
    
}

/****************************
**  Scroller::HandleInput()  **
****************************/
STATIC IPTR scroller_handleinput(Class *cl, Object *o, struct gpInput *msg)
{
/*    struct InputEvent *ie = msg->gpi_IEvent;*/
    IPTR retval;

#if 0    
    LONG top1, top2;
    
    EnterFunc(bug("Scroller::HandleInput()\n"));
 
    /* Get the PGA_Top attribute */
    DoSuperMethod(cl, o, OM_GET, PGA_Top, &top1);
#endif
     
    retval = DoSuperMethodA(cl, o, (Msg)msg);

#if 0
    /* now done by GT_FilterImsg */
    /* Mousemove ? */
    if ((ie->ie_Class == IECLASS_RAWMOUSE) && (ie->ie_Code == IECODE_NOBUTTON))
    {
    	DoSuperMethod(cl, o, OM_GET, PGA_Top, &top2);
    	
    	/* PGA_Top changed ? */
    	if (top1 != top2)
    	{
    	    retval = GMR_INTERIMUPDATE;
    	    *(msg->gpi_Termination) = top2;
    	}
    }
#endif
    
    ReturnInt("Scroller::HandleInput", IPTR, retval);
}


/****************************
**  Scroller::Render()     **
****************************/
STATIC IPTR scroller_render(Class *cl, Object *o, struct gpRender *msg)
{
    struct ScrollerData *data;
    IPTR retval;
    
    data = INST_DATA(cl, o);
    
    if (msg->gpr_Redraw == GREDRAW_REDRAW)
    {
	DrawImageState(msg->gpr_RPort,
		(struct Image *)data->frame,
		G(o)->LeftEdge - BORDERPROPSPACINGX,
		G(o)->TopEdge  - BORDERPROPSPACINGY,
		IDS_NORMAL,
		msg->gpr_GInfo->gi_DrInfo);

        renderlabel(GadToolsBase, (struct Gadget *)o, msg->gpr_RPort, data->labelplace);   
    }
    
    retval = DoSuperMethodA(cl, o, (Msg)msg);
    
    ReturnInt("Scroller::Render", IPTR, retval);
}



AROS_UFH3S(IPTR, dispatch_scrollerclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, o, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
    struct ScrollerData *data;
    IPTR retval = 0UL;

    switch (msg->MethodID) {
    case OM_NEW:
	retval = (IPTR) scroller_new(cl, o, (struct opSet *) msg);
	break;

    case OM_DISPOSE:
	data = INST_DATA(cl, o);
	DisposeObject(data->frame);
	retval = DoSuperMethodA(cl, o, msg);
	break;

    case OM_UPDATE:
    case OM_SET:
	retval = scroller_set(cl, o, (struct opSet *) msg);
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
    
    case OM_GET:
	retval = scroller_get(cl, o, (struct opGet *)msg);
    	break;
    
    case GM_HANDLEINPUT:
    	retval = scroller_handleinput(cl, o, (struct gpInput *)msg);
    	break;

    case GM_RENDER:
    	retval = scroller_render(cl, o, (struct gpRender *)msg);
	break;
	    
    default:
	retval = DoSuperMethodA(cl, o, msg);
	break;
    }
    
    return retval;
}

/*************************** STRING_KIND and INTEGER_KIND *****************************/

#ifdef SDEBUG
#   undef SDEBUG
#endif
#ifdef DEBUG
#   undef DEBUG
#endif
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

struct StringData
{
    Object	*frame;
    struct TextFont *font;
    WORD	gadgetkind;
    UBYTE	labelplace;
};
/***********************
**  String::SetNew()  **
***********************/

IPTR string_setnew(Class *cl, Object *o, struct opSet *msg)
{
    struct TagItem *tag, *tstate, tags[] =
    {
    	{TAG_IGNORE,	0UL},  /* 0 STRINGA_TextVal  */
	{TAG_IGNORE,	0UL},  /* 1 STRINGA_LongVal  */
    	{TAG_IGNORE,	0UL},  /* 2 STRINGA_MaxChars */
    	{TAG_IGNORE,	0UL},  /* 3 STRINGA_EditHook */
    	{TAG_MORE,	0UL}
    };
    
    LONG labelplace = GV_LabelPlace_Left;
    struct DrawInfo *dri;
    struct TextAttr *tattr = NULL;
    LONG gadgetkind = STRING_KIND;
    
    IPTR retval = 0UL;

    EnterFunc(bug("String::SetNew()\n"));
    
    tstate = msg->ops_AttrList;
    while ((tag = NextTagItem((const struct TagItem **)&tstate)))
    {
    	IPTR tidata = tag->ti_Data;
    	
    	switch (tag->ti_Tag)
    	{
	    case GTA_GadgetKind:
		gadgetkind = tidata;
	        break;
	    
    	    case GTST_String:
	    	tags[0].ti_Tag = STRINGA_TextVal;
	    	tags[0].ti_Data = tidata;
		break;
		
    	    case GTIN_Number:
	    	tags[1].ti_Tag = STRINGA_LongVal;
	    	tags[1].ti_Data = tidata;
		break;
    	    
    	    /* Another weird inconsistency of AmigaOS GUI objects:
    	    ** For intuition and strgclass gadgets, MaxChars includes trailing
    	    ** zero, but this is NOT true for gadtools string gadgets
    	    */
    	    case GTIN_MaxChars:
    	    case GTST_MaxChars:
	    	tags[2].ti_Tag = STRINGA_MaxChars;
	    	tags[2].ti_Data = ((WORD)tidata) + 1;
		break;
		
/*    	    case GTIN_EditHook:  Duplicate case value */
    	    case GTST_EditHook:
	    	tags[3].ti_Tag = STRINGA_EditHook;
	    	tags[3].ti_Data = tidata;
		break;
    	    
    	    case GA_LabelPlace:
    	    	labelplace = tidata;
    	    	break;
    	    case GA_DrawInfo:
    	    	dri = (struct DrawInfo *)tidata;
    	    	break;
    	    case GA_TextAttr:
    	    	tattr = (struct TextAttr *)tidata;
    	    	break;

    	}
    }
    
    tags[4].ti_Data = (IPTR)msg->ops_AttrList;

    retval = DoSuperMethod(cl, o, msg->MethodID, tags, msg->ops_GInfo);
   
    D(bug("Returned from supermethod: %p\n", retval));
    
    if ((msg->MethodID == OM_NEW) && (retval != 0UL))
    {
    	struct StringData *data = INST_DATA(cl, retval);
    	struct TagItem fitags[] =
    	{
	    {IA_Width		, 0UL		},
	    {IA_Height		, 0UL		},
	    {IA_Resolution	, 0UL		},
	    {IA_FrameType	, FRAME_RIDGE	},
	    {IA_EdgesOnly	, TRUE		},
	    {TAG_DONE		, 0UL		}
    	};
    	
    	fitags[0].ti_Data = G(retval)->Width;
    	fitags[1].ti_Data = G(retval)->Height;
    	fitags[2].ti_Data = (dri->dri_Resolution.X << 16) + dri->dri_Resolution.Y;
    	
    	data->labelplace = labelplace;
    	data->frame = NULL;
    	data->font  = NULL;
	data->gadgetkind = gadgetkind;
	
    	D(bug("Creating frame image"));
    	
    	data->frame = NewObjectA(NULL, FRAMEICLASS, fitags);

    	D(bug("Created frame image: %p", data->frame));
    	if (!data->frame)
    	{
    	    CoerceMethod(cl, (Object *)retval, OM_DISPOSE);
    	    
    	    retval = (IPTR)NULL;
    	}

    	if (tattr)
    	{
    	    data->font = OpenFont(tattr);

    	    if (data->font)
    	    {
    	    	struct TagItem sftags[] = {{STRINGA_Font, (IPTR)NULL}, {TAG_DONE, }};
    	    	
    	    	sftags[0].ti_Data = (IPTR)data->font;

    	    	DoSuperMethod(cl, (Object *)retval, OM_SET, sftags, NULL);
    	    }
    	}
    }
    
    ReturnPtr ("String::SetNew", IPTR, retval);

}

/***********************
**  String::Render()  **
***********************/
#undef G
#define G(o) ((struct Gadget *)o)
STATIC IPTR string_render(Class *cl, Object *o, struct gpRender *msg)
{
    IPTR retval;
    
    EnterFunc(bug("String::Render()\n"));
    retval = DoSuperMethodA(cl, o, (Msg)msg);
    
    D(bug("Superclass render OK\n"));
    
    if (msg->gpr_Redraw == GREDRAW_REDRAW)
    {
    	struct StringData *data = INST_DATA(cl, o);
    	
	WORD x, y;
	    
	struct TagItem itags[] =
	{
	    {IA_Width,	0L},
	    {IA_Height,	0L},
	    {TAG_DONE,}
	};
	
	D(bug("Full redraw\n"));

	/* center image position, we assume image top and left is 0 */
	itags[0].ti_Data = G(o)->Width + BORDERSTRINGSPACINGX * 2;
	itags[1].ti_Data = G(o)->Height + BORDERSTRINGSPACINGY * 2;

	SetAttrsA((Object *)data->frame, itags);
	
	x = G(o)->LeftEdge - BORDERSTRINGSPACINGX; 
	y = G(o)->TopEdge - BORDERSTRINGSPACINGY;
	    
	DrawImageState(msg->gpr_RPort,
		(struct Image *)data->frame,
		x, y,
		IDS_NORMAL,
		msg->gpr_GInfo->gi_DrInfo);
   
   	/* render label */
   	renderlabel(GadToolsBase, (struct Gadget *)o, msg->gpr_RPort, data->labelplace);
   	
    } /* if (whole gadget should be redrawn) */
    
    ReturnInt ("String::Render", IPTR, retval);
}

AROS_UFH3S(IPTR, dispatch_stringclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, o, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{

    IPTR retval = 0UL;

    switch (msg->MethodID) {
    case OM_NEW:
    case OM_SET:
	retval = string_setnew(cl, o, (struct opSet *)msg);
	break;

    case OM_GET:
    {
    	struct StringData *data = INST_DATA(cl, o);
	struct opGet cloned_msg = *(struct opGet *)msg;
	
   	switch(cloned_msg.opg_AttrID)
	{
	    case GTA_GadgetKind:
	    case GTA_ChildGadgetKind:
	    	*(OPG(msg)->opg_Storage) = data->gadgetkind;
		retval = 1UL;
		break;
	    
	    case GTST_String:
	    	cloned_msg.opg_AttrID = STRINGA_TextVal;
		break;
	
	    case GTIN_Number:
	    	cloned_msg.opg_AttrID = STRINGA_LongVal;
		break;
		
	    default:
		break;
	}
	
    	if (!retval) retval = DoSuperMethodA(cl, o, (Msg)&cloned_msg);
	if (cloned_msg.opg_AttrID == STRINGA_LongVal)
	{
	    /* kprintf("********** GT_Get STRINGA_LongVal: --> %d\n",*cloned_msg.opg_Storage); */
	}
    }
    break;
    
    case GM_RENDER:
    	retval = string_render(cl, o, (struct gpRender *)msg);
    	break;
   
    case OM_DISPOSE: {
    	struct StringData *data = INST_DATA(cl, o);
    	if (data->frame)
    	    DisposeObject(data->frame);
    	if (data->font)
    	    CloseFont(data->font);
	
	retval = DoSuperMethodA(cl, o, msg);
    } break;

    default:
	retval = DoSuperMethodA(cl, o, msg);
	break;
    }

    return retval;
}
/*************************** LISTVIEW_KIND *****************************/


#ifdef SDEBUG
#   undef SDEBUG
#endif
#ifdef DEBUG
#   undef DEBUG
#endif
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

struct LVData
{
    struct Hook *ld_CallBack;
    struct List	*ld_Labels;
    struct DrawInfo *ld_Dri;
    struct TextFont *ld_Font;
    Object	*ld_Frame;
    Object	*ld_Scroller;
    WORD	ld_Top;
    WORD	ld_Selected;
    WORD	ld_Spacing;
    WORD	ld_ItemHeight;
    
    /* The number of first damaged entry, counting from first visible.
    ** A value o -1 means "nothing has to be redrawn"
    */
    WORD	ld_FirstDamaged;
    UWORD	ld_NumDamaged;

    /* Number of entries the listview should scroll in GM_RENDER.
    ** Negative valu means scroll up.
    */
    WORD	ld_ScrollEntries;
    WORD	ld_NumEntries;

    UBYTE	ld_Flags;
    UBYTE	ld_LabelPlace;
};

/* This one goes into cl->cl_UserData */
struct StaticLVData
{
    struct GadToolsBase_intern	*ls_GadToolsBase;
    struct Hook	ls_RenderHook;
};

/* Flags */
#define LVFLG_READONLY		(1 << 0)
#define LVFLG_FONT_OPENED	(1 << 1)


/* The flags below are used for as a trick to easily select the right pens,
** or the right hook draw states. The combinations of the flags can be used
** as an index into the table. This saves me from a LOT of if-else statements.
** As one can se, the 4 last ones is all LVR_NORMAL, cause it makes no sense
** to mark entris selected or disabled in a readony listview.
*/

#define NORMAL		0
#define SELECTED	(1 << 0)
#define DISABLED	(1 << 1)
#define READONLY	(1 << 2)

#define TotalItemHeight(data) (data->ld_Spacing + data->ld_ItemHeight)

const ULONG statetab[] = {
    LVR_NORMAL, 	/* 0 NORMAL		*/
    LVR_SELECTED,	/* 1 SELECTED		*/
    LVR_NORMALDISABLED,	/* 2 NORMAL|DISABLED	*/ 
    LVR_SELECTED, 	/* 3 SELECTED|DISABLED	*/ 
    LVR_NORMAL,		/* 4 NORMAL|READONLY	*/
    LVR_NORMAL,		/* 5 SELECTED|READONLY	*/
    LVR_NORMAL,		/* 6 NORMAL|DISABLED|READONLY	*/
    LVR_NORMAL		/* 7 SELECTED|DISABLED|READONLY	*/
};

#undef GadToolsBase
#define GadToolsBase ((struct GadToolsBase_intern *)hook->h_Data)

/******************
**  RenderHook()  **
******************/

AROS_UFH3(IPTR, RenderHook,
    AROS_UFHA(struct Hook *,            hook,     	A0),
    AROS_UFHA(struct Node *,    	node,           A2),
    AROS_UFHA(struct LVDrawMsg *,	msg,	        A1)
)
{
    IPTR retval;
    EnterFunc(bug("RenderHook: hook=%p, node=%sm msg=%p)\n",
    	hook, node->ln_Name, msg));
    
    if (msg->lvdm_MethodID == LV_DRAW)
    {
    	struct DrawInfo *dri = msg->lvdm_DrawInfo;
    	struct RastPort *rp  = msg->lvdm_RastPort;
    	
    	WORD min_x = msg->lvdm_Bounds.MinX;
    	WORD min_y = msg->lvdm_Bounds.MinY;
    	WORD max_x = msg->lvdm_Bounds.MaxX;
    	WORD max_y = msg->lvdm_Bounds.MaxY;

        UWORD erasepen = BACKGROUNDPEN;


     	SetDrMd(rp, JAM1);
     	    
    	
     	switch (msg->lvdm_State)
     	{
     	    case LVR_SELECTED:
     	    case LVR_SELECTEDDISABLED:
     	    	/* We must fill the backgound with FILLPEN */
		erasepen = FILLPEN;

		/* Fall through */
		
     	    case LVR_NORMAL:
     	    case LVR_NORMALDISABLED: {

    	    	WORD numfit;
    	    	struct TextExtent te;
    	    
		SetAPen(rp, dri->dri_Pens[erasepen]);
     	    	RectFill(rp, min_x, min_y, max_x, max_y);
     	    	
    	    	numfit = TextFit(rp, node->ln_Name, strlen(node->ln_Name),
    	    		&te, NULL, 1, max_x - min_x + 1, max_y - min_y + 1);

	    	SetAPen(rp, dri->dri_Pens[TEXTPEN]);
	    	
    	    	/* Render text */
    	    	Move(rp, min_x, min_y + rp->Font->tf_Baseline);
    	    	Text(rp, node->ln_Name, numfit);
	    	
     	    	
     	    } break;
     	    	
     	    default:
     	    	kprintf("!! LISTVIEW DRAWING STATE NOT SUPPORTED !!\n");
     	    	break;
     	
     	}
     	
     	retval = LVCB_OK;
     }
     else
     {
     	retval = LVCB_UNKNOWN;
     }
     	
     ReturnInt ("RenderHook", IPTR, retval);
}

#undef GadToolsBase

/**********************
**  RenderEntries()  **
**********************/
STATIC VOID RenderEntries(Class *cl, Object *o, struct gpRender *msg, 
	WORD entryoffset, UWORD numentries, struct GadToolsBase_intern *GadToolsBase)
{

    /* NOTE: entry is the the number ot the item to draw
    ** counted from first visible
    */
    
    struct LVData *data = INST_DATA(cl, o);
    struct Node *node;
    UWORD entry_count, totalitemheight;
    WORD left, top, width;
    struct LVDrawMsg drawmsg;
    struct TextFont *oldfont;
    
    UBYTE state = 0;
    
    UWORD current_entry = 0;
    
    EnterFunc(bug("RenderEntries(msg=%p, entryoffset=%d, numentries=%d)\n",
    	msg, entryoffset, numentries));
    
    oldfont = msg->gpr_RPort->Font;
    SetFont(msg->gpr_RPort, data->ld_Font);
    
    totalitemheight = TotalItemHeight(data);
    
    left = G(o)->LeftEdge + LV_BORDER_X;
    top  = G(o)->TopEdge + LV_BORDER_Y;
    top += totalitemheight * entryoffset;
    
    width = G(o)->Width - LV_BORDER_X * 2;
    
    if (data->ld_CallBack)
    {
    	drawmsg.lvdm_MethodID = LV_DRAW;
    	drawmsg.lvdm_RastPort = msg->gpr_RPort;
    	drawmsg.lvdm_DrawInfo = data->ld_Dri;
    	drawmsg.lvdm_Bounds.MinX  = left;
    	drawmsg.lvdm_Bounds.MaxX  = left + width - 1;
    	
    }
    
    /* Update the state */
    if (data->ld_Flags & LVFLG_READONLY)
    	state |= READONLY;
    	
    if (G(o)->Flags & GFLG_DISABLED)
    	state |= DISABLED;
     
    /* Find first entry to rerender */
    entry_count = data->ld_Top + entryoffset;
    for (node = data->ld_Labels->lh_Head; node->ln_Succ && entry_count; node = node->ln_Succ)
    {
    	entry_count --;
    	current_entry ++;
    }
    	
    /* Start rerndering entries */
    D(bug("About to render %d nodes\n", numentries));
    
    
    for ( ; node->ln_Succ && numentries; node = node->ln_Succ)
    {
    	ULONG retval;

    	/* update state */
    	if (current_entry == data->ld_Selected)
    	    state |= SELECTED;
    
    	D(bug("Rendering entry %d: node %s\n", current_entry, node->ln_Name));

    	/* Call custom render hook */
    	    
    	/* Here comes the nice part about the state mechanism ! */
    	D(bug("Rendering in state %d\n", state));
    	drawmsg.lvdm_State = statetab[state];
    	    
    	drawmsg.lvdm_Bounds.MinY = top;
    	drawmsg.lvdm_Bounds.MaxY = top + data->ld_ItemHeight - 1;
    	    
    	retval = CallHookPkt( data->ld_CallBack, node, &drawmsg);
    	
    	numentries --;
    	current_entry ++;
    	top += totalitemheight;
    	
    	/* Reset SELECTED bit of state */
    	state &= ~SELECTED;

    }
    
    SetFont(msg->gpr_RPort, oldfont);
    ReturnVoid("RenderEntries");
}

/********************
**  NumItemsFit()  **
********************/

STATIC WORD NumItemsFit(Object *o, struct LVData *data)
{
    /* Returns the number of items that can possibly fit within the list */
    UWORD numfit;
    
    EnterFunc(bug("NumItemsFit(o=%p, data=%p)\n",o, data));
    D(bug("total spacing: %d\n", TotalItemHeight(data) ));
    
    numfit = (G(o)->Height - 2 * LV_BORDER_Y) / 
    			TotalItemHeight(data);
    	
    ReturnInt ("NumItemsFit", UWORD, numfit);
    
}

/*********************
**  ShownEntries()  **
*********************/

STATIC WORD ShownEntries(Object *o, struct LVData *data)
{
    WORD numitemsfit;
    WORD shown;

    EnterFunc(bug("ShownEntries(o=%p, data=%p)\n", o, data));
    
    numitemsfit = NumItemsFit(o, data);
    
    shown = ((data->ld_NumEntries < numitemsfit) ? data->ld_NumEntries : numitemsfit);
    
    ReturnInt("ShownEntries", WORD, shown);
}

/***********************
**  UpdateScroller()  **
***********************/

STATIC VOID UpdateScroller(Object *o, struct LVData *data, struct GadgetInfo *gi, struct GadToolsBase_intern *GadToolsBase)
{
    struct TagItem scrtags[] = 
    {
	{PGA_Top,		0L},
	{PGA_Total,		1L},
	{PGA_Visible, 	1L},
	{TAG_DONE,}
    };

    EnterFunc(bug("UpdateScroller(data=%p, gi=%p\n", data, gi));

    if (data->ld_Scroller)
    {
	if (data->ld_NumEntries > 0)
	{
	    scrtags[0].ti_Data = data->ld_Top;
	    scrtags[1].ti_Data = data->ld_NumEntries;
	    scrtags[2].ti_Data = ShownEntries(o, data);
	}

	if (gi)
    	    SetGadgetAttrsA((struct Gadget *)data->ld_Scroller, gi->gi_Window, NULL, scrtags);
	else
    	    SetAttrsA(data->ld_Scroller, scrtags);
    }
    
    ReturnVoid("UpdateScroller");
}

/**********************
**  ScrollEntries()  **
**********************/

STATIC VOID ScrollEntries(Object *o, struct LVData *data, WORD old_top, WORD new_top,
	struct GadgetInfo *gi, struct GadToolsBase_intern *GadToolsBase)
{
    EnterFunc(bug("ScrollEntries(new_tio=%d, gi=%p)\n", new_top, gi));

    /* Tries to scroll the listiew to the new top */
    if (gi) /* Sanity check */
    {
    	UWORD abs_steps;
    	ULONG redraw_type;
    	struct RastPort *rp;
    	
    	data->ld_ScrollEntries = new_top - old_top;
    	abs_steps = abs(data->ld_ScrollEntries);
    	
	/* We do a scroll only if less than half of the visible area
	** is to be scrolled
	*/
    	
    	if (abs_steps < (NumItemsFit(o, data) >> 1))
    	{
    	    redraw_type = GREDRAW_UPDATE;
    	}
    	else
    	{
    	    redraw_type = GREDRAW_REDRAW;
    	    
    	    data->ld_ScrollEntries = 0;
    	    
    	}
    	
    	
    	if ( (rp = ObtainGIRPort(gi)) )
    	{
    	    DoMethod(o, GM_RENDER, gi, rp, redraw_type);
    	    
    	    ReleaseGIRPort(rp);
    	}
    	
    }
    	
    ReturnVoid("ScrollEntries");
}

#define lvS(x) ((struct StaticLVData *)x)
#define GadToolsBase ((struct GadToolsBase_intern *)lvS(cl->cl_UserData)->ls_GadToolsBase)



/**********************
**  Listview::Set()  **
**********************/

STATIC IPTR listview_set(Class *cl, Object *o,struct opSet *msg)
{
    IPTR retval = 0UL;

    struct TagItem *tag, *tstate;
    struct LVData *data = INST_DATA(cl, o);
    struct RastPort *rp;
    
    BOOL labels_set = FALSE;
    BOOL update_scroller = FALSE;
    BOOL scroll_entries = FALSE;
    BOOL refresh_all = FALSE;
    
    WORD new_top, old_top;
    
    EnterFunc(bug("Listview::Set()\n"));

    tstate = msg->ops_AttrList;
    while ((tag = NextTagItem((const struct TagItem **)&tstate)) != NULL)
    {
    	IPTR tidata = tag->ti_Data;
    	
	switch (tag->ti_Tag)
	{
	    case GTA_Listview_Scroller:	/* [IS] */
	    	data->ld_Scroller = (Object *)tidata;
	    	update_scroller = TRUE;
	    	break;
	    	
	    case GTLV_Top:	/* [IS]	*/
	    case GTLV_MakeVisible: /* [IS] */
	    	old_top = data->ld_Top;
	    	new_top = (WORD)tidata;
	    	data->ld_Top = new_top;
	    	
	    	update_scroller = TRUE;
	    	scroll_entries = TRUE;
	    	
	    	retval = 1UL;
	    	break;

	    case GTLV_Labels:	/* [IS] */
	    	data->ld_Labels = (struct List *)tidata;
	    	retval = 1UL;
	    	labels_set = TRUE;
    		update_scroller = TRUE;
		refresh_all = TRUE;
	    	break;
	    	
	    case GTLV_ReadOnly:	/* [I] */
	    	if (tidata)
	    	    data->ld_Flags |= LVFLG_READONLY;
	    	else
	    	    data->ld_Flags &= ~LVFLG_READONLY;
	    	    
	    	D(bug("Readonly: tidata=%d, flags=%d\n",
	    		tidata, data->ld_Flags));
	    	break;
	    	
	    case GTLV_Selected:	/* [IS] */
	    	data->ld_Selected = (UWORD)tidata;
		
		#warning changing GTLV_Selected should not rerender everything
		
		refresh_all = TRUE;
	    	retval = 1UL;
	    	break;
	    	
	    case LAYOUTA_Spacing:	/* [I] */
	    	data->ld_Spacing = (UWORD)tidata;
	    	break;
	    	
	    case GTLV_ItemHeight:	/* [I] */
	    	data->ld_ItemHeight = (UWORD)tidata;
	    	break;
	    	
	    case GTLV_CallBack:	/* [I] */
	    	data->ld_CallBack = (struct Hook *)tidata;
	    	break;
	    	
	    case GA_LabelPlace: /* [I] */
	    	data->ld_LabelPlace = (LONG)tidata;
	    	break;
	    	
	} /* switch (tag->ti_Tag) */

    } /* while (more tags to iterate) */
    
    if (labels_set)
    {
    	struct Node *n;
    	
    	data->ld_NumEntries = 0;

	if (data->ld_Labels)
	{
    	    /* Update the labelcount */
    	    ForeachNode(data->ld_Labels, n)
    	    {
    		data->ld_NumEntries ++;
    	    }
	}
    	D(bug("Number of items added: %d\n", data->ld_NumEntries));
    }

    if (update_scroller)
    {
    	/* IMPORTANT! If this is an OM_UPDATE, we should NOT redraw the
	** set the scroller, as we the scroller has allready been updated
    	*/
    	if (msg->MethodID != OM_UPDATE)
    	    UpdateScroller(o, data, msg->ops_GInfo, GadToolsBase);
    }
   
    if (scroll_entries && !refresh_all)
    {
    	ScrollEntries(o, data, old_top, new_top, msg->ops_GInfo, GadToolsBase);
    }
    
    if (refresh_all && msg->ops_GInfo)
    {
    	if ((rp = ObtainGIRPort(msg->ops_GInfo)))
	{
   	    DoMethod(o, GM_RENDER, msg->ops_GInfo, rp, GREDRAW_REDRAW);
 
	    ReleaseGIRPort(rp);
	}
    }

    ReturnInt ("Listview::Set", IPTR, retval);
}

/**********************
**  Listview::New()  **
**********************/
STATIC Object *listview_new(Class *cl, Object *o, struct opSet *msg)
{
    struct DrawInfo *dri;
    
    EnterFunc(bug("Listview::New()\n"));

    dri = (struct DrawInfo *)GetTagData(GA_DrawInfo, NULL, msg->ops_AttrList);
    if (dri == NULL)
    	ReturnPtr ("Listview::New", Object *, NULL);
    	
    D(bug("Got dri: %p, dri font=%p, size=%d\n", dri, dri->dri_Font, dri->dri_Font->tf_YSize));
    
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);

    if (o != NULL)
    {
	struct LVData *data = INST_DATA(cl, o);
	struct TextAttr *tattr;
	
	struct TagItem fitags[] =
	{
	    {IA_Width, 0UL},
	    {IA_Height, 0UL},
	    {IA_Resolution, 0UL},
	    {IA_FrameType, FRAME_BUTTON},
	    {IA_EdgesOnly, TRUE},
	    {TAG_DONE, 0UL}
	};

	memset(data, 0, sizeof (struct LVData));

	/* Create a frame for the listview */
    	data->ld_Frame = NewObjectA(NULL, FRAMEICLASS, fitags);
    	if (!data->ld_Frame)
    	{
    	    CoerceMethod(cl, (Object *)o, OM_DISPOSE);
    	    o = (IPTR)NULL;
    	}
    	else
    	{

	    data->ld_Dri = dri;

	    /* Set some defaults */
	    tattr = (struct TextAttr *)GetTagData(GA_TextAttr, NULL, msg->ops_AttrList);
	    if (tattr)
	    {
	    	data->ld_Font = OpenFont(tattr);
	    	if (data->ld_Font)
	    	{
	    	    data->ld_Flags |= LVFLG_FONT_OPENED;
	    	}
	    }


	    if (!data->ld_Font)
	    	data->ld_Font = dri->dri_Font;
	    
	    data->ld_ItemHeight = data->ld_Font->tf_YSize;
	    data->ld_LabelPlace = GV_LabelPlace_Above;
	    data->ld_Spacing = LV_DEF_INTERNAL_SPACING;
	    
	    /* default render hook */
	    data->ld_CallBack = 
	    	&(((struct StaticLVData *)cl->cl_UserData)->ls_RenderHook);
	
	    listview_set(cl, o, msg);

	} /* if (frame created) */

    } /* if (object created) */

    ReturnPtr ("Listview::New", Object *, o);
}

/**********************
**  Listview::Get()  **
**********************/
STATIC IPTR listview_get(Class *cl, Object *o, struct opGet *msg)
{
    IPTR retval = 1UL;
    struct LVData *data;

    data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
    	case GTA_GadgetKind:
	case GTA_ChildGadgetKind:
	    *(msg->opg_Storage) = LISTVIEW_KIND;
	    break;

	case GTLV_Top:
	    *(msg->opg_Storage) = (IPTR)data->ld_Top;
	    break;

	case GTLV_Selected:
	    *(msg->opg_Storage) = (IPTR)data->ld_Selected;
	    break;

	case GTLV_Labels:
	    *(msg->opg_Storage) = (IPTR)data->ld_Labels;
	    break;

	default:
	    retval = DoSuperMethodA(cl, o, (Msg)msg);
	    break;
    }
    return (retval);
}


/*************************
**  Lisview::Dispose()  **
*************************/
STATIC VOID listview_dispose(Class *cl, Object *o, Msg msg)
{
    struct LVData *data = INST_DATA(cl, o);

    if (data->ld_Flags & LVFLG_FONT_OPENED)
	CloseFont(data->ld_Font);

    DoSuperMethodA(cl, o, msg);
}

/**************************
**  Lisview::GoActive()  **
**************************/
STATIC IPTR listview_input(Class *cl, Object *o, struct gpInput *msg)
{
    struct LVData *data = INST_DATA(cl, o);
    WORD clickpos;    
    BOOL shown;
    
    EnterFunc(bug("Listview::GoActive()\n"));

    if (msg->MethodID == GM_GOACTIVE)
    {
	if (!msg->gpi_IEvent) /* Not activated ny user ? */
    	    ReturnInt("Listview::GoActive", IPTR, GMR_NOREUSE);    

	if (data->ld_Flags & LVFLG_READONLY)
    	    ReturnInt("Listview::GoActive", IPTR, GMR_NOREUSE);
    }	
    
    /* How many entries are currently shown in the Gtlv ? */
    shown = ShownEntries(o, data);


    if (msg->gpi_IEvent->ie_Class == IECLASS_RAWMOUSE)
    {
        if ((msg->gpi_IEvent->ie_Code == SELECTDOWN) ||
	    (msg->gpi_IEvent->ie_Code == IECODE_NOBUTTON))
	{
	    /* offset from top of listview of the entry clicked */
	    clickpos = (msg->gpi_Mouse.Y - LV_BORDER_Y) / 
    			TotalItemHeight(data);

	    if (clickpos < 0)
	    {
	        if (data->ld_Top > 0)
		{
	            struct TagItem set_tags[] =
		    {
			 {GTLV_Top, data->ld_Top - 1},
			 {TAG_DONE		    }
		    };

		    DoMethod(o, OM_SET, set_tags, msg->gpi_GInfo);		    
		}
		
	        clickpos = 0;
		
	    } else if (clickpos >= shown)
	    {
	        WORD max_top = data->ld_NumEntries - NumItemsFit(o, data);
		
		if (max_top < 0) max_top = 0;
		
	    	if (data->ld_Top < max_top)
		{
	            struct TagItem set_tags[] =
		    {
			 {GTLV_Top, data->ld_Top + 1},
			 {TAG_DONE		    }
		    };

		    DoMethod(o, OM_SET, set_tags, msg->gpi_GInfo);		    		
		}
		
	        clickpos = shown - 1;
	    }
	    
	    
	    if ((clickpos >= 0) && (clickpos < shown))
	    {
    		if (clickpos + data->ld_Top != data->ld_Selected)
    		{
		    struct RastPort *rp;
		    WORD oldpos = data->ld_Selected;
		    data->ld_Selected = clickpos + data->ld_Top;

		    rp = ObtainGIRPort(msg->gpi_GInfo);
		    if (rp)
		    {
	    		/* Rerender new active */
	    		data->ld_FirstDamaged = clickpos;
	    		data->ld_NumDamaged = 1;

			DoMethod(o, GM_RENDER, msg->gpi_GInfo, rp, GREDRAW_UPDATE);

			/* Rerender old active if it was shown in the listview */
			if (    (oldpos >= data->ld_Top) 
			     && (oldpos < data->ld_Top + NumItemsFit(o, data)) )
			{

	    		    data->ld_FirstDamaged = oldpos - data->ld_Top;
	    		    data->ld_NumDamaged = 1;

			    DoMethod(o, GM_RENDER, msg->gpi_GInfo, rp, GREDRAW_UPDATE);
			}

			ReleaseGIRPort(rp);

			ReturnInt ("ListView::Input", IPTR, GMR_MEACTIVE);
		    }

		} /* if (click wasn't on old active item) */

	    } /* if (entry is shown) */

        } /* if mouse down or mouse move event */
	else if (msg->gpi_IEvent->ie_Code == SELECTUP)
	{
	    *(msg->gpi_Termination) = data->ld_Selected;	    
	    ReturnInt ("ListView::Input", IPTR, GMR_VERIFY | GMR_NOREUSE);	    
	} /* mouse up event */
	 	 
    } /* if (is mouse event) */
    
    ReturnInt ("Listview::Input", IPTR, GMR_MEACTIVE);
}


/*************************
**  Listview::Render()  **
*************************/

STATIC IPTR listview_render(Class *cl, Object *o, struct gpRender *msg)
{
    struct LVData *data = INST_DATA(cl, o);
    
    EnterFunc(bug("Listview::Render()\n"));

    switch (msg->gpr_Redraw)
    {
	case GREDRAW_REDRAW: {

	    WORD x, y;
	    struct TagItem itags[] =
	    {
	    	{IA_Width,	0L},
	    	{IA_Height,	0L},
	    	{TAG_DONE,}
	    };
	
	    D(bug("GREDRAW_REDRAW\n"));

	     /* Erase the old gadget imagery */
	    SetAPen(msg->gpr_RPort, data->ld_Dri->dri_Pens[BACKGROUNDPEN]);


	    RectFill(msg->gpr_RPort,
		G(o)->LeftEdge,
		G(o)->TopEdge,
		G(o)->LeftEdge + G(o)->Width  - 1,
		G(o)->TopEdge  + G(o)->Height - 1);

	    if (data->ld_Labels)
	    {
	    	RenderEntries(cl, o, msg,
			0, ShownEntries(o, data),
			GadToolsBase);
	    }
	    
	    /* center image position, we assume image top and left is 0 */
	    itags[0].ti_Data = G(o)->Width;
	    itags[1].ti_Data = G(o)->Height;
	
	    SetAttrsA((Object *)data->ld_Frame, itags);
	

	    x = G(o)->LeftEdge; 
	    y = G(o)->TopEdge;
	    
	    DrawImageState(msg->gpr_RPort,
		(struct Image *)data->ld_Frame,
		x, y,
		((data->ld_Flags & LVFLG_READONLY) ? IDS_SELECTED : IDS_NORMAL),
		msg->gpr_GInfo->gi_DrInfo);
		
	    /* Render gadget label */
	    renderlabel(GadToolsBase, (struct Gadget *)o, msg->gpr_RPort, data->ld_LabelPlace);

	} break;

	case GREDRAW_UPDATE:

	    D(bug("GREDRAW_UPDATE\n"));
	
	    /* Should we scroll the listview ? */
	    if (data->ld_ScrollEntries)
	    {
	    	UWORD abs_steps, visible;
		LONG dy;
		
	    	abs_steps = abs(data->ld_ScrollEntries);
	    	visible = NumItemsFit(o, data);
		
		/* We make the assumption that the listview
		** is alvays 'full'. If it isn't, the
		** Scroll gadget won't be scrollable, and
		** we won't receive any OM_UPDATEs.
		*/

		dy = data->ld_ScrollEntries * TotalItemHeight(data);
		
		D(bug("Scrolling delta y: %d\n", dy));

		ScrollRaster(msg->gpr_RPort, 0, dy,
			G(o)->LeftEdge + LV_BORDER_X,
			G(o)->TopEdge  + LV_BORDER_Y,
			G(o)->LeftEdge + G(o)->Width  - 1 - LV_BORDER_X,
			G(o)->TopEdge  + G(o)->Height - 1 - LV_BORDER_Y);

		data->ld_FirstDamaged = ((data->ld_ScrollEntries > 0) ?
				visible - abs_steps : 0);

		data->ld_NumDamaged = abs_steps;

	    	data->ld_ScrollEntries = 0;
	    	
	    } /* If (we should do a scroll) */
	    
	    D(bug("Rerendering entries: first damaged=%d, num=%d\n",
	    	data->ld_FirstDamaged, data->ld_NumDamaged));
	    
	    /* Redraw all damaged entries */
	    if (data->ld_FirstDamaged != -1)
	    {

		RenderEntries(cl, o, msg,
			data->ld_FirstDamaged, 
			data->ld_NumDamaged,
			GadToolsBase);
			
		data->ld_FirstDamaged = -1;

	    }

	    break; /* GREDRAW_UPDATE */

    } /* switch (render mode) */

    ReturnInt ("Listview::Render", IPTR, 1UL);
}


/*****************
**  Dispatcher	**
*****************/

AROS_UFH3S(IPTR, dispatch_listviewclass,
    AROS_UFHA(Class *,  cl,  A0),
    AROS_UFHA(Object *, o,   A2),
    AROS_UFHA(Msg,      msg, A1)
)
{
    IPTR retval = 0UL;

    switch(msg->MethodID)
    {
	case GM_RENDER:
	    retval = listview_render(cl, o, (struct gpRender *)msg);
	    break;

	case GM_GOACTIVE:
	case GM_HANDLEINPUT:
	    retval = listview_input(cl, o, (struct gpInput *)msg);
	    break;

	case OM_NEW:
	    retval = (IPTR)listview_new(cl, o, (struct opSet *)msg);
	    break;

	case OM_UPDATE: {
	
	#define opS(x) ((struct opSet *)x)
	LONG top;
	top = GetTagData(GTLV_Top, 148, opS(msg)->ops_AttrList);
	D(bug("Received OM_UPDATE: top=%d, attrs=%p, gi=%p\n",
		top, opS(msg)->ops_AttrList, opS(msg)->ops_GInfo));
	}
	case OM_SET:

	    retval = DoSuperMethodA(cl, o, msg);
	    retval += (IPTR)listview_set(cl, o, (struct opSet *)msg);
	    break;

	case OM_GET:
	    retval = (IPTR)listview_get(cl, o, (struct opGet *)msg);
	    break;

	case OM_DISPOSE:
	    listview_dispose(cl, o, msg);
	    break;

	default:
	    retval = DoSuperMethodA(cl, o, msg);
	    break;
    } /* switch */

    return (retval);
}  /* dispatch_Gtlvclass */


#undef GadToolsBase

#define GadToolsBase ((struct GadToolsBase_intern *)cl->cl_UserData)

/*************************** CHECKBOX_KIND *****************************/

struct CheckBoxData {
    UBYTE dummy;
};

AROS_UFH3S(IPTR, dispatch_checkboxclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, obj, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
    IPTR retval = 0UL;

    switch (msg->MethodID) {
    case OM_GET:
	switch (OPG(msg)->opg_AttrID)
	{
	    case GTA_GadgetKind:
	    case GTA_ChildGadgetKind:
	    	*(OPG(msg)->opg_Storage) = CHECKBOX_KIND;
	    	retval = 1UL;
		break;
		
	    default:
	    	retval = DoSuperMethodA(cl, obj, msg);
		break;
	}
	break;

    default:
	retval = DoSuperMethodA(cl, obj, msg);
	break;
    }

    return retval;
}


/*************************** CYCLE_KIND *****************************/

struct CycleData {
    UBYTE labelplace;
};


/*************************
**  Cycle::Render()     **
*************************/
STATIC IPTR cycle_render(Class *cl, Object *o, struct gpRender *msg)
{
    struct CycleData *data;
    IPTR retval;
    
    data = INST_DATA(cl, o);
    
    if (msg->gpr_Redraw == GREDRAW_REDRAW)
    {
       renderlabel(GadToolsBase, (struct Gadget *)o, msg->gpr_RPort, data->labelplace);
    }
    
    retval = DoSuperMethodA(cl, o, (Msg)msg);
    
    ReturnInt("Cycle::Render", IPTR, retval);
}


AROS_UFH3S(IPTR, dispatch_cycleclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, obj, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
    IPTR retval = 0UL;
	    
    switch (msg->MethodID) {
    case OM_NEW:
    	if ((retval = DoSuperMethodA(cl, obj, msg)))
	{
	    struct CycleData *data;

	    data = INST_DATA(cl, retval);	    
	    data->labelplace = GetTagData(GA_LabelPlace, GV_LabelPlace_Left, ((struct opSet *)msg)->ops_AttrList);
	}
	break;
	
    case OM_GET:
	switch (OPG(msg)->opg_AttrID)
	{
	    case GTA_GadgetKind:
	    case GTA_ChildGadgetKind:
	    	*(OPG(msg)->opg_Storage) = CYCLE_KIND;
	    	retval = 1UL;
		break;
		
	    default:
	    	retval = DoSuperMethodA(cl, obj, msg);
		break;
	}
	break;

    case GM_RENDER:
    	retval = cycle_render(cl, obj, (struct gpRender *)msg);
	break;
	
    default:
	retval = DoSuperMethodA(cl, obj, msg);
	break;
    }

    return retval;
}

/*************************** MX_KIND *****************************/

struct MXData {
    UBYTE dummy;
};

AROS_UFH3S(IPTR, dispatch_mxclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, obj, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
    IPTR retval = 0UL;

    switch (msg->MethodID) {
    case OM_GET:
	switch (OPG(msg)->opg_AttrID)
	{
	    case GTA_GadgetKind:
	    case GTA_ChildGadgetKind:
	    	*(OPG(msg)->opg_Storage) = MX_KIND;
	    	retval = 1UL;
		break;
		
	    default:
	    	retval = DoSuperMethodA(cl, obj, msg);
		break;
	}
	break;

    default:
	retval = DoSuperMethodA(cl, obj, msg);
	break;
    }

    return retval;
}

/*************************** PALETTE_KIND *****************************/

struct PaletteData {
    UBYTE dummy;
};

AROS_UFH3S(IPTR, dispatch_paletteclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, obj, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
    IPTR retval = 0UL;

    switch (msg->MethodID) {
    case OM_GET:
	switch (OPG(msg)->opg_AttrID)
	{
	    case GTA_GadgetKind:
	    case GTA_ChildGadgetKind:
	    	*(OPG(msg)->opg_Storage) = PALETTE_KIND;
	    	retval = 1UL;
		break;
		
	    default:
	    	retval = DoSuperMethodA(cl, obj, msg);
		break;
	}
	break;

    default:
	retval = DoSuperMethodA(cl, obj, msg);
	break;
    }

    return retval;
}


/*************************** Classes *****************************/

#undef GadToolsBase

Class *makebuttonclass(struct GadToolsBase_intern * GadToolsBase)
{
    Class *cl;

    ObtainSemaphore(&GadToolsBase->classsema);

    cl = GadToolsBase->buttonclass;
    if (!cl)
    {	
	cl = MakeClass(NULL, FRBUTTONCLASS, NULL, sizeof(struct ButtonData), 0UL);
	if (cl)
	{
	    cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_buttonclass);
	    cl->cl_Dispatcher.h_SubEntry = NULL;
	    cl->cl_UserData = (IPTR) GadToolsBase;

	    GadToolsBase->buttonclass = cl;
	}
    }
    
    ReleaseSemaphore(&GadToolsBase->classsema);

    return cl;
}

/***************************************************************************************************/

Class *maketextclass(struct GadToolsBase_intern * GadToolsBase)
{
    Class *cl;

    ObtainSemaphore(&GadToolsBase->classsema);

    cl = GadToolsBase->textclass;
    if (!cl)
    {
	cl = MakeClass(NULL, GADGETCLASS, NULL, sizeof(struct TextData), 0UL);
	if (cl)
	{
	    cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_textclass);
	    cl->cl_Dispatcher.h_SubEntry = NULL;
	    cl->cl_UserData = (IPTR) GadToolsBase;

	    GadToolsBase->textclass = cl;
	}
    }
    
    ReleaseSemaphore(&GadToolsBase->classsema);

    return cl;
}

/***************************************************************************************************/

Class *makesliderclass(struct GadToolsBase_intern * GadToolsBase)
{
    Class *cl;

    ObtainSemaphore(&GadToolsBase->classsema);

    cl = GadToolsBase->sliderclass;
    if (!cl)
    {
	cl = MakeClass(NULL, PROPGCLASS, NULL, sizeof(struct SliderData), 0UL);
	if (cl)
	{
	    cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_sliderclass);
	    cl->cl_Dispatcher.h_SubEntry = NULL;
	    cl->cl_UserData = (IPTR) GadToolsBase;

	    GadToolsBase->sliderclass = cl;
	}
    }
    
    ReleaseSemaphore(&GadToolsBase->classsema);

    return cl;
}

/***************************************************************************************************/

Class *makescrollerclass(struct GadToolsBase_intern * GadToolsBase)
{
    Class *cl;

    ObtainSemaphore(&GadToolsBase->classsema);

    cl = GadToolsBase->scrollerclass;
    if (!cl)
    {
	cl = MakeClass(NULL, PROPGCLASS, NULL, sizeof(struct ScrollerData), 0UL);
	if (cl)
	{
	    cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_scrollerclass);
	    cl->cl_Dispatcher.h_SubEntry = NULL;
	    cl->cl_UserData = (IPTR) GadToolsBase;

	    GadToolsBase->scrollerclass = cl;
	}
    }
    
    ReleaseSemaphore(&GadToolsBase->classsema);

    return cl;
}

/***************************************************************************************************/

Class *makearrowclass(struct GadToolsBase_intern * GadToolsBase)
{
    Class *cl;

    ObtainSemaphore(&GadToolsBase->classsema);

    cl = GadToolsBase->arrowclass;
    if (!cl)
    {
	cl = MakeClass(NULL, FRBUTTONCLASS, NULL, sizeof(struct ArrowData), 0UL);
	if (cl)
	{
	    cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_arrowclass);
	    cl->cl_Dispatcher.h_SubEntry = NULL;
	    cl->cl_UserData = (IPTR) GadToolsBase;

	    GadToolsBase->arrowclass = cl;
	}
    }
    
    ReleaseSemaphore(&GadToolsBase->classsema);

    return cl;
}

/***************************************************************************************************/

Class *makestringclass(struct GadToolsBase_intern * GadToolsBase)
{
    Class *cl;

    ObtainSemaphore(&GadToolsBase->classsema);
    
    cl = GadToolsBase->stringclass;
    if (!cl)
    {
	cl = MakeClass(NULL, STRGCLASS, NULL, sizeof(struct StringData), 0UL);
	if (cl)
	{
	    cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_stringclass);
	    cl->cl_Dispatcher.h_SubEntry = NULL;
	    cl->cl_UserData = (IPTR) GadToolsBase;

	    GadToolsBase->stringclass = cl;
	}
    }
    
    ReleaseSemaphore(&GadToolsBase->classsema);

    return cl;
}

/***************************************************************************************************/

Class *makelistviewclass(struct GadToolsBase_intern * GadToolsBase)
{
    Class *cl;
    
    ObtainSemaphore(&GadToolsBase->classsema);

    cl = GadToolsBase->listviewclass;
    if (!cl)
    {
        struct StaticLVData *ls;
	
	ls = AllocMem(sizeof (struct StaticLVData), MEMF_ANY);
	if (ls)
	{
   	    cl = MakeClass(NULL, GADGETCLASS, NULL, sizeof(struct LVData), 0UL);
   	    if (cl)
   	    {
    		cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_listviewclass);
    		cl->cl_Dispatcher.h_SubEntry = NULL;

    		/* Initalize ststic hook */
    		ls->ls_GadToolsBase = GadToolsBase;
    		ls->ls_RenderHook.h_Entry = (APTR) AROS_ASMSYMNAME(RenderHook);
    		ls->ls_RenderHook.h_SubEntry = NULL;
    		ls->ls_RenderHook.h_Data = (APTR)GadToolsBase;

    		cl->cl_UserData = (IPTR) ls;

    		GadToolsBase->listviewclass = cl;
    	    } else {
    	        FreeMem(ls, sizeof (struct StaticLVData));
	    }
	}
    }
    
    ReleaseSemaphore(&GadToolsBase->classsema);
    
    return cl;
}

VOID freelistviewclass(Class *cl, struct GadToolsBase_intern *GadToolsBase)
{
    FreeMem((APTR)cl->cl_UserData, sizeof (struct StaticLVData));
    FreeClass(cl);
}

/***************************************************************************************************/

Class *makecheckboxclass(struct GadToolsBase_intern * GadToolsBase)
{
    Class *cl;

    ObtainSemaphore(&GadToolsBase->classsema);

    cl = GadToolsBase->checkboxclass;
    if (!cl)
    {
	if (!GadToolsBase->aroscbbase)
            GadToolsBase->aroscbbase = OpenLibrary(AROSCHECKBOXNAME, 0);

	if (GadToolsBase->aroscbbase)
        {
	    cl = MakeClass(NULL, AROSCHECKBOXCLASS, NULL, sizeof(struct CheckBoxData), 0UL);
	    if (cl)
	    {
		cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_checkboxclass);
		cl->cl_Dispatcher.h_SubEntry = NULL;
		cl->cl_UserData = (IPTR) GadToolsBase;

		GadToolsBase->checkboxclass = cl;
	    }
	}
    }
    
    ReleaseSemaphore(&GadToolsBase->classsema);
  
    return cl;
}

/***************************************************************************************************/

Class *makecycleclass(struct GadToolsBase_intern * GadToolsBase)
{
    Class *cl;

    ObtainSemaphore(&GadToolsBase->classsema);

    cl = GadToolsBase->cycleclass;
    if (!cl)
    {
	if (!GadToolsBase->aroscybase)
            GadToolsBase->aroscybase = OpenLibrary(AROSCYCLENAME, 0);
	
	if (GadToolsBase->aroscybase)
	{
	    cl = MakeClass(NULL, AROSCYCLECLASS, NULL, sizeof(struct CycleData), 0UL);
	    if (cl)
	    {
		cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_cycleclass);
		cl->cl_Dispatcher.h_SubEntry = NULL;
		cl->cl_UserData = (IPTR) GadToolsBase;

		GadToolsBase->cycleclass = cl;
	    }
	}
    }
    
    ReleaseSemaphore(&GadToolsBase->classsema);
  
    return cl;
}

/***************************************************************************************************/

Class *makemxclass(struct GadToolsBase_intern * GadToolsBase)
{
    Class *cl;

    ObtainSemaphore(&GadToolsBase->classsema);

    cl = GadToolsBase->mxclass;
    if (!cl)
    {
	if (!GadToolsBase->arosmxbase)
            GadToolsBase->arosmxbase = OpenLibrary(AROSMXNAME, 0);

	if (GadToolsBase->arosmxbase)
	{
	    cl = MakeClass(NULL, AROSMXCLASS, NULL, sizeof(struct MXData), 0UL);
	    if (cl)
	    {
		cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_mxclass);
		cl->cl_Dispatcher.h_SubEntry = NULL;
		cl->cl_UserData = (IPTR) GadToolsBase;

		GadToolsBase->mxclass = cl;
	    }
	}
    }
    
    ReleaseSemaphore(&GadToolsBase->classsema);
  
    return cl;
}

/***************************************************************************************************/

Class *makepaletteclass(struct GadToolsBase_intern * GadToolsBase)
{
    Class *cl;

    ObtainSemaphore(&GadToolsBase->classsema);

    cl = GadToolsBase->paletteclass;
    if (!cl)
    {
	if (!GadToolsBase->arospabase)
            GadToolsBase->arospabase = OpenLibrary(AROSPALETTENAME, 0);

	if (GadToolsBase->arospabase)
	{
	    cl = MakeClass(NULL, AROSPALETTECLASS, NULL, sizeof(struct PaletteData), 0UL);
	    if (cl)
	    {
		cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_paletteclass);
		cl->cl_Dispatcher.h_SubEntry = NULL;
		cl->cl_UserData = (IPTR) GadToolsBase;

		GadToolsBase->paletteclass = cl;
	    }
	}
    }
    
    ReleaseSemaphore(&GadToolsBase->classsema);
    
    return cl;
}

/***************************************************************************************************/
