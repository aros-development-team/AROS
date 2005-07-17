/*
    Copyright © 2004-2005, The AROS Development Team. All rights reserved.
    $Id$

    Based on ascii.datatype
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
#include <graphics/text.h>
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

#include "common.h"
#include "htmlclass.h"

#define POOLSIZE (16384)

#undef DEBUG
#define DEBUG 1
#include <aros/debug.h>

#include <aros/symbolsets.h>

/* Open superclass */
ADD2LIBS("datatypes/text.datatype", 0, struct Library *, TextBase);

/*******************************************************************************************/
/* Parser */

static int call_parser( struct HtmlData *data, STRPTR buffer, LONG bytes )
{
    BOOL ret;
    page_struct *page;

    D(bug("call parser\n");)
    page = parse_init( data->mempool );
    if( !page )
	return FALSE;
    D(bug("parse_init done, page %p\n", page);)
    ret = parse_do( page, buffer, bytes );
    if( !ret )
    {
	parse_free( page );
	return FALSE;
    }
    D(bug("parse_do done\n");)
    ret = parse_end( page );
    if( !ret )
    {
	parse_free( page );
	return FALSE;
    }
    D(bug("parse_end done\n");)

    ret = layout_init( page );
    if( !ret )
    {
	parse_free( page );
	return FALSE;
    }
    D(bug("layout_init done\n");)

    data->page = page;
    return TRUE;
}

/*******************************************************************************************/
/* Text Size */

int text_len( layout_struct *ldata, string str, int strlen )
{
    struct HtmlData 	*data;
    int len, i;

    /* Compute the width of the line. */
    data = ldata->userdata;
//    len = strlen;
    D( bug("["); for(i=0; i<strlen; i++) bug("%c", str[i]); bug("] "); )
    len = TextLength(data->rastport, str, strlen);
    D(bug("text_len %d -> %d\n", strlen, len);)
    return len;
}

int text_height( layout_struct *ldata )
{
    struct HtmlData 	*data;

    data = ldata->userdata;
/*	height = data->font->tf_YSize;*/
	return 1;
}

int text_fit( layout_struct *ldata, string str, int strlen, int *strsize, int maxwidth )
{
    struct HtmlData 	*data;
    struct TextExtent	te;
    int len, i;

    data = ldata->userdata;
//    fontx = data->font->tf_XSize;
    D( bug("["); for(i=0; i<strlen; i++) bug("%c", str[i]); bug("] "); )
	
/*	if( strlen*fontx > maxwidth )
		len = maxwidth/fontx;
	else
		len = strlen;
	*strsize = len * fontx; */
    len = TextFit( data->rastport, str, strlen, &te, NULL, 1, maxwidth, data->font->tf_YSize );
    D(bug("text_fit %d < %d -> %d %d\n", strlen, maxwidth, len, te.te_Width);)
    *strsize = te.te_Width;
    return len;
}

/*******************************************************************************************/
/* Line List */

int linelist_init( layout_struct *ldata )
{
	return 0;
}

void * linelist_store( layout_struct *ldata, string textseg, u_short textlen,
		u_short xpos, u_short ypos, u_short width, u_short height, style_flags styleflags, int linebreak )
{
    struct HtmlData 	*data;
    struct List 	*linelist;
    struct Line 	*line;
    struct TextFont 	*font;
    ULONG		softstyle;

	D( {
		int	i;
		bug("\nLINE len=%2d: x=%2d y=%2d w=%2d h=%2d [", textlen, xpos, ypos, width, height);
		for(i=0; i<textlen; i++)
			bug("%c", textseg[i]);
		if( linebreak )
			bug("}<BR>\n");
		else
			bug("]\n");
	} )

    data = ldata->userdata;
    linelist = data->linelist;
    font = data->font;
    softstyle = 0;
    if( styleflags.fl.underlined )
	softstyle |= FSF_UNDERLINED;
    if( styleflags.fl.bold )
	softstyle |= FSF_BOLD;
    if( styleflags.fl.italics )
	softstyle |= FSF_ITALIC;
//    D(bug("ldata=%p data=%p linelist=%p font=%p\n", ldata, data, linelist, font);)

    /* Allocate a new line segment from our memory pool */
    if( (line = AllocPooled(data->mempool, sizeof(struct Line))) )
    {
	line->ln_Text = textseg;
	line->ln_TextLen = textlen;
	line->ln_Data = (APTR)data->linenum++;
	line->ln_XOffset = xpos;
	line->ln_YOffset = ypos * font->tf_YSize + font->tf_Baseline;
	line->ln_Width = width;
	line->ln_Height = height * font->tf_YSize;
	line->ln_Flags = (linebreak) ? LNF_LF : 0;
	line->ln_FgPen = (softstyle & FSF_UNDERLINED) ? 3 : data->fgpen;
	line->ln_BgPen = data->bgpen;
	line->ln_Style = softstyle;
	D(bug("%08p => num=%2d xo=%2d yo=%2d w=%2d h=%2d flag=%d style=%d\n", line, (int)line->ln_Data,
		line->ln_XOffset, line->ln_YOffset, line->ln_Width, line->ln_Height, line->ln_Flags, line->ln_Style);)

	/* Add the line to the list */
	AddTail(linelist, (struct Node *) line);
    }
    else
    {
	D(bug("Alloc line failed\n");)
    }
    return line;
}

void * linelist_addlf( layout_struct *ldata, void * myline )
{
    struct HtmlData 	*data;
    struct Line 	*line;

    line = myline;
    data = ldata->userdata;
    line->ln_Flags |= LNF_LF;
    D(bug("%08p => num=%2d addlf\n", line, (int)line->ln_Data);)
    return line;
}

void linelist_free( layout_struct *ldata )
{
}

/**************************************************************************************************/

static IPTR NotifyAttrChanges(Object * o, VOID * ginfo, ULONG flags, ULONG tag1,...)
{
    return DoMethod(o, OM_NOTIFY, (IPTR) &tag1, (IPTR) ginfo, flags);
}

/**************************************************************************************************/

IPTR Html__OM_NEW(Class * cl, Object *o, struct opSet *msg)
{
    IPTR retval;
    
    if ((retval = DoSuperMethodA (cl, o, (Msg)msg)))
    {
         struct HtmlData 	*data;
         IPTR 			len;
         BOOL 			success = FALSE;
         STRPTR 		buffer;
	 int			ret;

         /* Get a pointer to the object data */
         data = INST_DATA (cl, (Object *) retval);
	 data->mempool = NULL;
	 data->page = NULL;

         /* Get the attributes that we need to determine
	  * memory pool size */
         GetDTAttrs ((Object *) retval,
                     TDTA_Buffer	, (IPTR)&buffer,
                     TDTA_BufferLen	, (IPTR)&len,
                     TAG_DONE);

	D(bug("HtmlDataType_new: buffer = %x  bufferlen = %d\n", buffer, len));

         /* Make sure we have a text buffer */
         if (buffer && len)
         {
             /* Create a memory pool for the line list */
             if ((data->mempool = CreatePool (MEMF_CLEAR | MEMF_PUBLIC, POOLSIZE, POOLSIZE)))
	     {
		 ret = call_parser( data, buffer, len );
		 if( ret )
		     success = TRUE;
	     }
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

IPTR Html__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct HtmlData 	*data;
    struct List 	*linelist = 0;
    IPTR		retval;
    
    /* Get a pointer to our object data */
    data = INST_DATA (cl, o);

    /* Free parse&layout private data */
    layout_free( data->page );
    D(bug("layout_free done\n");)
    parse_free( data->page );		/* also frees seglist & page */
    D(bug("parse_free done\n");)

    /* Don't let the super class free the line list */
    if (GetDTAttrs (o, TDTA_LineList, (IPTR) &linelist, TAG_DONE) && linelist)
        NewList (linelist);

    /* Delete the line pool */
    DeletePool (data->mempool);

    retval = DoSuperMethodA(cl, o, msg);
    
    return retval;
}

/**************************************************************************************************/

IPTR Html__OM_SET(Class *cl, Object *o, struct opSet *msg)
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

/**************************************************************************************************/

IPTR Html__GM_LAYOUT(Class *cl, Object *o, struct gpLayout *msg)
{
    IPTR retval;
    
    /* Tell everyone that we are busy doing things */
    NotifyAttrChanges (o, msg->gpl_GInfo, 0,
                       GA_ID	, G(o)->GadgetID,
                       DTA_Busy	, TRUE		,
                       TAG_DONE);

    /* Let the super-class partake */
    retval = (IPTR) DoSuperMethodA (cl, o, (Msg)msg);

    /* We need to do this one asynchronously */
    retval += DoAsyncLayout (o, msg);
    
    return retval;
}

/**************************************************************************************************/

IPTR Html__DTM_PROCLAYOUT(Class *cl, Object *o, struct gpLayout *msg)
{
    IPTR retval;
    
    /* Tell everyone that we are busy doing things */
    NotifyAttrChanges (o, ((struct gpLayout *) msg)->gpl_GInfo, 0,
                       GA_ID	, G(o)->GadgetID,
                       DTA_Busy	, TRUE		,
                       TAG_DONE);

    /* Let the super-class partake and then fall through to our layout method */
    retval = (IPTR) DoSuperMethodA (cl, o, (Msg)msg);

    return retval;
}
	
/**************************************************************************************************/

IPTR Html__DTM_ASYNCLAYOUT(Class *cl, Object *o, struct gpLayout *gpl)
{
    struct DTSpecialInfo 	*si;
    struct HtmlData 		*data;
    struct RastPort 		trp;
    ULONG 			hunit = 1, vunit = 1;
    ULONG 			bsig = 0;

    /* Attributes obtained from super-class */
    struct TextAttr 		*tattr;
    struct TextFont 		*font;
    struct List 		*linelist;
    struct IBox 		*domain;
    IPTR 			wrap = FALSE;
    IPTR 			bufferlen;
    STRPTR 			buffer;
    STRPTR 			title;

    struct Line 		*line;
    ULONG 			nomwidth = 0, nomheight = 0;
    BOOL			ret;
    int				width, height;

    D(bug("HtmlDataType_AsyncLayout\n"));
    si = (struct DTSpecialInfo *) G (o)->SpecialInfo;
    data = INST_DATA (cl, o);

    /* Get all the attributes that we are going to need for a successful layout */
    if (GetDTAttrs (o, DTA_TextAttr	, (IPTR) &tattr		,
                       DTA_TextFont	, (IPTR) &font		,
                       DTA_Domain	, (IPTR) &domain	,
                       DTA_ObjName	, (IPTR) &title		,
                       TDTA_Buffer	, (IPTR) &buffer	,
                       TDTA_BufferLen	, (IPTR) &bufferlen	,
                       TDTA_LineList	, (IPTR) &linelist	,
                       TDTA_WordWrap	, (IPTR) &wrap		,
                       TAG_DONE) == 8)
    {
        D(bug("HtmlDataType_AsyncLayout: Got all attrs\n"));

        /* Lock the global object data so that nobody else can manipulate it */
        ObtainSemaphore (&(si->si_Lock));

        /* Make sure the parser has run already */
        if (data->page)
        {
	    D(bug("HtmlDataType_AsyncLayout: Got parsed page\n"));
	    
            /* Initialize the temporary RastPort */
            InitRastPort (&trp);
            SetFont (&trp, font);

    	    D(bug("HtmlDataType_AsyncLayout: Temp RastPort initialized\n"));

	    data->page->ldata->userdata = data;	/* looping :-) */
	    data->linelist = linelist;
	    data->rastport = &trp;
	    data->font = font;
	    data->style = 0;
	    data->fgpen = 1;
	    data->bgpen = 2;
	    D(bug("ldata=%p data=%p linelist=%p font=%p\n", data->page->ldata, data, linelist, font);)

    	    D(bug("HtmlDataType_AsyncLayout: Checking if layout is needed\n"));
	    
	    D(bug("HtmlDataType_AsyncLayout: Layout IS needed. Freeing old LineList\n"));
	    
	    /* Delete the old line list */
	    while ((line = (struct Line *) RemHead (linelist)))
		FreePooled (data->mempool, line, sizeof (struct Line));

	    D(bug("HtmlDataType_AsyncLayout. Old LineList freed\n"));

	    data->linenum = 1;
	    ret = layout_do( data->page, domain->Width, &width, &height );
	    if( !ret )
		return 10;
	    D(bug("layout_do done, width=%d height=%d domainwidth=%d\n", width, height, domain->Width);)
	    nomwidth = width;
	    nomheight = height;

	    /* Check to see if layout has been aborted */
	    bsig = CheckSignal (SIGBREAKF_CTRL_C);

	    DeinitRastPort(&trp);
	    
        } /* if (buffer) */

        /* Compute the lines and columns type information */
        si->si_VertUnit  = vunit = font->tf_YSize;
        si->si_VisVert   = domain->Height / vunit;
        si->si_TotVert   = height;

//	si->si_HorizUnit = hunit = font->tf_XSize;
	si->si_HorizUnit = hunit = 1;
	si->si_VisHoriz  = domain->Width / hunit;
	si->si_TotHoriz  = width;
								   
        /* Release the global data lock */
        ReleaseSemaphore (&si->si_Lock);

        /* Were we aborted? */
        if (bsig == 0)
        {
            /* Not aborted, so tell the world of our newest attributes */
            NotifyAttrChanges (o, gpl->gpl_GInfo, 0,
                               GA_ID		, G(o)->GadgetID,

                               DTA_VisibleVert	, (domain->Height / vunit)		,
                               DTA_TotalVert	, height				,
                               DTA_NominalVert	, nomheight				,
                               DTA_VertUnit	, vunit					,

                               DTA_VisibleHoriz	, (domain->Width / hunit)		,
                               DTA_TotalHoriz	, width					,
                               DTA_NominalHoriz	, nomwidth				,
                               DTA_HorizUnit	, hunit					,

                               DTA_Title	, (IPTR)title				,
                               DTA_Busy		, FALSE					,
                               DTA_Sync		, TRUE					,
                               TAG_DONE);
        } /* if (bsig == 0) */
		
    } /* if GetDTAttrs(... */

    D(bug("HtmlDataType_AsyncLayout: Done. Returning %d\n", height));
    
    return (IPTR)height;
}


/**************************************************************************************************/
