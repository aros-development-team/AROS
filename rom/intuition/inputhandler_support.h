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

void fire_intuimessage(struct Window *w,
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

void PrepareGadgetInfo(struct GadgetInfo *gi, struct Window *win);

void SetGadgetInfoGadget(struct GadgetInfo *gi, struct Gadget *gad);

struct Gadget *HandleCustomGadgetRetVal(IPTR retval, struct GadgetInfo *gi, struct Gadget *gadget,
					ULONG *termination,
					BOOL *reuse_event,struct IntuitionBase *IntuitionBase);

struct Gadget * FindGadget (struct Window * window, int x, int y,
			    struct GadgetInfo * gi, struct IntuitionBase *IntuitionBase);


/*********************************************************************/

void FixWindowCoords(struct Window *win, WORD *left, WORD *top, WORD *width, WORD *height);

/*********************************************************************/
/*********************************************************************/
/*********************************************************************/
/*********************************************************************/
/*********************************************************************/
/*********************************************************************/
/*********************************************************************/
/*********************************************************************/
/*********************************************************************/
/*********************************************************************/
/*********************************************************************/
/*********************************************************************/
/*********************************************************************/
/*********************************************************************/
/*********************************************************************/
