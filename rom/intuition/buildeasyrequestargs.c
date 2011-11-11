/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#define	DEBUG_BUILDEASYREQUEST(x)

/**********************************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <clib/macros.h>
#include <aros/asmcall.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <exec/memory.h>
#include <exec/rawfmt.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/screens.h>
#include <graphics/rastport.h>
#include <graphics/gfxmacros.h>
#include <utility/tagitem.h>
#include <aros/debug.h>
#include "intuition_intern.h"


/**********************************************************************************************/

struct reqdims
{
    UWORD width;       /* width of the requester */
    UWORD height;      /* height of the requester */
    UWORD fontheight;  /* height of the default font */
    UWORD fontxheight; /* extra height */
    UWORD textleft;
    UWORD textheight;  /* Height of text frame */
    WORD  gadgets;     /* number of gadgets */
    UWORD gadgetwidth; /* width of a gadget */
};

/**********************************************************************************************/

static STRPTR *buildeasyreq_makelabels(struct reqdims *dims,CONST_STRPTR labeltext, APTR args, struct IntuitionBase *IntuitionBase);
static STRPTR buildeasyreq_formattext(CONST_STRPTR textformat, APTR args, APTR *nextargptr, struct IntuitionBase *IntuitionBase);
static BOOL buildeasyreq_calculatedims(struct reqdims *dims,
                                       struct Screen *scr,
                                       STRPTR formattedtext,
                                       STRPTR *gadgetlabels,
                                       struct IntuitionBase *IntuitionBase);
static struct Gadget *buildeasyreq_makegadgets(struct reqdims *dims,
			        STRPTR *gadgetlabels,
			        struct Screen *scr,
			        struct IntuitionBase *IntuitionBase);
static void buildeasyreq_draw(struct reqdims *dims, STRPTR text,
                              struct Window *win, struct Screen *scr,
                              struct Gadget *gadgets,
                              struct IntuitionBase *IntuitionBase);

static int charsinstring(CONST_STRPTR string, char c);

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>
#include <exec/types.h>
#include <intuition/intuition.h>

    AROS_LH4(struct Window *, BuildEasyRequestArgs,

/*  SYNOPSIS */
         AROS_LHA(struct Window     *, RefWindow, A0),
         AROS_LHA(struct EasyStruct *, easyStruct, A1),
         AROS_LHA(ULONG              , IDCMP, D0),
         AROS_LHA(APTR               , Args, A3),

/*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 99, Intuition)

/*  FUNCTION
	Opens a requester, which provides one or more choices. The control is
	returned to the application after the requester was opened. It is
	handled by subsequent calls to SysReqHandler() and closed by calling
	FreeSysRequest().
 
    INPUTS
	RefWindow - A reference window. If NULL, the requester opens on
		    the default public screen.
	easyStruct - The EasyStruct structure (<intuition/intuition.h>),
		     which describes the requester.
	IDCMP - IDCMP flags, which should satisfy the requester, too. This is
		useful for requesters, which want to listen to disk changes,
		etc. Note that this is not a pointer to the flags as in
		EasyRequestArgs().
	Args - The arguments for easyStruct->es_TextFormat.
 
    RESULT
	Returns a pointer to the requester. Use this pointer only for calls
	to SysReqHandler() and FreeSysRequest().
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
	EasyRequestArgs(), SysReqHandler(), FreeSysRequest()
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Screen           	*scr = NULL, *lockedscr = NULL;
    struct Window           	*req;
    struct Gadget           	*gadgets;
    CONST_STRPTR              	 reqtitle;
    STRPTR                  	 formattedtext;
    STRPTR                  	*gadgetlabels;
    struct                  	 reqdims dims;
    struct IntRequestUserData	*requserdata;
    APTR    	    	    	 nextarg;
    
    DEBUG_BUILDEASYREQUEST(dprintf("intrequest_buildeasyrequest: window 0x%p easystruct 0x%p IDCMPFlags 0x08%x args 0x%p\n",
                                   RefWindow, easyStruct, IDCMP, Args));

    if (!easyStruct)
        return FALSE;

    DEBUG_BUILDEASYREQUEST(dprintf("intrequest_buildeasyrequest: easy title <%s> Format <%s> Gadgets <%s>\n",
                                   easyStruct->es_Title,
                                   easyStruct->es_TextFormat,
                                   easyStruct->es_GadgetFormat));

    /* get requester title */
    reqtitle = easyStruct->es_Title;
    if ((!reqtitle) && (RefWindow))
        reqtitle = RefWindow->Title;

    if (!reqtitle) reqtitle = "System Request"; /* stegerg: should be localized */

    /* get screen and screendrawinfo */
    if (RefWindow)
        scr = RefWindow->WScreen;
	
    if (!scr)
    {
        scr = LockPubScreen(NULL);
        if (!scr)
            return FALSE;
	    
        lockedscr = scr;
    }

    /* create everything */
    formattedtext = buildeasyreq_formattext(easyStruct->es_TextFormat,
                                            Args,
					    &nextarg,
                                            IntuitionBase);
    DEBUG_BUILDEASYREQUEST(bug("intrequest_buildeasyrequest: formatted text 0x%p\n", formattedtext));
    if (formattedtext)
    {
	gadgetlabels = buildeasyreq_makelabels(&dims,
                                               easyStruct->es_GadgetFormat,
					       nextarg,
                                               IntuitionBase);
        DEBUG_BUILDEASYREQUEST(bug("intrequest_buildeasyrequest: gadget labels 0x%p\n", gadgetlabels));

    	if(gadgetlabels)
	{
            if (buildeasyreq_calculatedims(&dims, scr,
                                           formattedtext, gadgetlabels, IntuitionBase))
            {
	        DEBUG_BUILDEASYREQUEST(bug("intrequest_buildeasyrequest: dimensions OK\n"));

                gadgets = buildeasyreq_makegadgets(&dims, gadgetlabels, scr, IntuitionBase);
                if (gadgets)
                {
	            DEBUG_BUILDEASYREQUEST(dprintf("intrequest_buildeasyrequest: gadgets 0x%p\n", gadgets));

                    requserdata = AllocVec(sizeof(struct IntRequestUserData),
                                           MEMF_ANY|MEMF_CLEAR);
                    DEBUG_BUILDEASYREQUEST(dprintf("intrequest_buildeasyrequest: requester data 0x%p\n", requserdata));

                    if (requserdata)
                    {
                        struct TagItem win_tags[] =
                        {
                            { WA_Width                      	    	  , dims.width              	    	    	    	    	},
                            { WA_Height                      	    	  , dims.height             	    	    	    	    	},
                            { WA_Left                       	    	  , - scr->LeftEdge + (scr->ViewPort.DWidth/2) - (dims.width/2) },
                            { WA_Top                        	    	  , - scr->TopEdge + (scr->ViewPort.DHeight/2) - (dims.height/2)},
                            { WA_IDCMP                      	    	  , IDCMP_GADGETUP | IDCMP_RAWKEY | (IDCMP & ~IDCMP_VANILLAKEY) },
                            { WA_Gadgets                    	    	  , (IPTR)gadgets           	    	    	    	    	},
                            { WA_Title                      	    	  , (IPTR)reqtitle          	    	    	    	    	},
                            { (lockedscr ? WA_PubScreen : WA_CustomScreen), (IPTR)scr               	    	    	    	    	},
                            { WA_Flags                      	    	  , WFLG_DRAGBAR     |
                              	    	    	    	    	    	    WFLG_DEPTHGADGET |
                              	    	    	    	    	    	    WFLG_ACTIVATE    |
                              	    	    	    	    	    	    WFLG_RMBTRAP   /*|
                                                                      	    WFLG_SIMPLE_REFRESH*/   	    	    	    	    	},
                            {TAG_DONE                                       	    	    	    	    	    	    	    	}
                        };

                        req = OpenWindowTagList(NULL, win_tags);
                        
                        DEBUG_BUILDEASYREQUEST(bug("intrequest_buildeasyrequest: window 0x%p\n", req));
                        
                        if (req)
                        {
                            if (lockedscr) UnlockPubScreen(NULL, lockedscr);

                            req->UserData = (BYTE *)requserdata;
                            requserdata->IDCMP = IDCMP;
                            requserdata->GadgetLabels = gadgetlabels;
                            requserdata->Gadgets = gadgets;
                            requserdata->NumGadgets = dims.gadgets;
                            
                            buildeasyreq_draw(&dims, formattedtext,
                                              req, scr, gadgets, IntuitionBase);
                            FreeVec(formattedtext);
			    
                            return req;
                        }

                        /* opening requester failed -> free everything */
                        FreeVec(requserdata);
			
                    } /* if (requserdata) */
		    
                    intrequest_freegadgets(gadgets, IntuitionBase);
		    
                } /* if (gadgets) */
		
            } /* if (if (buildeasyreq_calculatedims... */

            intrequest_freelabels(gadgetlabels, IntuitionBase);
	    
        } /* if (gadgetlabels) */

        FreeVec(formattedtext);
	
    } /* if (formattedtext) */
    
    if (lockedscr) UnlockPubScreen(NULL, lockedscr);

    return NULL;

    AROS_LIBFUNC_EXIT

} /* BuildEasyRequestArgs */

/**********************************************************************************************/

CONST UWORD BgPattern[2]  = { 0xAAAA, 0x5555 };

/**********************************************************************************************/

/* draw the contents of the requester */
static void buildeasyreq_draw(struct reqdims *dims, STRPTR text,
                              struct Window *req, struct Screen *scr,
                              struct Gadget *gadgets,
                              struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct TagItem   frame_tags[] =
    {
        {IA_Left    	, req->BorderLeft + OUTERSPACING_X  	    	    	    	    	},
        {IA_Top         , req->BorderTop + OUTERSPACING_Y                    	    	    	},
        {IA_Width    	, req->Width - req->BorderLeft - req->BorderRight - OUTERSPACING_X * 2  },
        {IA_Height    	, dims->textheight			                    	    	},
        {IA_Recessed    , TRUE                                      	    	    	    	},
        {IA_EdgesOnly   , FALSE                                     	    	    	    	},
        {TAG_DONE                                           	    	    	    	    	}
    };
    struct DrawInfo *dri;
    struct Image    *frame;
    LONG             currentline;

    dri = GetScreenDrawInfo(scr);
    if (!dri)
        return;

    SetFont(req->RPort, dri->dri_Font);

    /* draw background pattern */
    SetABPenDrMd(req->RPort,
                 dri->dri_Pens[SHINEPEN], dri->dri_Pens[BACKGROUNDPEN],
                 JAM1);
    SetAfPt(req->RPort, BgPattern, 1);
    RectFill(req->RPort, req->BorderLeft,
             req->BorderTop,
             req->Width - req->BorderRight,
             req->Height - req->BorderBottom);
    SetAfPt(req->RPort, NULL, 0);

    /* draw textframe */
    frame = (struct Image *)NewObjectA(NULL, FRAMEICLASS, frame_tags);
    if (frame)
    {
        DrawImageState(req->RPort, frame, 0, 0, IDS_NORMAL, dri);
        DisposeObject((Object *)frame);
    }

    /* draw text */
    SetABPenDrMd(req->RPort,
                 dri->dri_Pens[TEXTPEN], dri->dri_Pens[BACKGROUNDPEN], JAM1);
    for (currentline = 1; text[0] != '\0'; currentline++)
    {
        STRPTR strend;
        int    length;

        strend = strchr(text, '\n');
        if (strend)
	{
            length = strend - text;
	}
        else
	{
            length = strlen(text);
	}
	   
        Move(req->RPort,
             dims->textleft,
             req->BorderTop + (dims->fontheight + dims->fontxheight) * (currentline - 1) +
             OUTERSPACING_Y + TEXTBOXBORDER_Y + req->RPort->Font->tf_Baseline);
	     
        Text(req->RPort, text, length);
	
        text += length;
        if (text[0] == '\n')
            text++;
    }

    /* draw gadgets */
    RefreshGList(gadgets, req, NULL, -1L);

    FreeScreenDrawInfo(scr, dri);
}

/**********************************************************************************************/

/* create an array of gadgetlabels */
static STRPTR *buildeasyreq_makelabels(struct reqdims *dims,
                                       CONST_STRPTR labeltext,
				       APTR args,
                                       struct IntuitionBase *IntuitionBase)
{
    STRPTR  *gadgetlabels;
    STRPTR   label;
    int      currentgadget;
    ULONG    len = 0;


    /* make room for pointer-array */
    dims->gadgets = charsinstring(labeltext, '|') + 1;
    
    gadgetlabels = AllocVec((dims->gadgets + 1) * sizeof(STRPTR), MEMF_ANY);
    if (!gadgetlabels)
        return NULL;
	
    gadgetlabels[dims->gadgets] = NULL;

    /* copy label-string */
    RawDoFmt(labeltext, args, (VOID_FUNC)RAWFMTFUNC_COUNT, &len);
    
    label = AllocVec(len + 1, MEMF_ANY);
    if (!label)
    {
        FreeVec(gadgetlabels);
        return NULL;
    }

    RawDoFmt(labeltext, args, (VOID_FUNC)RAWFMTFUNC_STRING, label);

    /* set up the pointers and insert null-bytes */
    for (currentgadget = 0; currentgadget < dims->gadgets; currentgadget++)
    {
        gadgetlabels[currentgadget] = label;
	
        if (currentgadget != (dims->gadgets - 1))
        {
            while (label[0] != '|')
                label++;
		
            label[0] = '\0';
            label++;
        }
    }

    return gadgetlabels;
}

/**********************************************************************************************/

/**********************************************************************************************/

/* format the supplied text string by using the supplied args */
static STRPTR buildeasyreq_formattext(CONST_STRPTR textformat,
                                      APTR args,
				      APTR *nextargptr,
                                      struct IntuitionBase *IntuitionBase)
{
    STRPTR buffer;
    ULONG  len = 0;

    RawDoFmt(textformat, args, (VOID_FUNC)RAWFMTFUNC_COUNT, &len);

    buffer = AllocVec(len + 1, MEMF_ANY | MEMF_CLEAR);
    if (!buffer) return NULL;

    *nextargptr = RawDoFmt(textformat, args, (VOID_FUNC)RAWFMTFUNC_STRING, buffer);

    return buffer;
}

/**********************************************************************************************/

/* calculate dimensions of the requester */
static BOOL buildeasyreq_calculatedims(struct reqdims *dims,
                                       struct Screen *scr,
                                       STRPTR formattedtext,
                                       STRPTR *gadgetlabels,
                                       struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    STRPTR  textline;
    int     textlines, line; /* number of lines in es_TextFormat */
    int     currentgadget = 0;
    UWORD   textboxwidth = 0, gadgetswidth; /* width of upper/lower part */

    /* calculate height of requester */
    dims->fontheight = scr->RastPort.Font->tf_YSize;
    dims->fontxheight = dims->fontheight - scr->RastPort.Font->tf_Baseline;
    if (dims->fontxheight < 1) dims->fontxheight = 1;

    textlines = charsinstring(formattedtext, '\n') + 1;
    dims->textheight = TEXTBOXBORDER_Y +
                       textlines * (dims->fontheight + dims->fontxheight) - dims->fontxheight +
                       TEXTBOXBORDER_Y;
    dims->height     = scr->WBorTop + dims->fontheight + 1 +
                       OUTERSPACING_Y +
                       dims->textheight +
                       TEXTGADGETSPACING +
                       /* Width of gadgets is not counted here. It's counted in buildeasyreq_makegadgets(). */
                       OUTERSPACING_Y +
                       scr->WBorBottom;

    /* calculate width of text-box */
    textline = formattedtext;
    for (line = 0; line<textlines; line++)
    {
        int   linelen; /* length of current text line */
        UWORD linewidth; /* width (pixel) of current text line */

        if (line == (textlines - 1))
            linelen = strlen(textline);
        else
        {
            linelen = 0;
            while (textline[linelen] != '\n')
                linelen++;
        }
        linewidth = TextLength(&scr->RastPort, textline, linelen);
        if (linewidth > textboxwidth)
            textboxwidth = linewidth;
        textline = textline + linelen + 1;
    }
    textboxwidth += TEXTBOXBORDER_X * 2;

    /* calculate width of gadgets */
    dims->gadgetwidth = 0;
    while (gadgetlabels[currentgadget])
    {
        UWORD gadgetwidth; /* width of current gadget */

        gadgetwidth = TextLength(&scr->RastPort, gadgetlabels[currentgadget],
                                 strlen(gadgetlabels[currentgadget]));
        if (gadgetwidth > dims->gadgetwidth)
            dims->gadgetwidth = gadgetwidth;
        currentgadget++;
    }

    dims->gadgetwidth += BUTTONBORDER_X * 2;
    gadgetswidth = (dims->gadgetwidth + GADGETGADGETSPACING) * dims->gadgets - GADGETGADGETSPACING;

    DEBUG_BUILDEASYREQUEST(bug("buildeasyreq_calculatedims: Textbox %u gadgets %u\n", textboxwidth, gadgetswidth));

    /* calculate width of requester and position of requester text */
    dims->textleft = scr->WBorLeft + OUTERSPACING_X + TEXTBOXBORDER_X;
    if (textboxwidth > gadgetswidth)
    {
        dims->width = textboxwidth;
    }
    else
    {
        dims->textleft += (gadgetswidth - textboxwidth) / 2;
        dims->width = gadgetswidth;
    }
    dims->width += OUTERSPACING_X * 2 + scr->WBorLeft + scr->WBorRight;

    if (dims->width > scr->Width)
    {
        DEBUG_BUILDEASYREQUEST(bug("buildeasyreq_calculatedims: Too wide (requester %u, screen %u)\n", dims->width, scr->Width));
        dims->width = scr->Width;
    }

    return TRUE;
}

/**********************************************************************************************/

/* make all the gadgets */
static struct Gadget *buildeasyreq_makegadgets(struct reqdims *dims,
                    STRPTR *gadgetlabels,
                    struct Screen *scr,
                    struct IntuitionBase *IntuitionBase)
{
    UWORD gadgetheight = dims->fontheight + BUTTONBORDER_Y * 2;
    struct Gadget   *gadgetlist, *thisgadget = NULL;
    struct Image    *gadgetframe;
    WORD      	     currentgadget;
    UWORD            xoffset, yoffset, spacing, gadgetswidth, gadgetsheight, ngadgets, nrows;
    UWORD	     x, y;
    struct DrawInfo *dri;

    if (gadgetlabels[0] == NULL)
        return NULL;

    gadgetframe = (struct Image *)NewObject(NULL, FRAMEICLASS, IA_FrameType, FRAME_BUTTON,
							       IA_Width    , dims->gadgetwidth,
							       IA_Height   , gadgetheight,
        						       TAG_DONE);

    if (!gadgetframe)
        return NULL;

    ngadgets = dims->gadgets;
    gadgetswidth = (dims->gadgetwidth + GADGETGADGETSPACING) * ngadgets - GADGETGADGETSPACING;
    spacing = dims->width - scr->WBorLeft - scr->WBorRight - OUTERSPACING_X * 2;

    DEBUG_BUILDEASYREQUEST(bug("buildeasyreq_makegadgets: Gadgets width %u, avalable space %u\n", gadgetswidth, spacing));

    /* 
     * At this point 'spacing' holds total width of inner space available for use by gadgets.
     * If gadgets would occupy more space than we have (window/screen is too narrow),
     * we will rearrange gadgets in several rows.
     * First we need to calculate how many gadgets per row will fit on the screen.
     */
    while (gadgetswidth > spacing)
    {
	if (ngadgets == 1)
	{
	    /* Only one gadget left? Too bad... */
	    break;
	}

    	ngadgets--;
    	gadgetswidth = (dims->gadgetwidth + GADGETGADGETSPACING) * ngadgets - GADGETGADGETSPACING;
    	DEBUG_BUILDEASYREQUEST(bug("buildeasyreq_makegadgets: Trying %u gadgets per row, width %u\n", ngadgets, gadgetswidth));
    }

    nrows = dims->gadgets / ngadgets;
    if (nrows * ngadgets < dims->gadgets)
    	nrows++;

    DEBUG_BUILDEASYREQUEST(bug("buildeasyreq_makegadgets: Gadgets arranged in %u rows\n", nrows));

    /* Now calculate spacing between gadgets */
    if (ngadgets > 1)
        spacing = (spacing - dims->gadgetwidth) / (ngadgets - 1);

    dri = GetScreenDrawInfo(scr);

    /* Now we know how much space our gadgets will occupy. Add the required height to the requester. */
    gadgetheight += GADGETGADGETSPACING_Y;
    gadgetsheight = nrows * gadgetheight - GADGETGADGETSPACING_Y;
    dims->height += gadgetsheight;

    DEBUG_BUILDEASYREQUEST(bug("buildeasyreq_makegadgets: Resulting requester height: %u\n", dims->height));

    /* Check if the resulting height fits on the screen. */
    if (dims->height > scr->Height)
    {
        DEBUG_BUILDEASYREQUEST(bug("buildeasyreq_makegadgets: Too high (screen %u)\n", scr->Height));

	/* Decrease height of the requester at the expense of textbox */
        dims->height = scr->Height;
        dims->textheight = dims->height - scr->WBorTop - dims->fontheight - 1 -
        		   OUTERSPACING_Y -
        		   TEXTGADGETSPACING -
			   gadgetsheight -
        		   OUTERSPACING_Y -
        		   scr->WBorBottom;
    }

    gadgetlist = NULL;
    currentgadget = 0;
    yoffset = dims->height - scr->WBorBottom - OUTERSPACING_Y - gadgetsheight;

    for (y = 0; y < nrows; y++)
    {
    	xoffset = scr->WBorLeft + OUTERSPACING_X;
    	if (ngadgets == 1)
            xoffset += (spacing - dims->gadgetwidth) / 2;

    	for (x = 0; x < ngadgets; x++)
    	{
            WORD gadgetid = (currentgadget == (dims->gadgets - 1)) ? 0 : currentgadget + 1;

            thisgadget = NewObject(NULL, FRBUTTONCLASS, GA_ID	    , gadgetid,
            						GA_Previous , thisgadget,
            						GA_Left     , xoffset,
            						GA_Top      , yoffset,
            						GA_Image    , gadgetframe,
            						GA_RelVerify, TRUE,
            						GA_DrawInfo ,dri,
            						TAG_DONE);
            if (currentgadget == 0)
            	gadgetlist = thisgadget;

            if (!thisgadget)
            {
            	intrequest_freegadgets(gadgetlist, IntuitionBase);
            	return NULL;
            }

            SetAttrs(thisgadget, GA_Text, gadgetlabels[currentgadget++], TAG_DONE);

	    if (currentgadget == dims->gadgets)
	    {
	    	/*
	    	 * The last row can be incomplete, if number of gadgets does not
	    	 * divide on number of rows.
	    	 */
	    	break;
	    }

            xoffset += spacing;
        }
        yoffset += gadgetheight;
    }

    FreeScreenDrawInfo(scr, dri);

    return gadgetlist;
}

/**********************************************************************************************/

static int charsinstring(CONST_STRPTR string, char c)
{
    int count = 0;

    while (string[0])
    {
        if (string[0] == c)
            count++;
        string++;
    }
    return count;
}

/**********************************************************************************************/
