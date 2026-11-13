#ifndef _REQUESTERS_H_
#define _REQUESTERS_H_
    
/*
    Copyright © 2002-2023 The AROS Development Team, All Rights Reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/exec.h>
#include <exec/types.h>
#include <aros/asmcall.h>
#include "intuition_intern.h"

#define OUTERSPACING_X      4
//horiz space between win border and requester's box
#define OUTERSPACING_Y      4
//horiz space between win border and requester's box
#define GADGETGADGETSPACING 8
//space between gadgets
#define GADGETGADGETSPACING_Y 8
//vert space  between gadgets
#define TEXTGADGETSPACING   4
//space between textbox and gadgets row
#define TEXTBOXBORDER_X     16
//min space between text and textboxborder
#define TEXTBOXBORDER_Y     4
#define BUTTONBORDER_X      8
//min space between gadaget text and gadget border
#define BUTTONBORDER_Y      4
#define LOGOTEXTSPACING_X   8

/* I'm too lazy to open the font and query it. Anyway this is hardcoded. */
#define TOPAZ_8_BASELINE 6

struct IntReqDims
{
    UWORD           width;          /* width of the requester */
    UWORD           height;         /* height of the requester */
    UWORD           offx;
    UWORD           offy;
    UWORD           fontheight;     /* height of the default font */
    UWORD           fontxheight;    /* extra height */
    UWORD           textleft;
    UWORD           textheight;     /* Height of text frame */
    UWORD           disptextheight; /* visible text frame */
    UWORD           gadgets;        /* number of gadgets */
    UWORD           gadgetwidth;    /* width of a gadget */
};

#ifdef SKINS
struct RequesterGadget
{
    LONG            xpos;
    LONG            ypos;
    LONG            width;
    LONG            height;
};

struct IntRequestUserData
{
    ULONG                        IDCMP;
    STRPTR                     * GadgetLabels;
    struct Gadget              * Gadgets;
    struct RequesterGadget     * ReqGadgets;
    struct Window              * ReqWindow;
    struct Screen              * ReqScreen;
    struct CustomButtonImage   * Logo;
    struct IntuiText           * Text;
    struct IntDrawInfo         * dri;
    struct Hook                  backfillhook;
    struct HookData              backfilldata;
    UWORD           	    	 NumGadgets;
    UWORD           	    	 NumLines;
    UWORD           	    	 ActiveGad;
    BOOL            	    	 freeitext;
    
    LONG            	    	 wwidth;
    LONG            	    	 wheight;
    LONG            	    	 gadgetswidth;
    LONG            	    	 gadgetsheight;
    LONG            	    	 gadgetsxpos;
    LONG            	    	 gadgetsypos;
    LONG            	    	 gadgetspacing;
    LONG            	    	 textboxwidth;
    LONG            	    	 textboxheight;
    LONG            	    	 textboxxpos;
    LONG            	    	 textboxypos;
    LONG            	    	 logoxpos;
    LONG            	    	 logoypos;
    LONG            	    	 textxpos;
    LONG            	    	 textypos;

    ULONG           	    	 logotype;
};
#else
struct IntRequestUserData
{
    ULONG           	        IDCMP;
    STRPTR          	        * GadgetLabels;
    struct Gadget               * Gadgets;
    struct IntuiText            * ReqBody;
    APTR                        RawBody;
    struct IntReqDims           ReqDims;
    void                        (*drawreq)(struct IntReqDims *dims, struct IntuiText *itext, struct Window *req,
                                                    struct Screen *scr, struct Gadget *gadgets, struct IntuitionBase *IntuitionBase);
};
#endif

/**********************************************************************************************/

static inline int charsinstring(CONST_STRPTR string, char c)
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

static inline struct IntuiText *requester_makebody(STRPTR string, struct TextAttr *font)
{
    struct IntuiText *res;
    char *s = string;
    unsigned int lines = 0;
    unsigned int i;

    /* First count number of lines */
    do
    {
        while (*s++);   /* Skip text bytes including NULL terminator  */
        lines++;
    } while (*s++);     /* This automatically skips continuation byte */

    res = AllocVec(sizeof(struct IntuiText) * lines, MEMF_ANY);
    if (res)
    {
        s = string;
        for (i = 0; i < lines; i++)
        {
            res[i].FrontPen = 1;
            res[i].BackPen  = 0;
            res[i].DrawMode = JAM2;
            res[i].ITextFont = font;
            res[i].LeftEdge = AROS_BE2WORD(*((UWORD *)s));
            s += 2;
            res[i].TopEdge  = *s++ - TOPAZ_8_BASELINE;
            res[i].IText = s;
            while(*s++);
            res[i].NextText = *s++ ? &res[i+1] : NULL;
        }
    }
    return res;
}

static inline struct IntuiText *requester_makebodyplain(struct IntReqDims *dims, STRPTR string, struct TextAttr *font)
{
    struct IntuiText *res;
    char *s;
    unsigned int lines = charsinstring(string, '\n') + 1;
    unsigned int i;

    res = AllocVec(sizeof(struct IntuiText) * lines, MEMF_ANY);
    if (res)
    {
        s = string;
        for (i = 0; i < lines; i++)
        {
            res[i].FrontPen = 1;
            res[i].BackPen  = 0;
            res[i].DrawMode = JAM2;
            res[i].ITextFont = font;
            res[i].LeftEdge = 0;
            res[i].TopEdge  = (dims->fontheight + dims->fontxheight) * i;
            res[i].IText = s;
            while (*s)
            {
                if (*s == '\n')
                    break;
                s++;
            }
            res[i].NextText = *s ? &res[i+1] : NULL;
            *s++ = '\0';
        }
    }
    return res;
}

/* Miscellaneous prototypes */
void intrequest_freelabels(STRPTR *gadgetlabels, struct IntuitionBase *IntuitionBase);
void intrequest_freegadgets(struct Gadget *gadgets, struct IntuitionBase *IntuitionBase);
#ifdef SKINS
struct IntuiText *intrequest_createitext(struct IntRequestUserData *udata,STRPTR text,APTR *args,struct IntuitionBase *IntuitionBase);
void intrequest_freeitext(struct IntuiText *it,struct IntuitionBase *IntuitionBase);
void intrequest_layoutrequester(struct IntRequestUserData *udata,struct IntuitionBase *IntuitionBase);
BOOL intrequest_creategadgets(struct IntRequestUserData *udata,struct IntuitionBase *IntuitionBase);
void intrequest_drawrequester(struct IntRequestUserData *udata,struct IntuitionBase *IntuitionBase);
void intrequest_hilightgadget(struct IntRequestUserData *udata,LONG numgad,BOOL dohilight,struct IntuitionBase *IntuitionBase);
void intrequest_initeasyreq(struct IntRequestUserData *udata,struct ExtEasyStruct *ees,struct IntuitionBase *IntuitionBase);
#endif
void render_requester(struct Requester *requester, struct IntuitionBase *IntuitionBase);
void buildreq_draw(struct IntReqDims *dims, struct IntuiText *itext,
                             struct Window *req, struct Screen *scr,
                             struct Gadget *gadgets,
                             struct IntuitionBase *IntuitionBase);

struct Window *buildsysreq_intern(struct Window *window, STRPTR reqtitle, struct IntuiText *bodytext,
				  struct IntuiText *postext, struct IntuiText *negtext,
				  ULONG IDCMPFlags, WORD width, WORD height, struct IntuitionBase *IntuitionBase);
LONG sysreqhandler_intern(struct Window *window, ULONG *IDCMPFlagsPtr, BOOL WaitInput, struct IntuitionBase *IntuitionBase);
void freesysreq_intern(struct Window *window, struct IntuitionBase *IntuitionBase);

struct RawInfo
{
    int Len;
    int Lines;
};

AROS_UFP2(void, RequesterCountChar,
           AROS_UFPA(UBYTE, chr, D0),
           AROS_UFPA(struct RawInfo *,RawInfo,A3)
          );

AROS_UFP2(void, RequesterPutChar,
           AROS_UFPA(UBYTE, chr, D0),
           AROS_UFPA(UBYTE **,buffer,A3)
          );

#endif /* _REQUESTERS_H_ */
