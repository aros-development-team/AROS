#ifndef INPUTHANDLER_H
#define INPUTHANDLER_H

#ifndef AROS_ASMCALL_H
#   include <aros/asmcall.h>
#endif

#ifndef INTUITION_CGHOOKS_H
#   include <intuition/cghooks.h>
#endif

struct IIHData
{
    struct IntuitionBase	*IntuitionBase;
    struct MsgPort		*IntuiReplyPort;
    struct MsgPort		*IntuiDeferedActionPort;
    struct Gadget		*ActiveGadget;
    struct GadgetInfo		GadgetInfo;
    WORD			LastMouseX;
    WORD			LastMouseY;
    WORD			DeltaMouseX;
    WORD			DeltaMouseY;
    UWORD			ActQualifier;
};

struct Interrupt *InitIIH(struct IntuitionBase *IntuitionBase);
VOID CleanupIIH(struct Interrupt *iihandler, struct IntuitionBase *IntuitionBase);

/* Some useful gadget handling macros */


#if 0
    /* ADDREL macro now in gadgets.h */
    
    /* stegerg: too complicated for macros, because of GZZ wins / GZZ gadgets
      now functions in gadgets.c! And GetLeft, GetTop now return coords
      releative to upper left window edge!!!!!!!!! */
  
    #define GetLeft(gad,w)           (ADDREL(gad,GFLG_RELRIGHT ,w,Width - 1)  + w->LeftEdge + gad->LeftEdge)
    #define GetTop(gad,w)            (ADDREL(gad,GFLG_RELBOTTOM,w,Height - 1) + w->TopEdge  + gad->TopEdge)
    #define GetWidth(gad,w)          (ADDREL(gad,GFLG_RELWIDTH ,w,Width)  + gad->Width)
    #define GetHeight(gad,w)         (ADDREL(gad,GFLG_RELHEIGHT,w,Height) + gad->Height)


    /* stegerg: now a function in inputhandler.c */
    #define InsideGadget(w,gad,x,y)   \
		((x) >= GetLeft(gad,w) && (y) >= GetTop(gad,w) \
		 && (x) < GetLeft(gad,w) + GetWidth(gad,w) \
		 && (y) < GetTop(gad,w) + GetHeight(gad,w))

#endif

AROS_UFP2(struct InputEvent *, IntuiInputHandler,
    AROS_UFPA(struct InputEvent *,      oldchain,       A0),
    AROS_UFPA(struct IIHData *,         iihdata,        A1)
);

#endif /* INPUTHANDLER_H */
