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

#include "ascii_intern.h"

#undef DEBUG
#define DEBUG 1
#include <aros/debug.h>

#include <aros/symbolsets.h>
ADD2LIBS("datatypes/text.datatype", 0, struct Library *, TextBase)

/**************************************************************************************************/

static IPTR NotifyAttrChanges(Object * o, VOID * ginfo, ULONG flags, ULONG tag1,...)
{
    return DoMethod(o, OM_NOTIFY, (IPTR) &tag1, (IPTR) ginfo, flags);
}

/**************************************************************************************************/

IPTR Ascii__OM_NEW(Class * cl, Object *o, struct opSet *msg)
{
    IPTR retval;
    
    if ((retval = DoSuperMethodA (cl, o, (Msg)msg)))
    {
         struct AsciiData 	*data;
         IPTR 			len, estlines, poolsize;
         BOOL 			success = FALSE;
         STRPTR 		buffer;

         /* Get a pointer to the object data */
         data = INST_DATA (cl, (Object *) retval);

         /* Get the attributes that we need to determine
	  * memory pool size */
         GetDTAttrs ((Object *) retval,
                     TDTA_Buffer	, (IPTR)&buffer,
                     TDTA_BufferLen	, (IPTR)&len,
                     TAG_DONE);

	D(bug("AsciiDataType_new: buffer = %x  bufferlen = %d\n", buffer, len));

         /* Make sure we have a text buffer */
         if (buffer && len)
         {
             /* Estimate the pool size that we will need */
             estlines = (len / 80) + 1;
             estlines = (estlines > 200) ? 200 : estlines;
             poolsize = sizeof (struct Line) * estlines;

             /* Create a memory pool for the line list */
             if ((data->Pool = CreatePool (MEMF_CLEAR | MEMF_PUBLIC, poolsize, poolsize)))
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

IPTR Ascii__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct AsciiData 	*data;
    struct List 	*linelist = 0;
    IPTR		retval;
    
    /* Get a pointer to our object data */
    data = INST_DATA (cl, o);

    /* Don't let the super class free the line list */
    if (GetDTAttrs (o, TDTA_LineList, (IPTR) &linelist, TAG_DONE) && linelist)
        NewList (linelist);

    /* Delete the line pool */
    DeletePool (data->Pool);

    retval = DoSuperMethodA(cl, o, msg);
    
    return retval;
}

/**************************************************************************************************/

IPTR Ascii__OM_SET(Class *cl, Object *o, struct opSet *msg)
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
IPTR Ascii__OM_UPDATE(Class *cl, Object *o, struct opSet *msg)
{
    return Ascii__OM_SET(cl, o, msg);
}

/**************************************************************************************************/

IPTR Ascii__GM_LAYOUT(Class *cl, Object *o, struct gpLayout *msg)
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

IPTR Ascii__DTM_PROCLAYOUT(Class *cl, Object *o, struct gpLayout *msg)
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

IPTR Ascii__DTM_ASYNCLAYOUT(Class *cl, Object *o, struct gpLayout *gpl)
{
    struct DTSpecialInfo 	*si = (struct DTSpecialInfo *) G (o)->SpecialInfo;
    struct AsciiData 		*data = INST_DATA (cl, o);
    ULONG 			visible = 0, total = 0;
    struct RastPort 		trp;
    ULONG 			hunit = 1;
    ULONG 			bsig = 0;

    /* Switches */
    BOOL 			linefeed = FALSE;
    BOOL 			newseg = FALSE;
    BOOL 			abort = FALSE;

    /* Attributes obtained from super-class */
    struct TextAttr 		*tattr;
    struct TextFont 		*font;
    struct List 		*linelist;
    struct IBox 		*domain;
    IPTR 			wrap = FALSE;
    IPTR 			bufferlen;
    STRPTR 			buffer;
    STRPTR 			title;

    /* Line information */
    ULONG 			num, offset, swidth;
    ULONG 			anchor = 0, newanchor;
    ULONG 			style = FS_NORMAL;
    struct Line 		*line;
    ULONG 			yoffset = 0;
    ULONG			linelength, max_linelength = 0;
    UBYTE 			fgpen = 1;
    UBYTE 			bgpen = 0;
    ULONG 			tabspace;
    ULONG 			numtabs;
    ULONG 			i;

    ULONG 			nomwidth, nomheight;

    D(bug("AsciiDataType_AsyncLayout\n"));
    
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
        D(bug("AsciiDataType_AsyncLayout: Got all attrs\n"));

        /* Lock the global object data so that nobody else can manipulate it */
        ObtainSemaphore (&(si->si_Lock));

        /* Make sure we have a buffer */
        if (buffer)
        {
	    D(bug("AsciiDataType_AsyncLayout: Got buffer\n"));
	    
            /* Initialize the temporary RastPort */
            InitRastPort (&trp);
            SetFont (&trp, font);

    	    D(bug("AsciiDataType_AsyncLayout: Temp RastPort initialized\n"));

            /* Calculate the nominal size */
            nomheight = (ULONG) (24 * font->tf_YSize);
            nomwidth  = (ULONG) (80 * font->tf_XSize);

	    /* Calculate the tab space */
	    tabspace = font->tf_XSize * 8;

            /* We only need to perform layout if we are doing word wrap, or this
             * is the initial layout call */

    	    D(bug("AsciiDataType_AsyncLayout: Checking if layout is needed\n"));
	    
            if (wrap || gpl->gpl_Initial)
            {
		D(bug("AsciiDataType_AsyncLayout: Layout IS needed. Freeing old LineList\n"));
		
                /* Delete the old line list */
                while ((line = (struct Line *) RemHead (linelist)))
                    FreePooled (data->Pool, line, sizeof (struct Line));

    		D(bug("AsciiDataType_AsyncLayout. Old LineList freed\n"));

                /* Step through the text buffer */
                for (i = offset = num = numtabs = 0;
                     (i < bufferlen) && (bsig == 0) && !abort;
                     i++)
                {
		    /* Check for end of line */
		    if (buffer[i] == '\n')	// && buffer[i+1]==10)

		    {
			newseg = linefeed = TRUE;
			newanchor = i + 1;
		    }
		    /* Check for end of page */
		    else if (buffer[i] == 12)
		    {
			newseg = linefeed = TRUE;
			newanchor = i + 1;
		    }
		    /* Check for tab */
		    else if (buffer[i] == '\t')
		    {
			/* See if we need to terminate a line segment */
			if ((numtabs == 0) && num)
			{
			    newseg = TRUE;
			}
			numtabs++;
		    }
		    else
		    {
			/* See if we have any TABs that we need to finish out */
			if (numtabs)
			{
			    offset += (((offset / tabspace) + 1) * tabspace) - offset;
			    num = numtabs = 0;
			    anchor = i;
			}

			/* Compute the width of the line. */
			swidth = TextLength(&trp, &buffer[anchor], num + 1);
			num++;
		    }

    	    	    if (i == bufferlen - 1) newseg = TRUE;
		    
		    /* Time for a new text segment yet? */
		    if (newseg)
		    {
			/* Allocate a new line segment from our memory pool */
			if ((line = AllocPooled(data->Pool, sizeof(struct Line))))
			{
			    swidth = TextLength(&trp, &buffer[anchor], num);
			    line->ln_Text = &buffer[anchor];
			    line->ln_TextLen = num;
			    line->ln_XOffset = offset;
			    line->ln_YOffset = yoffset + font->tf_Baseline;
			    line->ln_Width = swidth;
			    line->ln_Height = font->tf_YSize;
			    line->ln_Flags = (linefeed) ? LNF_LF : 0;
			    line->ln_FgPen = fgpen;
			    line->ln_BgPen = bgpen;
			    line->ln_Style = style;
			    line->ln_Data = NULL;

			    linelength = line->ln_Width + line->ln_XOffset;
			    if (linelength > max_linelength)
			    {
			    	max_linelength = linelength;
			    }
			    			    
			    /* Add the line to the list */
			    AddTail(linelist, (struct Node *) line);

			    /* Increment the line count */
			    if (linefeed)
			    {
				yoffset += font->tf_YSize;
				offset = 0;
				total++;
			    }
			    else
			    {
				/* Increment the offset */
				offset += swidth;
			    }
			}
			else
			{
			    abort = TRUE;
			}

			/* Clear the variables */
			newseg = linefeed = FALSE;
			anchor = newanchor;
			num = 0;

                        /* Check to see if layout has been aborted */
                        bsig = CheckSignal (SIGBREAKF_CTRL_C);
			
                    } /* if (newseg) */
		    
                }

		/*
		 * check for last line
		 */

		D(bug("AsciiDataType_AsyncLayout: end textloop, anchor %ld\n",anchor));

#if 0
		if (buffer[anchor])
		{
			linefeed=TRUE;
			D(bug("AsciiDataType_AsyncLayout: add last line <%s>\n",
				&buffer[anchor]));
			/* Allocate a new line segment from our memory pool */
			if ((line = AllocPooled(data->Pool, sizeof(struct Line))))
			{
			    swidth = TextLength(&trp, &buffer[anchor], num);
			    line->ln_Text = &buffer[anchor];
			    line->ln_TextLen = num;
			    line->ln_XOffset = offset;
			    line->ln_YOffset = yoffset + font->tf_Baseline;
			    line->ln_Width = swidth;
			    line->ln_Height = font->tf_YSize;
			    line->ln_Flags = (linefeed) ? LNF_LF : 0;
			    line->ln_FgPen = fgpen;
			    line->ln_BgPen = bgpen;
			    line->ln_Style = style;
			    line->ln_Data = NULL;

			    linelength = line->ln_Width + line->ln_XOffset;
			    if (linelength > max_linelength)
			    {
				max_linelength = linelength;
			    }
			    			    
			    /* Add the line to the list */
			    AddTail(linelist, (struct Node *) line);

			    /* Increment the line count */
			    if (linefeed)
			    {
				yoffset += font->tf_YSize;
				offset = 0;
				total++;
			    }
			    else
			    {
				/* Increment the offset */
				offset += swidth;
			    }
			}
			else
			{
			    abort = TRUE;
			}
		}
#endif
		
            } /* if (wrap || gpl->gpl_Initial) */
            else
            {
                /* No layout to perform */
                total  = si->si_TotVert;
		max_linelength = si->si_TotHoriz * si->si_HorizUnit;
            }
	    
	    DeinitRastPort(&trp);
	    
        } /* if (buffer) */

        /* Compute the lines and columns type information */
        si->si_VertUnit  = font->tf_YSize;
        si->si_VisVert   = visible = domain->Height / si->si_VertUnit;
        si->si_TotVert   = total;

/*        si->si_HorizUnit = hunit = 1;
        si->si_VisHoriz  = (LONG) domain->Width / hunit;
        si->si_TotHoriz  = domain->Width;*/
	
	si->si_HorizUnit = hunit = font->tf_XSize;
	si->si_VisHoriz  = domain->Width / hunit;
	si->si_TotHoriz  = max_linelength / hunit;
								   
        /* Release the global data lock */
        ReleaseSemaphore (&si->si_Lock);

        /* Were we aborted? */
        if (bsig == 0)
        {
            /* Not aborted, so tell the world of our newest attributes */
            NotifyAttrChanges (o, gpl->gpl_GInfo, 0,
                               GA_ID		, G(o)->GadgetID,

                               DTA_VisibleVert	, visible				,
                               DTA_TotalVert	, total					,
                               DTA_NominalVert	, nomheight				,
                               DTA_VertUnit	, font->tf_YSize			,
                            /* DTA_TopVert   	, si->si_TopVert                        , */

                               DTA_VisibleHoriz	, (domain->Width / hunit)		,
                               DTA_TotalHoriz	, max_linelength / hunit		,
                               DTA_NominalHoriz	, nomwidth				,
                               DTA_HorizUnit	, hunit					,
    	    	    	       DTA_TopHoriz 	, si->si_TopHoriz   	    	    	,
			       
                               DTA_Title	, (IPTR)title				,
                               DTA_Busy		, FALSE					,
                               DTA_Sync		, TRUE					,
                               TAG_DONE);
        } /* if (bsig == 0) */
		
    } /* if GetDTAttrs(... */

    D(bug("AsciiDataType_AsyncLayout: Done. Returning %d\n", total));
    
    return (IPTR)total;
}


/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/
