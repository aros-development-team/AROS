/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#define	DEBUG_BUILDEASYREQUEST(x)	x;

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
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/screens.h>
#include <graphics/rastport.h>
#include <graphics/gfxmacros.h>
#include <utility/tagitem.h>
#include "intuition_intern.h"

#define OUTERSPACING_X		4
#define OUTERSPACING_Y		4
#define GADGETGADGETSPACING	8
#define TEXTGADGETSPACING	4
#define TEXTBOXBORDER_X		16
#define TEXTBOXBORDER_Y		4
#define BUTTONBORDER_X		8
#define BUTTONBORDER_Y		4

/**********************************************************************************************/

struct reqdims
{
    UWORD width;       /* width of the requester */
    UWORD height;      /* height of the requester */
    UWORD fontheight;  /* height of the default font */
    UWORD fontxheight; /* extra height */
    UWORD textleft;
    WORD  gadgets;     /* number of gadgets */
    UWORD gadgetwidth; /* width of a gadget */
};

/**********************************************************************************************/

static STRPTR *buildeasyreq_makelabels(struct reqdims *dims,STRPTR labeltext,struct IntuitionBase *IntuitionBase);
static STRPTR buildeasyreq_formattext(STRPTR textformat, APTR args,struct IntuitionBase *IntuitionBase);
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

static int charsinstring(STRPTR string, char c);

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
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct Screen           	*scr = NULL, *lockedscr = NULL;
    struct Window           	*req;
    struct Gadget           	*gadgets;
    STRPTR                  	 reqtitle;
    STRPTR                  	 formattedtext;
    STRPTR                  	*gadgetlabels;
    struct                  	 reqdims dims;
    struct IntRequestUserData	*requserdata;

    DEBUG_BUILDEASYREQUEST(dprintf("intrequest_buildeasyrequest: window 0x%lx easystruct 0x%lx IDCMPFlags 0x%lx args 0x%lx\n",
                                   (ULONG) RefWindow,
                                   (ULONG) easyStruct,
                                   IDCMP,
                                   (ULONG) Args));

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
    gadgetlabels = buildeasyreq_makelabels(&dims,
                                           easyStruct->es_GadgetFormat,
                                           IntuitionBase);
    if (gadgetlabels)
    {
        formattedtext = buildeasyreq_formattext(easyStruct->es_TextFormat,
                                                Args,
                                                IntuitionBase);
        if (formattedtext)
        {
            if (buildeasyreq_calculatedims(&dims, scr,
                                           formattedtext, gadgetlabels, IntuitionBase))
            {
                gadgets = buildeasyreq_makegadgets(&dims, gadgetlabels, scr, IntuitionBase);
                if (gadgets)
                {
                    requserdata = AllocVec(sizeof(struct IntRequestUserData),
                                           MEMF_ANY|MEMF_CLEAR);
                    if (requserdata)
                    {
                        struct TagItem win_tags[] =
                        {
                            { WA_Width                      	    	  , dims.width              	    	    	    	    	},
                            { WA_Height                      	    	  , dims.height             	    	    	    	    	},
                            { WA_Left                       	    	  , (scr->Width/2) - (dims.width/2) 	    	    	    	},
                            { WA_Top                        	    	  , (scr->Height/2) - (dims.height/2) 	    	    	    	},
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
                        if (req)
                        {
                            if (lockedscr) UnlockPubScreen(NULL, lockedscr);

                            req->UserData = (BYTE *)requserdata;
                            requserdata->IDCMP = IDCMP;
                            requserdata->GadgetLabels = gadgetlabels;
                            requserdata->Gadgets = gadgets;
                            requserdata->NumGadgets = dims.gadgets;

                            DEBUG_BUILDEASYREQUEST(dprintf("intrequest_buildeasyrequest: gadgets 0x%lx\n", (ULONG) gadgets));
                            
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
            FreeVec(formattedtext);
	    
        } /* if (formattedtext) */
        intrequest_freelabels(gadgetlabels, IntuitionBase);
	
    } /* if (gadgetlabels) */
    
    if (lockedscr) UnlockPubScreen(NULL, lockedscr);

    return NULL;

    AROS_LIBFUNC_EXIT

} /* BuildEasyRequestArgs */

/**********************************************************************************************/

UWORD BgPattern[2]  = { 0xAAAA, 0x5555 };

/**********************************************************************************************/

/* draw the contents of the requester */
static void buildeasyreq_draw(struct reqdims *dims, STRPTR text,
                              struct Window *req, struct Screen *scr,
                              struct Gadget *gadgets,
                              struct IntuitionBase *IntuitionBase)
{
    struct TagItem   frame_tags[] =
    {
        {IA_Left    	, req->BorderLeft + OUTERSPACING_X  	    	    	    	    	},
        {IA_Top         , req->BorderTop + OUTERSPACING_Y                    	    	    	},
        {IA_Width    	, req->Width - req->BorderLeft - req->BorderRight - OUTERSPACING_X * 2  },
        {IA_Height    	, req->Height - req->BorderTop - req->BorderBottom -
            	    	  dims->fontheight - OUTERSPACING_Y * 2 -
            	    	  TEXTGADGETSPACING - BUTTONBORDER_Y * 2                    	    	},
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
                                       STRPTR labeltext,
                                       struct IntuitionBase *IntuitionBase)
{
    STRPTR  *gadgetlabels;
    STRPTR   label;
    int      currentgadget;
    int      len;

    /* make room for pointer-array */
    dims->gadgets = charsinstring(labeltext, '|') + 1;
    
    gadgetlabels = AllocVec((dims->gadgets + 1) * sizeof(STRPTR), MEMF_ANY);
    if (!gadgetlabels)
        return NULL;
	
    gadgetlabels[dims->gadgets] = NULL;

    /* copy label-string */
    len = strlen(labeltext) + 1;
    
    label = AllocVec(len, MEMF_ANY);
    if (!label)
    {
        FreeVec(gadgetlabels);
        return NULL;
    }
    
    CopyMem(labeltext, label, len);

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

AROS_UFH2 (void, EasyReqPutChar,
           AROS_UFHA(UBYTE, chr, D0),
           AROS_UFHA(UBYTE **,buffer,A3)
          )
{
    AROS_LIBFUNC_INIT

    *(*buffer)++=chr;

    AROS_LIBFUNC_EXIT
}

/**********************************************************************************************/

AROS_UFH2 (void, EasyReqCountChar,
           AROS_UFHA(UBYTE, chr, D0),
           AROS_UFHA(ULONG *,counter,A3)
          )
{
    AROS_LIBFUNC_INIT

    /* shut up the compiler */
    chr = chr;

    (*counter)++;

    AROS_LIBFUNC_EXIT
}

/**********************************************************************************************/

/* format the supplied text string by using the supplied args */
static STRPTR buildeasyreq_formattext(STRPTR textformat,
                                      APTR args,
                                      struct IntuitionBase *IntuitionBase)
{
#if 1
    STRPTR buffer;
    STRPTR buf;
    ULONG  len = 0;

    RawDoFmt(textformat, args, (VOID_FUNC)AROS_ASMSYMNAME(EasyReqCountChar), &len);

    buffer = AllocVec(len + 1, MEMF_ANY | MEMF_CLEAR);
    if (!buffer) return NULL;

    buf = buffer;
    RawDoFmt(textformat, args, (VOID_FUNC)AROS_ASMSYMNAME(EasyReqPutChar), &buf);

    return buffer;

#else
    int    len;
    STRPTR buffer;

    len = strlen(textformat) + 256;
    for (;;)
    {
        buffer = AllocVec(len, MEMF_ANY);
        if (!buffer)
            return NULL;
	    
        if (vsnprintf(buffer, len, textformat, args) < len)
            return buffer;
	    
        FreeVec(buffer);
        len += 256;
    }
#endif

}

/**********************************************************************************************/

/* calculate dimensions of the requester */
static BOOL buildeasyreq_calculatedims(struct reqdims *dims,
                                       struct Screen *scr,
                                       STRPTR formattedtext,
                                       STRPTR *gadgetlabels,
                                       struct IntuitionBase *IntuitionBase)
{
    STRPTR  textline;
    int     textlines, line; /* number of lines in es_TextFormat */
    int     currentgadget = 0;
    UWORD   textboxwidth = 0, gadgetswidth; /* width of upper/lower part */

    /* calculate height of requester */
    dims->fontheight = scr->RastPort.Font->tf_YSize;
    dims->fontxheight = dims->fontheight - scr->RastPort.Font->tf_Baseline;
    if (dims->fontxheight < 1) dims->fontxheight = 1;

    textlines = charsinstring(formattedtext, '\n') + 1;
    dims->height = scr->WBorTop + dims->fontheight + 1 +
                   OUTERSPACING_Y +
                   TEXTBOXBORDER_Y +
                   textlines * (dims->fontheight + dims->fontxheight) - dims->fontxheight +
                   TEXTBOXBORDER_Y +
                   TEXTGADGETSPACING +
                   BUTTONBORDER_Y +
                   dims->fontheight +
                   BUTTONBORDER_Y +
                   OUTERSPACING_Y +
                   scr->WBorBottom;

    if (dims->height > scr->Height)
        return FALSE;

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
        return FALSE;

    return TRUE;
}

/**********************************************************************************************/

/* make all the gadgets */
static struct Gadget *buildeasyreq_makegadgets(struct reqdims *dims,
                    STRPTR *gadgetlabels,
                    struct Screen *scr,
                    struct IntuitionBase *IntuitionBase)
{
    struct TagItem   frame_tags[] =
    {
        {IA_FrameType   , FRAME_BUTTON                          },
        {IA_Width       , dims->gadgetwidth                     },
        {IA_Height      , dims->fontheight + BUTTONBORDER_Y * 2 },
        {TAG_DONE                                               }
    };
    struct Gadget   *gadgetlist, *thisgadget = NULL;
    struct Image    *gadgetframe;
    WORD      	     currentgadget;
    UWORD            xoffset, spacing;
    struct DrawInfo *dri;

    if (gadgetlabels[0] == NULL)
        return NULL;

    gadgetframe = (struct Image *)NewObjectA(NULL, FRAMEICLASS, frame_tags);
    if (!gadgetframe)
        return NULL;

    spacing = dims->width - scr->WBorLeft - scr->WBorRight - OUTERSPACING_X * 2;
    xoffset = scr->WBorLeft + OUTERSPACING_X;

    if (dims->gadgets == 1)
        xoffset += (spacing - dims->gadgetwidth) / 2;
    else
        spacing = (spacing - dims->gadgetwidth) / (dims->gadgets - 1);

    dri = GetScreenDrawInfo(scr);

    gadgetlist = NULL;

    for (currentgadget = 0; gadgetlabels[currentgadget]; currentgadget++)
    {
        WORD           gadgetid = (currentgadget == (dims->gadgets - 1)) ? 0 : currentgadget + 1;
        struct TagItem gad_tags[] =
        {
            {GA_ID          , gadgetid	    	    	    	    },
            {GA_Previous    , (IPTR)thisgadget                	    },
            {GA_Left        , xoffset                       	    },
            {GA_Top         , dims->height -
             	    	      scr->WBorBottom - dims->fontheight -
             	    	      OUTERSPACING_Y - BUTTONBORDER_Y * 2   },
            {GA_Image       , (IPTR)gadgetframe                     },
            {GA_RelVerify   , TRUE                          	    },
            {GA_DrawInfo    , (IPTR)dri                     	    },
            {TAG_DONE                               	    	    }
        };
        struct TagItem gad2_tags[] =
        {
            {GA_Text        , (IPTR)gadgetlabels[currentgadget]     },
            {TAG_DONE                                               }
        };
        
        thisgadget = NewObjectA(NULL, FRBUTTONCLASS, gad_tags);

        if (currentgadget == 0)
            gadgetlist = thisgadget;

        if (!thisgadget)
        {
            intrequest_freegadgets(gadgetlist, IntuitionBase);
            return NULL;
        }

        SetAttrsA(thisgadget, gad2_tags);

        xoffset += spacing;
    }

    FreeScreenDrawInfo(scr, dri);

    return gadgetlist;
}

/**********************************************************************************************/

static int charsinstring(STRPTR string, char c)
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
