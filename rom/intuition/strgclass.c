/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Code for BOOPSI strgclass.
    Lang: english
*/

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/boopsi.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/intuition.h>
#include <intuition/cghooks.h>
#include <intuition/sghooks.h>
#include <aros/asmcall.h>
#include <string.h>

#include "intuition_intern.h"
#include "strgadgets.h"

#define DEBUG 0
#include <aros/debug.h>

#undef IntuitionBase
#define IntuitionBase ((struct IntuitionBase *)(cl->cl_UserData))
#define EG(o) ((struct ExtGadget *)o)

struct StrGData
{
    struct StringInfo	StrInfo;
    struct StringExtend	StrExtend;
    UBYTE		PenFlags;

};

#define PFLG_INACTIVE_SET	(1 << 0)
#define PFLG_ACTIVE_SET		(1 << 1)

/*****************
** StrG::Set()  **
*****************/
#define SETFLAG(flagvar, boolvar, flag) \
    if (boolvar)			\
    	flagvar |= flag;		\
    else				\
    	flagvar &= ~flag;


IPTR strg_set(Class *cl, Object * o, struct opSet *msg)
{
    IPTR retval = 0UL;
    struct TagItem *tag, *tstate;
    struct StrGData *data = INST_DATA(cl, o);
    struct TagItem notify_tags[] = 
    {
    	{ 0UL, 		0UL},
    	{ GA_ID,	EG(o)->GadgetID},
    	{ TAG_END, }
    };
    
    for (tstate = msg->ops_AttrList; (tag = NextTagItem(&tstate)); )
    {
    	IPTR tidata = tag->ti_Data;
    	BOOL notify = FALSE;
    	
    	switch (tag->ti_Tag)
    	{

    	case STRINGA_LongVal:		/* [ISGNU] */
     	    data->StrInfo.LongInt = (LONG)tidata;
     	    EG(o)->Activation |= GACT_LONGINT;
     	    retval = 1UL;
     	    notify = TRUE;
    	    break;
    	    
    	case STRINGA_TextVal:		/* [ISGNU] */
	    strcpy(data->StrInfo.Buffer, (STRPTR)tidata);
     	    EG(o)->Activation &= ~GACT_LONGINT;
     	    retval = 1UL;
     	    notify = TRUE;
    	    break;

    	case STRINGA_MaxChars:		/* [I] */
    	    data->StrInfo.MaxChars = (WORD)tidata;
    	    break;

    	case STRINGA_Buffer:		/* [I] */
    	    data->StrInfo.Buffer = (STRPTR)tidata;
    	    break;

    	case STRINGA_UndoBuffer:	/* [I] */
    	    data->StrInfo.UndoBuffer = (STRPTR)tidata;
    	    break;

    	case STRINGA_WorkBuffer:	/* [I] */
    	    data->StrExtend.WorkBuffer = (STRPTR)tidata;
    	    break;

    	case STRINGA_BufferPos:		/* [ISU] */
    	    data->StrInfo.BufferPos = (WORD)tidata;
    	    retval = 1UL;
    	    break;

    	case STRINGA_DispPos:		/* [ISU] */
    	    data->StrInfo.DispPos = (WORD)tidata;
    	    retval = 1UL;
    	    break;

    	case STRINGA_AltKeyMap:		/* [IS] */
    	    data->StrInfo.AltKeyMap = (struct KeyMap *)tidata;
    	    break;

    	case STRINGA_Font:		/* [IS] */
    	    data->StrExtend.Font = (struct TextFont *)tidata;
    	    retval = 1UL;
    	    break;

    	case STRINGA_Pens:		/* [IS] */
    	    data->StrExtend.Pens[0] = ((LONG)tidata) & 0x0000FFFF;
    	    data->StrExtend.Pens[1] = (((LONG)tidata) & 0xFFFF0000) >> 16;
    	    retval = 1UL;
    	    break;

    	case STRINGA_ActivePens:	/* [IS] */
    	    data->StrExtend.ActivePens[0] = ((LONG)tidata) & 0x0000FFFF;
    	    data->StrExtend.ActivePens[1] = (((LONG)tidata) & 0xFFFF0000) >> 16;
    	    retval = 1UL;
    	    break;

    	case STRINGA_EditHook:		/* [I] */
    	    data->StrExtend.EditHook = (struct Hook *)tidata;
    	    break;
    	
    	case STRINGA_EditModes:		/* [IS] */
    	    data->StrExtend.InitialModes = (ULONG)tidata;
    	    break;

    	case STRINGA_ReplaceMode:	/* [IS] */
    	    SETFLAG(data->StrExtend.InitialModes, (ULONG)tidata, SGM_REPLACE);
    	    break;

    	case STRINGA_FixedFieldMode:	/* [IS] */
    	    SETFLAG(data->StrExtend.InitialModes, (ULONG)tidata, SGM_FIXEDFIELD);
    	    break;
    	    
    	case STRINGA_NoFilterMode:	/* [IS] */
    	    SETFLAG(data->StrExtend.InitialModes, (ULONG)tidata, SGM_NOFILTER);
    	    break;

    	case STRINGA_Justification:	/* [IS] */
    	    EG(o)->Activation |= (UWORD)tidata;
    	    retval = 1UL;
    	    break;

    	case STRINGA_ExitHelp:
    	    SETFLAG(data->StrExtend.InitialModes, (ULONG)tidata, SGM_EXITHELP);
    	    break;

    	    
    	} /* switch (currently parsed tag) */
    	
    	if (notify)
    	{
    	    struct opUpdate nmsg = {OM_NOTIFY, notify_tags, msg->ops_GInfo, 0};
    	    notify_tags[0].ti_Tag  = tag->ti_Tag;
    	    notify_tags[0].ti_Data = tidata;
    	    
	    DoSuperMethodA(cl, o, (Msg)&nmsg);
    	} /* if (the currently parsed attr supports notification) */
    } /* for (each tag in taglist) */
    return (retval);
} /* strg_set() */

/******************
**  StrG::Get()  **
******************/

IPTR strg_get(Class *cl, Object * o, struct opGet *msg)
{
    IPTR retval = 1UL;
    struct StrGData *data = INST_DATA(cl, o);
    
    switch (msg->opg_AttrID)
    {
    	case STRINGA_LongVal:	/* [ISGNU] */
    	    if (EG(o)->Activation & GACT_LONGINT)
    	    	*(msg->opg_Storage) = (IPTR)data->StrInfo.LongInt;
    	    else
    	    	*(msg->opg_Storage) = 0UL;
    	    break;
    	    
    	case STRINGA_TextVal:	/* [ISGNU] */
    	    if (!(EG(o)->Activation & GACT_LONGINT))
    	    	*(msg->opg_Storage) = (IPTR)data->StrInfo.Buffer;
    	    else
    	    	*(msg->opg_Storage) = 0UL;
    	    break;
    	    
    	default:
    	    retval = DoSuperMethodA(cl, o, (Msg)msg);
    	    break;
    }
    return (retval);
}

/******************
**  StrG::New()  **
******************/
 
Object *strg_new(Class *cl, Object * o, struct opSet *msg)
{
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);
    if (o)
    {
    	struct StrGData *data = INST_DATA(cl, o);
    	memset(data, 0, sizeof (struct StrGData));
    	
    	/* Set some defaults */
    	
    	strg_set(cl, o, msg);
    	
    	EG(o)->SpecialInfo = &(data->StrInfo);
    	EG(o)->Flags |= GFLG_STRINGEXTEND;
    	data->StrInfo.Extension = &(data->StrExtend);
    }
    return (o);
}

/*********************
**  Strg::Render()  **
*********************/

VOID strg_render(Class *cl, Object *o, struct gpRender *msg)
{
    struct StrGData *data = INST_DATA(cl, o);
    /* This is a kludge to set default values for 
    ** STRINGA_Pens and STRINGA_ActivePens attrs. These defaults may not
    ** be set during OM_NEW, because we then know nothing about
    ** our display environment
    */
    if (!(data->PenFlags & PFLG_INACTIVE_SET))
    {
    	UWORD *pens = msg->gpr_GInfo->gi_DrInfo->dri_Pens;
    	
    	data->StrExtend.Pens[0] = pens[TEXTPEN];
    	data->StrExtend.Pens[1] = pens[BACKGROUNDPEN];
    }

    if (!(data->PenFlags & PFLG_ACTIVE_SET))
    {
    	UWORD *pens = msg->gpr_GInfo->gi_DrInfo->dri_Pens;
    	
    	data->StrExtend.ActivePens[0] = pens[TEXTPEN];
    	data->StrExtend.ActivePens[1] = pens[SHINEPEN];
    }
    
    switch (msg->gpr_Redraw)
    {
    case GREDRAW_REDRAW: {
    	#undef IM
    	#define IM(im)	((struct Image *)im)
    	
	ULONG x, y;

	if (EG(o)->GadgetRender)
	{
	    struct IBox container;
	    GetGadgetIBox(o, msg->gpr_GInfo, &container);

	    /* center image position, we assume image top and left is 0 */
	    SetAttrs(EG(o)->GadgetRender,
                         IA_Width, container.Width + 4,
                         IA_Height, container.Height + 4,
                         TAG_DONE);

	    x = container.Left - 2; 
	    
	    /*(container.Width / 2) -
		(IM(EG(o)->GadgetRender)->Width / 2);*/
		
	    y = container.Top - 2; /* + (container.Height / 2) -
		(IM(EG(o)->GadgetRender)->Height / 2) - 2;*/

	    D(bug("strg: Rendering framing image at pos (%d,%d)\n", x, y));
	    DrawImageState(msg->gpr_RPort,
		    IM(EG(o)->GadgetRender),
		    x, y,
		    ((EG(o)->Flags & GFLG_SELECTED) ? IDS_SELECTED : IDS_NORMAL ),
		    msg->gpr_GInfo->gi_DrInfo);
	} /* if (gadget has a GadgetRender image) */
   	    	
	UpdateStrGadget((struct Gadget *)o,
    	    	    msg->gpr_GInfo->gi_Window,
    	    	    IntuitionBase);
    	} break;
    	    	
    case GREDRAW_UPDATE:
	UpdateStrGadget((struct Gadget *)o,
    	   msg->gpr_GInfo->gi_Window,
    	   IntuitionBase);
    	break;
    }
    return;
}


/**************************
**  StrG::HandleInput()  **
**************************/
IPTR strg_handleinput(Class *cl, Object *o, struct gpInput *msg)
{

    ULONG ret;
    IPTR retval = 0UL;
    
    UWORD imsgcode;
    struct InputEvent *ie = msg->gpi_IEvent;
    
    if ((ie->ie_Class == IECLASS_RAWMOUSE) && (ie->ie_Code == SELECTDOWN))
    {
    	struct IBox container;
    
    	GetGadgetIBox(o, msg->gpi_GInfo, &container);
    	
    	/* Click outside gadget */
    	if (    (ie->ie_X > container.Left + container.Width)
    	     || (ie->ie_X < container.Left)
    	     || (ie->ie_Y > container.Top  + container.Height)
    	     || (ie->ie_Y < container.Top))
    	     
    	{
    	     retval = GMR_REUSE;
    	     
    	}
    }
    
    if (!retval)
    {    
    	ret = HandleStrInput((struct Gadget *)o
	    		,msg->gpi_GInfo
	    		,ie
	    		,&imsgcode
	    		,IntuitionBase);
	    		
	    
    	if (ret & (SGA_END|SGA_PREVACTIVE|SGA_NEXTACTIVE))
    	{
	    if (ret & SGA_REUSE)
	    	retval = GMR_REUSE;
	    else
	    	retval = GMR_NOREUSE;
	    	    
	    if (ret & SGA_PREVACTIVE)
	    	retval |= GMR_PREVACTIVE;
	    else if (ret & SGA_NEXTACTIVE)
	    	retval |= GMR_NEXTACTIVE;
	    	    
	    retval |= GMR_VERIFY;
		*(msg->gpi_Termination) = (LONG)imsgcode; 
	
    	}
    	else
    	{
	     retval = GMR_MEACTIVE;
    	}
    } /* if (retval hasn't allreay been set) */
    
    return (retval);
}

/*************************
**  Strg::GoInactive()  **
*************************/
IPTR strg_goinactive(Class *cl, Object *o, struct gpGoInactive *msg)
{
    struct RastPort *rp;
    
    struct opUpdate nmsg;
    struct TagItem tags[2];
    struct StrGData *data = INST_DATA(cl, o);
    
    EG(o)->Flags &= ~GFLG_SELECTED;
    
    /* Rerender gadget in inactive state */
    rp = ObtainGIRPort(msg->gpgi_GInfo);
    if (rp)
    {
    	DoMethod(o, GM_RENDER, msg->gpgi_GInfo, rp, GREDRAW_REDRAW);
    	
    	
    	ReleaseGIRPort(rp);
    }
    
    /* Notify evt. change of string gadget contents */

    if (EG(o)->Activation & GACT_LONGINT)
    {
    	tags[0].ti_Tag  = STRINGA_LongVal;
    	tags[0].ti_Data = (IPTR)data->StrInfo.LongInt; 
    }
    else
    {
    	tags[0].ti_Tag  = STRINGA_TextVal;
    	tags[0].ti_Data = (IPTR)data->StrInfo.Buffer;
    }
    
    tags[1].ti_Tag = TAG_END;
    
    nmsg.MethodID	= OM_NOTIFY;
    nmsg.opu_AttrList	= tags;
    nmsg.opu_GInfo	= msg->gpgi_GInfo;
    nmsg.opu_Flags	= 0;
    
    DoSuperMethodA(cl, o, (Msg)&nmsg);
    
    return (0UL);
}


/*****************
**  Dispatcher  **
*****************/

#define gpR(msg) ((struct gpRender *)msg)
#define gpI(msg) ((struct gpInput  *)msg)

AROS_UFH3(STATIC IPTR, dispatch_strgclass,
    AROS_UFHA(Class *,  cl,  A0),
    AROS_UFHA(Object *, o,   A2),
    AROS_UFHA(Msg,      msg, A1)
)
{
    IPTR retval = 0UL;

    D(bug("strg dispatcher: %d\n", msg->MethodID));

    switch(msg->MethodID)
    {
    	case GM_RENDER:
    	    strg_render(cl, o, (struct gpRender *)msg);
    	    break;
	    
	case GM_GOACTIVE:
	    
	    if (gpI(msg)->gpi_IEvent)
	    {
	    	UWORD imsgcode;
	    	
	    	HandleStrInput((struct Gadget *)o, gpI(msg)->gpi_GInfo,
	    		gpI(msg)->gpi_IEvent, &imsgcode, IntuitionBase);

	    }
	    retval = GMR_MEACTIVE;
	    break;

	case GM_HANDLEINPUT:
	    retval = strg_handleinput(cl, o, (struct gpInput *)msg);
	    break;
	    	    
	case GM_GOINACTIVE:
	    retval = strg_goinactive(cl, o, (struct gpGoInactive *)msg);
	    break;

	case OM_NEW:
	    retval = (IPTR)strg_new(cl, o, (struct opSet *)msg);
	    break;

	case OM_SET:
	case OM_UPDATE:
	    retval = DoSuperMethodA(cl, o, msg);
	    retval += (IPTR)strg_set(cl, o, (struct opSet *)msg);

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
	    retval = (IPTR)strg_get(cl, o, (struct opGet *)msg);
	    break;

	default:
	    retval = DoSuperMethodA(cl, o, msg);
	    break;
    } /* switch */

    ReturnPtr ("strg disp", IPTR, retval);
}  /* dispatch_strgclass */


#undef IntuitionBase

/****************************************************************************/

/* Initialize our strg class. */
struct IClass *InitStrGClass(struct IntuitionBase * IntuitionBase)
{
    struct IClass *cl = NULL;

    /* This is the code to make the strgclass...
     */
    if ((cl = MakeClass(STRGCLASS, GADGETCLASS, NULL, sizeof(struct StrGData), 0)))
    {
	cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_strgclass);
	cl->cl_Dispatcher.h_SubEntry = NULL;
	cl->cl_UserData 	     = (IPTR)IntuitionBase;

	AddClass (cl);
    }

    return (cl);
}

