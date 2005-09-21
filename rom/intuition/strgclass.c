/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <exec/memory.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/intuition.h>
#include <intuition/cghooks.h>
#include <intuition/sghooks.h>
#include <aros/asmcall.h>
#include <string.h>
#include <stdio.h>

#include "intuition_intern.h"
#include "strgadgets.h"

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

#undef IntuitionBase
#define IntuitionBase 	    	((struct IntuitionBase *)(cl->cl_UserData))

#define SFLG_BUFFER_ALLOCATED   (1 << 2)
#define SFLG_WORKBUF_ALLOCATED  (1 << 3)
#define SFLG_UNDOBUF_ALLOCATED  (1 << 4)

static void set_buffer_str(struct StringInfo *StrInfo)
{
    char  buf[64];
    char *ptr = buf + sizeof(buf);
    LONG  value = StrInfo->LongInt;
    int   len;

    if (value < 0)
        value = -value;

    do
    {
        *--ptr = '0' + value % 10;
        value /= 10;
    }
    while (value != 0);

    if (StrInfo->LongInt < 0)
        *--ptr = '-';

    len = buf + sizeof(buf) - ptr;

    if (len >= StrInfo->MaxChars)
        len = StrInfo->MaxChars - 1;

    StrInfo->Buffer[len] = '\0';
    memcpy(StrInfo->Buffer, ptr, len);
}

/*****************
** StrG::Set()  **
*****************/
#define SETFLAG(flagvar, boolvar, flag) \
if (boolvar)                        \
flagvar |= flag;        \
else                \
flagvar &= ~flag;


STATIC IPTR strg_set(Class *cl, struct Gadget * g, struct opSet *msg)
{
    struct TagItem  *tag, *tstate;
    struct StrGData *data = INST_DATA(cl, g);
    IPTR    	     retval = (IPTR)0;

    for (tstate = msg->ops_AttrList; (tag = NextTagItem(&tstate)); )
    {
        IPTR tidata = tag->ti_Data;
        BOOL notify = FALSE;

        switch (tag->ti_Tag)
        {

            case STRINGA_LongVal:       /* [ISGNU] */
        	if (msg->MethodID != OM_NEW)
        	{

                    /* OM_NEW STRINGA_LongVal is handled in strg_new! */

                    data->StrInfo.LongInt = (LONG)tidata;

                    set_buffer_str(&data->StrInfo);
                    //snprintf(data->StrInfo.Buffer, data->StrInfo.MaxChars, "%d", data->StrInfo.LongInt);

                    g->Activation |= GACT_LONGINT;
                    retval = 1UL;
                    notify = TRUE;
        	}
        	break;

            case STRINGA_TextVal:       /* [ISGNU] */
        	if (msg->MethodID != OM_NEW)
        	{
                    /* OM_NEW STRINGA_TextVal is handled in strg_new! */

                    strcpy(data->StrInfo.Buffer, (STRPTR)tidata);
                    g->Activation &= ~GACT_LONGINT;
                    retval = 1UL;
                    notify = TRUE;
        	}
        	break;

            case STRINGA_MaxChars:      /* [I] */
        	data->StrInfo.MaxChars = (WORD)tidata;
        	break;

            case STRINGA_Buffer:        /* [I] */
        	data->StrInfo.Buffer = (STRPTR)tidata;
        	break;

            case STRINGA_UndoBuffer:    /* [I] */
        	data->StrInfo.UndoBuffer = (STRPTR)tidata;
        	break;

            case STRINGA_WorkBuffer:    /* [I] */
        	data->StrExtend.WorkBuffer = (STRPTR)tidata;
        	break;

            case STRINGA_BufferPos:     /* [ISU] */
        	data->StrInfo.BufferPos = (WORD)tidata;
        	retval = 1UL;
        	break;

            case STRINGA_DispPos:       /* [ISU] */
        	data->StrInfo.DispPos = (WORD)tidata;
        	retval = 1UL;
        	break;

            case STRINGA_AltKeyMap:     /* [IS] */
        	data->StrInfo.AltKeyMap = (struct KeyMap *)tidata;
        	break;

            case STRINGA_Font:      /* [IS] */
        	data->StrExtend.Font = (struct TextFont *)tidata;
        	retval = 1UL;
        	break;

            case STRINGA_Pens:      /* [IS] */
        	data->StrExtend.Pens[0] = ((LONG)tidata) & 0x0000FFFF;
        	data->StrExtend.Pens[1] = (((LONG)tidata) & 0xFFFF0000) >> 16;
        	retval = 1UL;
        	break;

            case STRINGA_ActivePens:    /* [IS] */
        	data->StrExtend.ActivePens[0] = ((LONG)tidata) & 0x0000FFFF;
        	data->StrExtend.ActivePens[1] = (((LONG)tidata) & 0xFFFF0000) >> 16;
        	retval = 1UL;
        	break;

            case STRINGA_EditHook:      /* [I] */
        	data->StrExtend.EditHook = (struct Hook *)tidata;
        	break;

            case STRINGA_EditModes:     /* [IS] */
        	data->StrExtend.InitialModes = (ULONG)tidata;
        	break;

            case STRINGA_ReplaceMode:   /* [IS] */
        	SETFLAG(data->StrExtend.InitialModes, (ULONG)tidata, SGM_REPLACE);
        	break;

            case STRINGA_FixedFieldMode:    /* [IS] */
        	SETFLAG(data->StrExtend.InitialModes, (ULONG)tidata, SGM_FIXEDFIELD);
        	break;

            case STRINGA_NoFilterMode:  /* [IS] */
        	SETFLAG(data->StrExtend.InitialModes, (ULONG)tidata, SGM_NOFILTER);
        	break;

            case STRINGA_Justification: /* [IS] */
        	g->Activation |= (UWORD)tidata;
        	retval = 1UL;
        	break;

            case STRINGA_ExitHelp:
        	SETFLAG(data->StrExtend.InitialModes, (ULONG)tidata, SGM_EXITHELP);
        	break;


        } /* switch (currently parsed tag) */

#if 0
        if (notify && (msg->MethodID != OM_NEW))
        {
            struct TagItem notify_tags[] =
            {
        	{ 0UL	    , 0UL   	    	},
        	{ GA_ID     , g->GadgetID	},
        	{ TAG_END   	    	    	}
            };
            struct opUpdate nmsg =
            {
                OM_NOTIFY, notify_tags, msg->ops_GInfo, 0
            };
            notify_tags[0].ti_Tag  = tag->ti_Tag;
            notify_tags[0].ti_Data = tidata;

            DoSuperMethodA(cl, (Object *)g, (Msg)&nmsg);
	    
        } /* if (the currently parsed attr supports notification) */
#endif
    } /* for (each tag in taglist) */
    
    return (retval);
} /* strg_set() */

/******************
**  StrG::Get()  **
******************/

IPTR StrGClass__OM_GET(Class *cl, struct Gadget * g, struct opGet *msg)
{
    struct StrGData *data = INST_DATA(cl, g);
    IPTR    	     retval = 1UL;

    switch (msg->opg_AttrID)
    {
	case STRINGA_LongVal:   /* [ISGNU] */
            if (g->Activation & GACT_LONGINT)
        	*(msg->opg_Storage) = (IPTR)data->StrInfo.LongInt;
            else
        	*(msg->opg_Storage) = 0UL;
            break;

	case STRINGA_TextVal:   /* [ISGNU] */
            if (!(g->Activation & GACT_LONGINT))
        	*(msg->opg_Storage) = (IPTR)data->StrInfo.Buffer;
            else
        	*(msg->opg_Storage) = 0UL;
            break;

	default:
            retval = DoSuperMethodA(cl, (Object *)g, (Msg)msg);
            break;
    }
    return (retval);
}

/******************
**  StrG::New()  **
******************/

IPTR StrGClass__OM_NEW(Class *cl, Object * o, struct opSet *msg)
{
    struct Gadget *g = (struct Gadget *)DoSuperMethodA(cl, o, (Msg)msg);
    if (g)
    {
        struct TagItem  *ti;
        struct StrGData *data = INST_DATA(cl, g);
        STRPTR      	 textval;
        WORD        	 maxchars;

        /*

          The instance object is cleared memory!
          memset(data, 0, sizeof (struct StrGData));
        */

        /* Set some defaults */
        data->StrInfo.MaxChars = 80;

        strg_set(cl, g, msg);

        /* If no buffers have been supplied, then allocate them */
        maxchars = data->StrInfo.MaxChars;
	
        if (!data->StrInfo.Buffer)
        {
            data->StrInfo.Buffer = (STRPTR)AllocVec(maxchars, MEMF_ANY);
            if (!data->StrInfo.Buffer)
                goto failure;
            data->StrInfo.Buffer[0] = '\0';
            data->Flags |= SFLG_BUFFER_ALLOCATED;
        }
	
        if (!data->StrInfo.UndoBuffer)
        {
            data->StrInfo.UndoBuffer = (STRPTR)AllocVec(maxchars, MEMF_ANY);
            if (!data->StrInfo.UndoBuffer)
                goto failure;
            data->StrInfo.UndoBuffer[0] = '\0';
            data->Flags |= SFLG_UNDOBUF_ALLOCATED;
        }
	
        if (!data->StrExtend.WorkBuffer)
        {
            data->StrExtend.WorkBuffer = (STRPTR)AllocVec(maxchars, MEMF_ANY);
            if (!data->StrExtend.WorkBuffer)
                goto failure;
            data->StrExtend.WorkBuffer[0] = '\0';
            data->Flags |= SFLG_WORKBUF_ALLOCATED;
        }

        /* Get inital string contents */
        textval = (STRPTR)GetTagData(STRINGA_TextVal, NULL, msg->ops_AttrList);
        if (textval)
        {
            strcpy(data->StrInfo.Buffer, textval);
            D(bug("strgclass:Initializing string gadget to text value %s\n", textval));
            g->Activation &= ~GACT_LONGINT;
        }
	
        ti = FindTagItem(STRINGA_LongVal, msg->ops_AttrList);
        if (ti != NULL)
        {
            LONG val = (LONG)ti->ti_Data;

            data->StrInfo.LongInt = val;
            set_buffer_str(&data->StrInfo);
            //snprintf(data->StrInfo.Buffer, data->StrInfo.MaxChars, "%d", val);

            D(bug("strgclass:Initializing string gadget to integer value %d\n", val));
            g->Activation |= GACT_LONGINT;
        }

        g->SpecialInfo = &(data->StrInfo);
        g->Flags |= GFLG_STRINGEXTEND;
        data->StrInfo.Extension = &(data->StrExtend);
    }
    return (IPTR)g;

failure:
    {
        STACKULONG method = OM_DISPOSE;
        CoerceMethodA(cl, (Object *)g, (Msg)&method);
    }
    return (IPTR)NULL;
}

/**********************
**  StrG::Dispose()  **
**********************/
IPTR StrGClass__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct StrGData *data = INST_DATA(cl, o);
    
    if ((data->StrInfo.Buffer) && (data->Flags & SFLG_BUFFER_ALLOCATED))
        FreeVec(data->StrInfo.Buffer);

    if ((data->StrInfo.UndoBuffer) && (data->Flags & SFLG_UNDOBUF_ALLOCATED))
        FreeVec(data->StrInfo.UndoBuffer);

    if ((data->StrExtend.WorkBuffer) && (data->Flags & SFLG_WORKBUF_ALLOCATED))
        FreeVec(data->StrExtend.WorkBuffer);

    return DoSuperMethodA(cl, o, msg);
}

/*********************
**  Strg::Render()  **
*********************/
IPTR StrGClass__GM_RENDER(Class *cl, struct Gadget *g, struct gpRender *msg)
{
    UpdateStrGadget(g,
        	    msg->gpr_GInfo->gi_Window,
        	    msg->gpr_GInfo->gi_Requester,
        	    IntuitionBase);
    
    return (IPTR)0;
}


/**************************
**  StrG::HandleInput()  **
**************************/
IPTR StrGClass__GM_HANDLEINPUT(Class *cl, struct Gadget *g, struct gpInput *msg)
{

    IPTR    	    	 ret;
    IPTR    	    	 retval = GMR_MEACTIVE;
    UWORD   	    	 imsgcode;
    struct InputEvent 	*ie = msg->gpi_IEvent;

    if (ie->ie_Class == IECLASS_RAWMOUSE)
    {
        if (ie->ie_Code == SELECTDOWN)
        {
            struct IBox container;

            GetGadgetIBox(g, msg->gpi_GInfo, &container);

            D(bug("*** click: mouse = %d,%d (%d %d) box = %d,%d - %d %d (%d x %d)\n ***\n",
                  ie->ie_X,
                  ie->ie_Y,
                  msg->gpi_Mouse.X,
                  msg->gpi_Mouse.Y,
                  container.Left,
                  container.Top,
                  container.Left + container.Width - 1,
                  container.Top + container.Height - 1,
                  container.Width,
                  container.Height));

            /* Click outside gadget ? */
            if (    (msg->gpi_Mouse.X >= container.Width)
                || (msg->gpi_Mouse.X < 0)
                || (msg->gpi_Mouse.Y >= container.Height)
                || (msg->gpi_Mouse.Y < 0))

            {
                retval = GMR_REUSE;
            }
        }
        else if (ie->ie_Code == MENUDOWN)
        {
            retval = GMR_REUSE;
        }
        /* Just to prevent a whole lot of MOUSE_MOVE messages being passed */
        else if (ie->ie_Code == IECODE_NOBUTTON)
        {
            return (retval);
        }
    }


    if (retval == GMR_MEACTIVE)
    {

        ret = HandleStrInput(g
		     ,msg->gpi_GInfo
                     ,ie
                     ,&imsgcode
                     ,IntuitionBase
	);


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
IPTR StrGClass__GM_GOINACTIVE(Class *cl, struct Gadget *g, struct gpGoInactive *msg)
{
    struct RastPort *rp;
    struct opUpdate  nmsg;
    struct TagItem   tags[3];
    struct StrGData *data = INST_DATA(cl, g);

    g->Flags &= ~GFLG_SELECTED;

    /* Rerender gadget in inactive state */
    rp = ObtainGIRPort(msg->gpgi_GInfo);
    if (rp)
    {
        struct gpRender method;
	
        method.MethodID   = GM_RENDER;
        method.gpr_GInfo  = msg->gpgi_GInfo;
        method.gpr_RPort  = rp;
        method.gpr_Redraw = GREDRAW_REDRAW;
        DoMethodA((Object *)g, (Msg)&method);

        ReleaseGIRPort(rp);
    }

    /* Notify evt. change of string gadget contents */

    if (g->Activation & GACT_LONGINT)
    {
        tags[0].ti_Tag  = STRINGA_LongVal;
        tags[0].ti_Data = (IPTR)data->StrInfo.LongInt;
    }
    else
    {
        tags[0].ti_Tag  = STRINGA_TextVal;
        tags[0].ti_Data = (IPTR)data->StrInfo.Buffer;
    }

    tags[1].ti_Tag  = GA_ID;
    tags[1].ti_Data = g->GadgetID;
    tags[2].ti_Tag  = TAG_END;

    nmsg.MethodID   	= OM_NOTIFY;
    nmsg.opu_AttrList   = tags;
    nmsg.opu_GInfo  	= msg->gpgi_GInfo;
    nmsg.opu_Flags  	= 0;

    DoSuperMethodA(cl, (Object *)g, (Msg)&nmsg);

    return (IPTR)0;
}

/***********************
**  Strg::GoActive()  **
***********************/
IPTR StrGClass__GM_GOACTIVE(Class *cl, struct Gadget *g, struct gpInput *msg)
{
    if (msg->gpi_IEvent)
    {
	UWORD imsgcode;

	HandleStrInput(g, msg->gpi_GInfo,
		       msg->gpi_IEvent, &imsgcode, IntuitionBase
	);

    }
    else
    {
	struct StrGData   *data = INST_DATA(cl, g);
	struct RastPort   *rp;
	struct GadgetInfo *gi = msg->gpi_GInfo;
	
	g->Flags |= GFLG_SELECTED;
	if (data->StrInfo.UndoBuffer)
	{
	    strcpy(data->StrInfo.UndoBuffer, data->StrInfo.Buffer);
	}

	if ((rp = ObtainGIRPort(gi)))
	{
	    struct gpRender method;
		    
	    method.MethodID   = GM_RENDER;
	    method.gpr_GInfo  = gi;
	    method.gpr_RPort  = rp;
	    method.gpr_Redraw = GREDRAW_REDRAW;
		    
	    DoMethodA((Object *)g, (Msg)&method);
		    
	    ReleaseGIRPort(rp);
	}
    }
    return (IPTR)GMR_MEACTIVE;
}

/******************
**  Strg::Set()  **
******************/
IPTR StrGClass__OM_SET(Class *cl, struct Gadget *g, struct opSet *msg)
{
    IPTR retval;
    
    retval = DoSuperMethodA(cl, (Object *)g, (Msg)msg);
    retval += (IPTR)strg_set(cl, g, msg);

    /* If we have been subclassed, OM_UPDATE should not cause a GM_RENDER
     * because it would circumvent the subclass from fully overriding it.
     * The check of cl == OCLASS(o) should fail if we have been
     * subclassed, and we have gotten here via DoSuperMethodA().
     */
    if ( retval && ( (msg->MethodID != OM_UPDATE) || (cl == OCLASS(g)) ) )
    {
	struct GadgetInfo *gi = msg->ops_GInfo;
	
	if (gi)
	{
	    struct RastPort *rp = ObtainGIRPort(gi);
		    
	    if (rp)
	    {
		struct gpRender method;
			
		method.MethodID   = GM_RENDER;
		method.gpr_GInfo  = gi;
		method.gpr_RPort  = rp;
		method.gpr_Redraw = GREDRAW_REDRAW;
			
		DoMethodA((Object *)g, (Msg)&method);
			
		ReleaseGIRPort(rp);
	    } /* if */
	} /* if */
    } /* if */

    return retval;
}
