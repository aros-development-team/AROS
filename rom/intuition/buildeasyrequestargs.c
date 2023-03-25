/*
    Copyright (C) 1995-2023, The AROS Development Team. All rights reserved.
    Copyright (C) 2001-2003, The MorphOS Development Team. All Rights Reserved.
*/

#define DEBUG_BUILDEASYREQUEST(x)

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
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/screens.h>
#include <graphics/rastport.h>
#include <graphics/gfxmacros.h>
#include <utility/tagitem.h>
#include <aros/debug.h>
#include "intuition_intern.h"

/**********************************************************************************************/

static STRPTR *buildeasyreq_makelabels(struct IntReqDims *dims,CONST_STRPTR labeltext, RAWARG args, struct IntuitionBase *IntuitionBase);
static STRPTR buildeasyreq_formattext(CONST_STRPTR textformat, RAWARG args, RAWARG *nextargptr, struct IntuitionBase *IntuitionBase);
static BOOL buildeasyreq_calculatedims(struct IntReqDims *dims,
                                       struct Screen *scr,
                                       STRPTR formattedtext,
                                       STRPTR *gadgetlabels,
                                       struct IntuitionBase *IntuitionBase);
static struct Gadget *buildeasyreq_makegadgets(struct IntReqDims *dims,
                                STRPTR *gadgetlabels,
                                struct Screen *scr,
                                ULONG *idcmp,
                                struct IntuitionBase *IntuitionBase);
static struct Gadget *FindGadgetByID(struct Gadget *thisgadget, UWORD GadgetID);

const struct TagItem mapproptop[] = {
    {PGA_Top, ICSPECIAL_CODE},
    {TAG_END, }
};

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
        AROS_LHA(RAWARG             , Args, A3),

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

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Screen               *scr = NULL, *lockedscr = NULL;
    struct Window               *req;
    struct Gadget               *gadgets;
    CONST_STRPTR                 reqtitle;
    STRPTR                      *gadgetlabels;
    struct IntRequestUserData   *requserdata;
    RAWARG                       nextarg;
    ULONG                       reqidcmp = IDCMP_GADGETUP | IDCMP_RAWKEY;

    DEBUG_BUILDEASYREQUEST(dprintf("%s: window 0x%p easystruct 0x%p IDCMPFlags 0x08%x args 0x%p\n", __func__,
                                   RefWindow, easyStruct, IDCMP, Args));

    if (!easyStruct)
        return FALSE;

    DEBUG_BUILDEASYREQUEST(dprintf("%s: easy title <%s> Format <%s> Gadgets <%s>\n", __func__,
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
    requserdata = AllocVec(sizeof(struct IntRequestUserData),
                           MEMF_ANY|MEMF_CLEAR);
    DEBUG_BUILDEASYREQUEST(dprintf("%s: requester data 0x%p\n", __func__, requserdata));

    if (requserdata)
    {

        requserdata->drawreq = buildreq_draw;

        requserdata->RawBody = buildeasyreq_formattext(easyStruct->es_TextFormat,
                                                Args,
                                                &nextarg,
                                                IntuitionBase);
        DEBUG_BUILDEASYREQUEST(bug("%s: formatted text 0x%p\n", __func__, requserdata->RawBody));
        if (requserdata->RawBody)
        {
            gadgetlabels = buildeasyreq_makelabels(&requserdata->ReqDims,
                                                   easyStruct->es_GadgetFormat,
                                                   nextarg,
                                                   IntuitionBase);
            DEBUG_BUILDEASYREQUEST(bug("%s: gadget labels 0x%p\n", __func__, gadgetlabels));

            if(gadgetlabels)
            {
                if (buildeasyreq_calculatedims(&requserdata->ReqDims, scr,
                                               requserdata->RawBody, gadgetlabels, IntuitionBase))
                {
                    DEBUG_BUILDEASYREQUEST(bug("%s: dimensions OK\n", __func__));
                    struct TextAttr *reqfont = NULL;
                    struct TextAttr defreqfont = {
                        "topaz.font",
                        scr->RastPort.Font->tf_YSize,
                        scr->RastPort.Font->tf_Style,
                        0
                    };
#if (0)
                    reqfont = &defreqfont;
#else
                    reqfont = GetPrivIBase(IntuitionBase)->ReqFont;
#endif
                    requserdata->ReqBody = requester_makebodyplain(&requserdata->ReqDims, requserdata->RawBody, reqfont);
                    DEBUG_BUILDEASYREQUEST(bug("%s: itext ReqBody 0x%p\n", __func__, requserdata->ReqBody));

                    gadgets = buildeasyreq_makegadgets(&requserdata->ReqDims, gadgetlabels, scr, &reqidcmp, IntuitionBase);
                    if (gadgets)
                    {
                        DEBUG_BUILDEASYREQUEST(dprintf("%s: gadgets 0x%p\n", __func__, gadgets));

                        struct TagItem win_tags[] =
                        {
                            { WA_Width                                    , requserdata->ReqDims.width                                                  },
                            { WA_Height                                   , requserdata->ReqDims.height                                                 },
                            { WA_Left                                     , - scr->LeftEdge + (scr->ViewPort.DWidth/2) - (requserdata->ReqDims.width/2) },
                            { WA_Top                                      , - scr->TopEdge + (scr->ViewPort.DHeight/2) - (requserdata->ReqDims.height/2)},
                            { WA_IDCMP                                    , reqidcmp | (IDCMP & ~IDCMP_VANILLAKEY)                      },
                            { WA_Gadgets                                  , (IPTR)gadgets                                               },
                            { WA_Title                                    , (IPTR)reqtitle                                              },
                            { (lockedscr ? WA_PubScreen : WA_CustomScreen), (IPTR)scr                                                   },
                            { WA_Flags                                    , WFLG_DRAGBAR     |
                                                                            WFLG_DEPTHGADGET |
                                                                            WFLG_ACTIVATE    |
                                                                            WFLG_RMBTRAP   /*|
                                                                            WFLG_SIMPLE_REFRESH*/                                       },
                            {TAG_DONE                                                                                                   }
                        };

                        req = OpenWindowTagList(NULL, win_tags);
                        
                        DEBUG_BUILDEASYREQUEST(bug("%s: window 0x%p\n", __func__, req));
                        
                        if (req)
                        {
                            if (lockedscr) UnlockPubScreen(NULL, lockedscr);

                            req->UserData = (BYTE *)requserdata;
                            requserdata->IDCMP = IDCMP;
                            requserdata->GadgetLabels = gadgetlabels;
                            requserdata->Gadgets = gadgets;

                            if (gadgets = FindGadgetByID(req->FirstGadget, (UWORD)-3))
                            {
                                DEBUG_BUILDEASYREQUEST(dprintf("%s: Adjusting VProp gadget @ 0x%p\n", __func__, gadgets));
                                SetGadgetAttrs(gadgets, req, NULL,
                                    PGA_Total, requserdata->ReqDims.textheight,
                                    PGA_Visible, req->GZZHeight,
                                    PGA_Top, 0,
                                    TAG_END);
                            }
                            if (gadgets = FindGadgetByID(req->FirstGadget, (UWORD)-4))
                            {
#if (0)
                                DEBUG_BUILDEASYREQUEST(dprintf("%s: Adjusting HProp gadget @ 0x%p\n", __func__, gadgets));
                                SetGadgetAttrs(gadgets, req, NULL,
                                    PGA_Total, content_w,
                                    PGA_Visible, req->GZZWidth,
                                    PGA_Top, pos_x,
                                    TAG_END);
#endif                    
                            }
                            gadgets = requserdata->Gadgets;
                            buildreq_draw(&requserdata->ReqDims, requserdata->ReqBody,
                                     req, scr, requserdata->Gadgets, IntuitionBase);
                            
                            return req;
                        }

                        intrequest_freegadgets(gadgets, IntuitionBase);
                        
                    } /* if (gadgets) */
                    
                } /* if (if (buildeasyreq_calculatedims... */

                intrequest_freelabels(gadgetlabels, IntuitionBase);
                
            } /* if (gadgetlabels) */

            FreeVec(requserdata->RawBody);
            
        } /* if (requserdata->RawBody) */

        /* opening requester failed -> free everything */
        FreeVec(requserdata);
        
    } /* if (requserdata) */
    
    if (lockedscr) UnlockPubScreen(NULL, lockedscr);

    return NULL;

    AROS_LIBFUNC_EXIT

} /* BuildEasyRequestArgs */



/**********************************************************************************************/

static struct Gadget *FindGadgetByID(struct Gadget *thisgadget, UWORD GadgetID)
{
    DEBUG_BUILDEASYREQUEST(dprintf("%s: Looking for %d\n", __func__, (WORD)GadgetID));
    while ((thisgadget))
    {
        DEBUG_BUILDEASYREQUEST(dprintf("%s: 0x%p (%d)\n", __func__, thisgadget, (WORD)thisgadget->GadgetID));
        if (thisgadget->GadgetID == GadgetID)
        {
            return thisgadget;
        }
        thisgadget = thisgadget->NextGadget;
    }
    return NULL;
}

/**********************************************************************************************/

/* create an array of gadgetlabels */
static STRPTR *buildeasyreq_makelabels(struct IntReqDims *dims,
                                       CONST_STRPTR labeltext,
                                       RAWARG args,
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
                                      RAWARG args,
                                      RAWARG *nextargptr,
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
static BOOL buildeasyreq_calculatedims(struct IntReqDims *dims,
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
static struct Gadget *buildeasyreq_makegadgets(struct IntReqDims *dims,
                    STRPTR *gadgetlabels,
                    struct Screen *scr,
                    ULONG   *idcmp,
                    struct IntuitionBase *IntuitionBase)
{
    UWORD gadgetheight = dims->fontheight + BUTTONBORDER_Y * 2;
    struct Gadget   *gadgetlist, *thisgadget = NULL;
    struct Image    *gadgetframe;
    WORD             currentgadget;
    UWORD            xoffset, yoffset, spacing, gadgetswidth, gadgetsheight, ngadgets, nrows;
    UWORD            x, y;
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

    DEBUG_BUILDEASYREQUEST(bug("%s: Gadgets width %u, avalable space %u\n", __func__, gadgetswidth, spacing));

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
        DEBUG_BUILDEASYREQUEST(bug("%s: Trying %u gadgets per row, width %u\n", __func__, ngadgets, gadgetswidth));
    }

    nrows = dims->gadgets / ngadgets;
    if (nrows * ngadgets < dims->gadgets)
        nrows++;

    DEBUG_BUILDEASYREQUEST(bug("%s: Gadgets arranged in %u rows\n", __func__, nrows));

    /* Now calculate spacing between gadgets */
    if (ngadgets > 1)
        spacing = (spacing - dims->gadgetwidth) / (ngadgets - 1);

    dri = GetScreenDrawInfo(scr);

    /* Now we know how much space our gadgets will occupy. Add the required height to the requester. */
    gadgetheight += GADGETGADGETSPACING_Y;
    gadgetsheight = nrows * gadgetheight - GADGETGADGETSPACING_Y;
    dims->height += gadgetsheight;

    DEBUG_BUILDEASYREQUEST(bug("%s: Resulting requester height: %u\n", __func__, dims->height));
    dims->disptextheight = dims->textheight;

    /* Check if the resulting height fits on the screen. */
    if (dims->height > scr->Height)
    {
        DEBUG_BUILDEASYREQUEST(bug("%s: Too high (screen %u)\n", __func__, scr->Height));

        /* Decrease height of the requester at the expense of textbox */
        dims->height = scr->Height;
        dims->disptextheight = dims->height - scr->WBorTop - dims->fontheight - 1 -
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

            thisgadget = NewObject(NULL, FRBUTTONCLASS, GA_ID       , gadgetid,
                                                        GA_Previous , (IPTR)thisgadget,
                                                        GA_Left     , xoffset,
                                                        GA_Top      , yoffset,
                                                        GA_Image    , (IPTR)gadgetframe,
                                                        GA_RelVerify, TRUE,
                                                        GA_DrawInfo , (IPTR)dri,
                                                        TAG_DONE);

            DEBUG_BUILDEASYREQUEST(bug("%s: GadgetID %u @ 0x%p\n", __func__, gadgetid, thisgadget));

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

    if (dims->textheight > dims->disptextheight)
    {
#define SCRLWIDTH   18
#define SCRLHEIGHT  9
        thisgadget = NewObject(NULL, PROPGCLASS, GA_ID, (UWORD)-3,
            GA_Previous, (IPTR)thisgadget,
            GA_RelRight, -(OUTERSPACING_X + SCRLWIDTH + 2),
            GA_Top, (scr->WBorTop + OUTERSPACING_Y + dims->fontheight + 1),
            GA_Width, SCRLWIDTH - 8,
            GA_RelHeight, -(dims->height - dims->disptextheight - scr->WBorTop + dims->fontheight),
            GA_DrawInfo, (IPTR)dri,
            PGA_Freedom, FREEVERT,
            PGA_NewLook, TRUE,
            ICA_TARGET, ICTARGET_IDCMP,
            ICA_MAP, mapproptop,
            TAG_END);

        DEBUG_BUILDEASYREQUEST(bug("%s: Vert Prop @ 0x%p\n", __func__, thisgadget));
        *idcmp |= IDCMP_IDCMPUPDATE;
        if (currentgadget == 0)
            gadgetlist = thisgadget;
    }
#if (0)
    if ()
    {
        thisgadget = NewObject (NULL,PROPGCLASS, GA_ID, (IPTR)-4UL,
            GA_Previous, thisgadget,
            GA_Left, wborleft,
            GA_RelBottom, -size_h + 3,
            GA_RelWidth, -wborleft - SCRLWIDTH - 2,
            GA_Height, size_h - 4,
            GA_DrawInfo, dri,
            PGA_Freedom, FREEHORIZ,
            PGA_NewLook, TRUE,
            ICA_TARGET, ICTARGET_IDCMP,
            ICA_MAP, mapproptop,
            TAG_END);
    }
#endif

    FreeScreenDrawInfo(scr, dri);

    return gadgetlist;
}

/**********************************************************************************************/
