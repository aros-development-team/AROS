#ifndef _REQUESTERS_H_
#define _REQUESTERS_H_
    
/*
    Copyright © 2002-2003 The MorphOS Development Team, All Rights Reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <exec/types.h>
#include <aros/asmcall.h>
#include "intuition_intern.h"

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
    ULONG           	  IDCMP;
    STRPTR          	* GadgetLabels;
    struct Gadget       * Gadgets;
    UWORD           	  NumGadgets;
};
#endif

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

#define OUTERSPACING_X      4
//horiz space between win border and requester's box
#define OUTERSPACING_Y      4
//horiz space between win border and requester's box
#define GADGETGADGETSPACING 8
//space between gadgets
#define TEXTGADGETSPACING   4
//space between textbox and gadgets row
#define TEXTBOXBORDER_X     8
//min space between text and textboxborder
#define TEXTBOXBORDER_Y     4
#define BUTTONBORDER_X      8
//min space between gadaget text and gadget border
#define BUTTONBORDER_Y      4
#define LOGOTEXTSPACING_X   8

#endif /* _REQUESTERS_H_ */
