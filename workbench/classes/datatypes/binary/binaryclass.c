/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
*/

/**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dostags.h>
#include <graphics/gfxbase.h>
#include <graphics/rpattr.h>
#include <intuition/imageclass.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/textclass.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#include <proto/datatypes.h>

#include "binary_intern.h"

#ifdef __AROS__
#include <aros/symbolsets.h>

ADD2LIBS("datatypes/text.datatype", 0, struct Library *, TextBase)
#else
#include "compilerspecific.h"
#endif
#include "debug.h"

/**************************************************************************************************/

    
const UBYTE hexstring[] = "0123456789ABCDEF";

/* ": 12345678 9ABCDEF0 12345678 9ABCDEF0 abcdefghijklmnop" */

#define LINELEN (1 + 1 + 8 + 1 + 8 + 1 + 8 + 1 + 8 + 1 + 16)

/**************************************************************************************************/

static IPTR NotifyAttrChanges(Object * o, VOID * ginfo, ULONG flags, ULONG tag1,...)
{
    return DoMethod(o, OM_NOTIFY, (IPTR) &tag1, (IPTR) ginfo, flags);
}

/**************************************************************************************************/

IPTR Binary__OM_NEW(Class * cl, Object *o, struct opSet *msg)
{
    IPTR retval;
    
    if ((retval = DoSuperMethodA (cl, o, (Msg)msg)))
    {
	 struct BinaryData      *data;
	 IPTR                   len, lines;
	 BOOL                   success = FALSE;
	 STRPTR                 buffer;

	 /* Get a pointer to the object data */
	 data = INST_DATA (cl, (Object *) retval);

	 /* Get the attributes that we need to determine
	  * memory pool size */
	 GetDTAttrs ((Object *) retval,
		     TDTA_Buffer        , (IPTR)&buffer,
		     TDTA_BufferLen     , (IPTR)&len,
		     TAG_DONE);

	 D(bug("BinaryDataType_new: buffer = %x  bufferlen = %d\n", buffer, len));

	 /* Make sure we have a text buffer */
	 if (buffer && len)
	 {
	     lines = (len + 15) / 16;
	     if (lines > 200) lines = 200;
	     
	     data->Pool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR,
				    lines * (sizeof(struct Line) + 8 + LINELEN),
				    lines * (sizeof(struct Line) + 8 + LINELEN));
	     
	     if (data->Pool)
		 success = TRUE;
	     else
		 SetIoErr (ERROR_NO_FREE_STORE);
	 }
	 else
	 {
	     /* Indicate that something was missing that we
	      * needed */
	     SetIoErr (ERROR_REQUIRED_ARG_MISSING);
	 }

	 if (!success)
	 {
	     CoerceMethod (cl, (Object *) retval, OM_DISPOSE);
	     retval = 0;
	 }
     }
     
     return retval;     
}

/**************************************************************************************************/

IPTR Binary__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct BinaryData   *data;
    struct List         *linelist = 0;
    IPTR                retval;
    
    /* Get a pointer to our object data */
    data = INST_DATA (cl, o);

    /* Don't let the super class free the line list */
    if (GetDTAttrs (o, TDTA_LineList, (IPTR) &linelist, TAG_DONE) && linelist)
	NewList (linelist);

    DeletePool(data->Pool);
    
    retval = DoSuperMethodA(cl, o, msg);
    
    return retval;
}

/**************************************************************************************************/

IPTR Binary__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;
    
    /* Pass the attributes to the text class and force a refresh
     * if we need it */

    if ((retval = DoSuperMethodA (cl, o, (Msg)msg)) && (OCLASS (o) == cl))
    {
	struct RastPort *rp;

	/* Get a pointer to the rastport */
	if ((rp = ObtainGIRPort (msg->ops_GInfo)))
	{
	    struct gpRender gpr;

	    /* Force a redraw */
	    gpr.MethodID   = GM_RENDER;
	    gpr.gpr_GInfo  = msg->ops_GInfo;
	    gpr.gpr_RPort  = rp;
	    gpr.gpr_Redraw = GREDRAW_UPDATE;
	    DoMethodA (o, (Msg)&gpr);

	    /* Release the temporary rastport */
	    ReleaseGIRPort (rp);
	}
	retval = 0;
    }
 
    return retval;  
}
IPTR Binary__OM_UPDATE(Class *cl, Object *o, struct opSet *msg)
{
    return Binary__OM_SET(cl, o, msg);
}

/**************************************************************************************************/

IPTR Binary__GM_LAYOUT(Class *cl, Object *o, struct gpLayout *msg)
{
    IPTR retval;
    
    /* Tell everyone that we are busy doing things */
    NotifyAttrChanges (o, msg->gpl_GInfo, 0,
		       GA_ID    , G(o)->GadgetID,
		       DTA_Busy , TRUE          ,
		       TAG_DONE);

    /* Let the super-class partake */
    retval = (IPTR) DoSuperMethodA (cl, o, (Msg)msg);

    /* We need to do this one asynchronously */
    retval += DoAsyncLayout (o, msg);
    
    return retval;
}

/**************************************************************************************************/

IPTR Binary__DTM_ASYNCLAYOUT(Class *cl, Object *o, struct gpLayout *gpl)
{
    struct DTSpecialInfo        *si = (struct DTSpecialInfo *) G (o)->SpecialInfo;
    struct BinaryData           *data = INST_DATA (cl, o);
    ULONG                       visible = 0, total = 0;
    struct RastPort             trp;
    ULONG                       hunit = 1;
    ULONG                       bsig = 0;

    /* Switches */
    BOOL                        abort = FALSE;

    /* Attributes obtained from super-class */
    struct TextAttr             *tattr;
    struct TextFont             *font;
    struct List                 *linelist;
    struct IBox                 *domain;
    IPTR                        wrap = FALSE;
    IPTR                        bufferlen;
    STRPTR                      buffer;
    STRPTR                      title;

    /* Line information */
    ULONG                       style = FS_NORMAL;
    struct Line                 *line;
    LONG                        lines, linelen, offsetlen;
    ULONG                       yoffset = 0;
    UBYTE                       fgpen = 1;
    UBYTE                       bgpen = 0;
    ULONG                       i;

    ULONG                       nomwidth, nomheight;

    D(bug("BinaryDataType_AsyncLayout\n"));
    
    /* Get all the attributes that we are going to need for a successful layout */
    if (GetDTAttrs (o, DTA_TextAttr     , (IPTR) &tattr         ,
		       DTA_TextFont     , (IPTR) &font          ,
		       DTA_Domain       , (IPTR) &domain        ,
		       DTA_ObjName      , (IPTR) &title         ,
		       TDTA_Buffer      , (IPTR) &buffer        ,
		       TDTA_BufferLen   , (IPTR) &bufferlen     ,
		       TDTA_LineList    , (IPTR) &linelist      ,
		       TDTA_WordWrap    , (IPTR) &wrap          ,
		       TAG_DONE) == 8)
    {
	D(bug("BinaryDataType_AsyncLayout: Got all attrs\n"));

	/* Lock the global object data so that nobody else can manipulate it */
	ObtainSemaphore (&(si->si_Lock));

	/* Make sure we have a buffer */
	if (buffer)
	{
	    D(bug("BinaryDataType_AsyncLayout: Got buffer\n"));
	    
	    /* Initialize the temporary RastPort */
	    InitRastPort (&trp);
	    SetFont (&trp, font);

	    lines = (bufferlen + 15) / 16;
	    offsetlen = 1;
	    if (lines * 16 >= 0x10) offsetlen = 2;
	    if (lines * 16 >= 0x100) offsetlen = 3;
	    if (lines * 16 >= 0x1000) offsetlen = 4;
	    if (lines * 16 >= 0x10000) offsetlen = 5;
	    if (lines * 16 >= 0x100000) offsetlen = 6;
	    if (lines * 16 >= 0x1000000) offsetlen = 7;
	    if (lines * 16 >= 0x10000000) offsetlen = 8;
	    
	    linelen = offsetlen + LINELEN;
	    
	    D(bug("BinaryDataType_AsyncLayout: Temp RastPort initialized\n"));

	    /* We only need to perform layout if we are doing word wrap, or this
	     * is the initial layout call. [for now we don't support word wrap in
	     * binary.datatype] */

	    D(bug("BinaryDataType_AsyncLayout: Checking if layout is needed\n"));
	    
	    if (gpl->gpl_Initial)
	    {
		D(bug("BinaryDataType_AsyncLayout: Layout IS needed. Freeing old LineList\n"));
		
		/* Delete the old line list */
		while ((line = (struct Line *) RemHead (linelist)))
		    FreePooled (data->Pool, line, sizeof (struct Line));

		D(bug("BinaryDataType_AsyncLayout. Old LineList freed\n"));

		/* Step through the text buffer */
		for (i = 0; (i < bufferlen) && (bsig == 0) && !abort; i += 16)
		{
		    /* Allocate a new line segment from our memory pool */
		    line = AllocPooled(data->Pool, sizeof(struct Line) + linelen);
		    if (!line)
		    {
			abort = TRUE;
		    }
		    else
		    {
			STRPTR text = (STRPTR)(line + 1);
			WORD n;
			
			line->ln_Text = text;
			
			for(n = 0; n < offsetlen; n++)
			{
			    text[offsetlen - 1 - n] = hexstring[(i >> (n * 4)) & 0xF];
			}
			text += offsetlen;
			*text++ = ':';
			
			for(n = 0; n < 16; n++)
			{
			    if ((n & 3) == 0) *text++ = ' ';
			    
			    if (i + n >= bufferlen)
			    {
				*text++ = ' ';
				*text++ = ' ';
			    }
			    else
			    {
				*text++ = hexstring[(buffer[i + n] >> 4) & 0xF];
				*text++ = hexstring[buffer[i + n] & 0xF];
			    }
			}
			
			*text++ = ' ';
			
			for(n = 0; n < 16; n++)
			{
			    if (i + n >= bufferlen)
			    {
				*text++ = ' ';
			    }
			    else
			    {
				BYTE c = (BYTE)buffer[i + n];
				*text++ = ( ((c & 0x7f) >= 0x20) && (c != 0x7f) ) ? c : '.';
			    }
			}
			
			line->ln_TextLen = linelen;
			line->ln_XOffset = 0;
			line->ln_YOffset = yoffset + font->tf_Baseline;
			line->ln_Width = TextLength(&trp, line->ln_Text, line->ln_TextLen);
			line->ln_Height = font->tf_YSize;
			line->ln_Flags = LNF_LF;
			line->ln_FgPen = fgpen;
			line->ln_BgPen = bgpen;
			line->ln_Style = style;
			line->ln_Data = NULL;

			/* Add the line to the list */
			AddTail(linelist, (struct Node *) line);

			yoffset += font->tf_YSize;
			
		    } /* if (!line) else ... */

		    /* Check to see if layout has been aborted */
		    bsig = CheckSignal (SIGBREAKF_CTRL_C);
					    
		} /* for (i = 0; (i < bufferlen) && (bsig == 0) && !abort; i += 16) */
		
	    } /* if (gpl->gpl_Initial) */
	    else
	    {
		lines   = si->si_TotVert;
		linelen = si->si_TotHoriz;
	    }
	    
#ifdef __AROS__
	    DeinitRastPort(&trp);
#endif /* __AROS__ */
	    
	} /* if (buffer) */

	/* Compute the lines and columns type information */
	si->si_VertUnit  = font->tf_YSize;
	si->si_VisVert   = visible = domain->Height / si->si_VertUnit;
	si->si_TotVert   = lines;

	si->si_HorizUnit = hunit = font->tf_XSize;
	si->si_VisHoriz  = domain->Width / hunit;
	si->si_TotHoriz  = linelen;
								   
	/* Release the global data lock */
	ReleaseSemaphore (&si->si_Lock);

	/* Calculate the nominal size */
	
	#if 0
	nomheight = (ULONG) (24 * font->tf_YSize);
	nomwidth  = (ULONG) (80 * font->tf_XSize);
	#endif
	
        nomwidth  = linelen * font->tf_XSize;
	nomheight = lines * font->tf_YSize;
	
	/* Were we aborted? */
	if (bsig == 0)
	{
	    /* Not aborted, so tell the world of our newest attributes */
	    NotifyAttrChanges (o, gpl->gpl_GInfo, 0,
			       GA_ID            , G(o)->GadgetID,

			       DTA_VisibleVert  , si->si_VisVert                        ,
			       DTA_TotalVert    , si->si_TotVert                        ,
			       DTA_NominalVert  , nomheight                             ,
			       DTA_VertUnit     , si->si_VertUnit                       ,

			       DTA_VisibleHoriz , si->si_VisHoriz                       ,
			       DTA_TotalHoriz   , si->si_TotHoriz                       ,
			       DTA_NominalHoriz , nomwidth                              ,
			       DTA_HorizUnit    , si->si_HorizUnit                      ,

			       DTA_Title        , (IPTR)title                           ,
			       DTA_Busy         , FALSE                                 ,
			       DTA_Sync         , TRUE                                  ,
			       TAG_DONE);
	} /* if (bsig == 0) */
		
    } /* if GetDTAttrs(... */

    D(bug("BinaryDataType_AsyncLayout: Done. Returning %d\n", total));
    
    return (IPTR)total;
}

/**************************************************************************************************/

IPTR Binary__DTM_PROCLAYOUT(Class *cl, Object *o, struct gpLayout *msg)
{
    IPTR retval;
    
    /* Tell everyone that we are busy doing things */
    NotifyAttrChanges (o, ((struct gpLayout *) msg)->gpl_GInfo, 0,
		       GA_ID    , G(o)->GadgetID,
		       DTA_Busy , TRUE          ,
		       TAG_DONE);

    /* Let the super-class partake and then fall through to our layout method */
    retval = (IPTR) DoSuperMethodA (cl, o, (Msg)msg);

    retval = Binary__DTM_ASYNCLAYOUT(cl, o, msg);

    return retval;
}
	

/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/

#ifndef __AROS__
ASM IPTR DT_Dispatcher(register __a0 struct IClass *cl, register __a2 Object * o, register __a1 Msg msg)
{
    IPTR retval = 0;

    putreg(REG_A4, (long) cl->cl_Dispatcher.h_SubEntry);        /* Small Data */

    switch(msg->MethodID)
    {
	case OM_NEW:
	    retval = Binary__OM_NEW(cl, o, (struct opSet *)msg);
	    break;
	
	case OM_DISPOSE:
	    retval = Binary__OM_DISPOSE(cl, o, msg);
	    break;
	    
	case OM_SET:
	case OM_UPDATE:
	    retval = Binary__OM_SET(cl, o, (struct opSet *)msg);
	    break;
	
	case GM_LAYOUT:
	    retval = Binary__GM_LAYOUT(cl, o, (struct gpLayout *)msg);
	    break;
	
	case DTM_PROCLAYOUT:
	    retval = Binary__DTM_PROCLAYOUT(cl, o, (struct gpLayout *)msg);
	    break;
	    
	case DTM_ASYNCLAYOUT:
	    retval = Binary__DTM_ASYNCLAYOUT(cl, o, (struct gpLayout *)msg);
	    break;
	
	default:
	    retval = DoSuperMethodA(cl, o, msg);
	    break;
	    
    } /* switch(msg->MethodID) */
    
    return retval;

}

/**************************************************************************************************/

struct IClass *DT_MakeClass(struct Library *binarybase)
{
    struct IClass *cl = MakeClass("binary.datatype", "text.datatype", 0, sizeof(struct BinaryData), 0);

    if (cl)
    {
	cl->cl_Dispatcher.h_Entry = (HOOKFUNC) DT_Dispatcher;
	cl->cl_Dispatcher.h_SubEntry = (HOOKFUNC) getreg(REG_A4);
	cl->cl_UserData = (IPTR)binarybase; /* Required by datatypes (see disposedtobject) */
    }

    return cl;
}
#endif /* !__AROS__ */

/**************************************************************************************************/
