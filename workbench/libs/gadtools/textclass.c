/*
   (C) 1997 AROS - The Amiga Research OS
   $Id$

   Desc: Internal GadTools text class (NUMERIC_KIND and TEXT_KIND) .
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

#include <string.h> /* memset() */

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#include "gadtools_intern.h"

/**********************************************************************************************/

#define G(x) ((struct Gadget *)(x))
#define EG(X) ((struct ExtGadget *)(x))

#define GadToolsBase ((struct GadToolsBase_intern *)cl->cl_UserData)

/**********************************************************************************************/

#define TEXTF_CLIPPED	(1 << 0)
#define TEXTF_BORDER	(1 << 1)
#define TEXTF_COPYTEXT	(1 << 2)

struct TextData
{
    struct DrawInfo 	*dri;
    Object 		*frame;    
    STRPTR 		format;
    IPTR 		toprint;
    UBYTE 		frontpen;
    UBYTE 		backpen;
    UBYTE 		justification;
    UBYTE 		flags;
    struct TextFont 	*font;
    UWORD 		maxnumberlength;
    WORD 		gadgetkind;
    LONG  		(*dispfunc)(struct Gadget *, WORD);
    UBYTE 		labelplace;
    
};

/**********************************************************************************************/

STATIC IPTR text_set(Class * cl, Object * o, struct opSet * msg)
{
    IPTR 		retval = 0UL;
    struct TagItem 	*tag, *tstate;
    struct TextData 	*data = INST_DATA(cl, o);
    struct RastPort 	*rport;
    
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

/**********************************************************************************************/

STATIC IPTR text_get(Class * cl, Object * o, struct opGet *msg)
{
    struct TextData 	*data = INST_DATA(cl, o);
    IPTR		retval;
    
    switch (msg->opg_AttrID)
    {
	case GTA_GadgetKind:
	case GTA_ChildGadgetKind:
	    *(msg->opg_Storage) = data->gadgetkind;
	    retval = 1UL;
	    break;

	default:
	    retval = DoSuperMethodA(cl, o, (Msg)msg);
	    break;
    }
    
    return retval;
}

/**********************************************************************************************/

STATIC IPTR text_new(Class * cl, Object * o, struct opSet *msg)
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
    ReturnPtr ("Text::New", IPTR, (IPTR)o);
    
error:
    CoerceMethod(cl, o, OM_DISPOSE);
    ReturnPtr ("Text::New", IPTR, NULL);
}

/**********************************************************************************************/

#define HBORDER 2
#define VBORDER 2

AROS_UFH2 (void, puttostr,
    AROS_UFHA(UBYTE, chr, D0),
    AROS_UFHA(STRPTR *,strPtrPtr,A3)
)
{
    AROS_USERFUNC_INIT
    D(bug("Putting character %c into buffer at adress %p\n",
    	chr, *strPtrPtr));
    *(*strPtrPtr)= chr;
    (*strPtrPtr) ++;
    AROS_USERFUNC_EXIT
}

/**********************************************************************************************/

STATIC IPTR text_render(Class *cl, Object *o, struct gpRender *msg)
{
    UWORD 		*pens = msg->gpr_GInfo->gi_DrInfo->dri_Pens;
    UBYTE 		textbuf[256], *str;
    struct TextData 	*data = INST_DATA(cl, o);
    WORD 		left, left2, top, width, height, numchars, tlength;
    struct TextFont 	*oldfont;
    struct RastPort 	*rp = msg->gpr_RPort;
    
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
    
    ReturnInt("Text::Render", IPTR, 0);
}

/**********************************************************************************************/

static IPTR text_dispose(Class *cl, Object *o, Msg msg)
{
    struct TextData *data = INST_DATA(cl, o);
    
    if (data->flags & TEXTF_COPYTEXT) FreeVec((APTR)data->toprint);
    if (data->font) CloseFont(data->font);
    if (data->frame) DisposeObject(data->frame);
    
    return DoSuperMethodA(cl, o, msg);

}

/**********************************************************************************************/

AROS_UFH3S(IPTR, dispatch_textclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, o, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
    AROS_USERFUNC_INIT

    IPTR retval;

    switch (msg->MethodID)
    {
	case OM_NEW:
	    retval = text_new(cl, o, (struct opSet *) msg);
	    break;

	case OM_DISPOSE:
    	    retval = text_dispose(cl, o, msg);
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
    	    retval = text_render(cl, o, (struct gpRender *)msg);
    	    break;

	case GM_GOACTIVE:
    	    retval = GMR_NOREUSE;
    	    break;

	case OM_GET:
    	    retval = text_get(cl, o, (struct opGet *)msg);
	    break;

	default:
	    retval = DoSuperMethodA(cl, o, msg);
	    break;
    }

    return retval;

    AROS_USERFUNC_EXIT
}

/**********************************************************************************************/

#undef GadToolsBase

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

/**********************************************************************************************/
