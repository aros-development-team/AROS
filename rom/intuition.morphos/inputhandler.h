/*
    (C) 1995-2001 AROS - The Amiga Research OS
    $Id$
 
    Desc: Header for Intuition's InputHandler
    Lang: english
*/

#ifndef INPUTHANDLER_H
#define INPUTHANDLER_H

#ifndef AROS_ASMCALL_H
#   include <aros/asmcall.h>
#endif

#ifndef INTUITION_CGHOOKS_H
#   include <intuition/cghooks.h>
#endif

#define PROPHACK

struct IIHData
{
    struct IntuitionBase    *IntuitionBase;
    struct MsgPort          *IntuiReplyPort;
    struct MinList           IntuiActionQueue;
    /* Inputevents allocated from the input.device task but outside our
     * handler are not sent in the input chain until the next time our
     * handler exits. So they must not be freed at the start of our handler.
     * Thus we maintain two lists of allocated input events. The 'New' list
     * contains events that have not yet been propagated, and the other one
     * contains those that have. At the beginning of our handler, the second
     * list is freed. At the end, the first one is transfered in the second.
     * When an event is allocated, it is put in the first list.
     */
    struct MinList           NewAllocatedInputEventList;
    struct MinList           AllocatedInputEventList;
    struct Gadget           *ActiveGadget;
    struct Gadget           *ActiveSysGadget;
    struct Gadget           *MasterDragGadget;
    struct Gadget           *MasterSizeGadget;
    struct GadgetInfo        GadgetInfo;
    struct RastPort          GadgetInfoRastPort;
    struct InputEvent       *FreeInputEvents;
    struct InputEvent       *ReturnInputEvent;
    struct InputEvent      **EndInputEventChain;
    struct Task             *InputDeviceTask;
    struct Window           *MenuWindow; /* The window for which the menus are actually active (on screen) */
    struct Window           *NewActWindow;
    struct IENewTablet      *ActEventTablet; /* not cacheable, valid when processing a single event */
#ifdef PROPHACK
    struct Task             *PropTask;
#endif
    APTR                     InputEventMemPool;
    APTR                     ActionsMemPool;
    WORD                     LastMouseX;
    WORD                     LastMouseY;
    WORD                     DeltaMouseX;
    WORD                     DeltaMouseY;
    UWORD                    ActQualifier;
    UWORD                    PrevKeyMouseState;
    UWORD                    ActKeyMouseState;
    struct Gadget           *LastHelpGadget;
    struct Window           *LastHelpWindow;
    UQUAD                    HelpGadgetFindTime;
    UQUAD                    TitlebarAppearTime;
    BOOL                     TitlebarOnTop;
    BOOL                     MouseWasInsideBoolGadget;

};

struct GeneratedInputEvent
{
    struct MinNode    node;
    struct InputEvent ie;
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
