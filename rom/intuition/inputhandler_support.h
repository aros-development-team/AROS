/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Support functions for Intuition's InputHandler
    Lang: english
*/

void notify_mousemove_screensandwindows(WORD x, 
                                        WORD y, 
                                        struct IntuitionBase * IntuitionBase);

void notify_newprefs(struct IntuitionBase * IntuitionBase);

/*********************************************************************/

void send_intuimessage(struct IntuiMessage *imsg,
		       struct Window *w,
		       struct IntuitionBase *IntuitionBase);

void free_intuimessage(struct IntuiMessage *imsg,
		       struct IntuitionBase *IntuitionBase);
		       
struct IntuiMessage *alloc_intuimessage(struct Window *w,
					struct IntuitionBase *IntuitionBase);

BOOL fire_intuimessage(struct Window *w,
                       ULONG Class,
		       UWORD Code,
		       APTR IAddress,
		       struct IntuitionBase *IntuitionBase);

/* use ih_fire_intuimessage if A) the inputevent because of which
   you call this function might have to be eaten or modified
   by Intuition or B) an inputevent might have to be created
   by Intuition because of a deferred action */
   
BOOL ih_fire_intuimessage(struct Window *w,
                       	  ULONG Class,
		       	  UWORD Code,
		       	  APTR IAddress,
		       	  struct IntuitionBase *IntuitionBase);

/*********************************************************************/

IPTR Locked_DoMethodA (Object * obj,
		       Msg message,
		       struct IntuitionBase *IntuitionBase);
		       
		       
/*********************************************************************/

void NotifyDepthArrangement(struct Window *w,
		       	    struct IntuitionBase *IntuitionBase);

/*********************************************************************/

void PrepareGadgetInfo(struct GadgetInfo *gi, struct Screen *scr, struct Window *win);

void SetGadgetInfoGadget(struct GadgetInfo *gi, struct Gadget *gad);

void SetGPIMouseCoords(struct gpInput *gpi, struct Gadget *gad);

void HandleSysGadgetVerify(struct GadgetInfo *gi, struct Gadget *gadget,
    	    	    	   struct IntuitionBase *IntuitionBase);

struct Gadget *HandleCustomGadgetRetVal(IPTR retval, struct GadgetInfo *gi, struct Gadget *gadget,
					ULONG termination,
					BOOL *reuse_event, struct IntuitionBase *IntuitionBase);

struct Gadget *DoGPInput(struct GadgetInfo *gi, struct Gadget *gadget,
			 struct InputEvent *ie, STACKULONG methodid,
			 BOOL *reuse_event, struct IntuitionBase *IntuitionBase);
			 
struct Gadget * FindGadget (struct Screen *scr, struct Window * window, int x, int y,
			    struct GadgetInfo * gi, struct IntuitionBase *IntuitionBase);

BOOL InsideGadget(struct Screen *scr, struct Window *win, struct Gadget *gad,
		  WORD x, WORD y);

struct Gadget *DoActivateGadget(struct Window *win, struct Gadget *gad, struct IntuitionBase *IntuitionBase);

struct Gadget *FindCycleGadget(struct Window *win, struct Gadget *gad, WORD direction);

/*********************************************************************/

void FixWindowCoords(struct Window *win, WORD *left, WORD *top, WORD *width, WORD *height);

/*********************************************************************/

void WindowNeedsRefresh(struct Window * w, struct IntuitionBase * IntuitionBase );

struct Window *FindActiveWindow(struct InputEvent *ie, BOOL *swallow_event,
				struct IntuitionBase *IntuitionBase);

/*********************************************************************/

struct InputEvent *AllocInputEvent(struct IIHData *iihdata);
void FreeGeneratedInputEvents(struct IIHData *iihdata);

/*********************************************************************/

BOOL FireMenuMessage(WORD code, struct Window *win,
		     struct InputEvent *ie, struct IntuitionBase *IntuitionBase);
		     
/*********************************************************************/

LONG Gad_BeginUpdate(struct Layer *layer, struct IntuitionBase *IntuitionBase);
void Gad_EndUpdate(struct Layer *layer, UWORD flag, struct IntuitionBase *IntuitionBase);

/*********************************************************************/
