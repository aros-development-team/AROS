#ifndef INPUTHANDLER_H
#define INPUTHANDLER_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#ifndef AROS_ASMCALL_H
#   include <aros/asmcall.h>
#endif

#ifndef INTUITION_CGHOOKS_H
#   include <intuition/cghooks.h>
#endif

// FIXME: cannot enable PROPHACK on AROS because of MOS-specific code
//#define PROPHACK

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
    WORD                     DeltaMouseX_Correction;
    WORD                     DeltaMouseY_Correction;
    BOOL                     MouseBoundsActiveFlag;
    LONG                    MouseBoundsLeft;
    LONG                    MouseBoundsTop;
    LONG                    MouseBoundsRight;
    LONG                    MouseBoundsBottom;
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


AROS_UFP2(struct InputEvent *, IntuiInputHandler,
          AROS_UFPA(struct InputEvent *,      oldchain,       A0),
          AROS_UFPA(struct IIHData *,         iihdata,        A1)
         );

#endif /* INPUTHANDLER_H */
