#ifndef INPUTHANDLER_H
#define INPUTHANDLER_H

#ifndef AROS_ASMCALL_H
#   include <aros/asmcall.h>
#endif

struct IIHData
{
    struct IntuitionBase	*IntuitionBase;
    struct MsgPort		*IntuiReplyPort;
    struct Gadget		*ActiveGadget;
    WORD			LastMouseX;
    WORD			LastMouseY;
};

struct Interrupt *InitIIH(struct IntuitionBase *IntuitionBase);
VOID CleanupIIH(struct Interrupt *iihandler, struct IntuitionBase *IntuitionBase);


AROS_UFP2(struct InputEvent *, IntuiInputHandler,
    AROS_UFPA(struct InputEvent *,      oldchain,       A0),
    AROS_UFPA(struct IIHData *,         iihdata,        A1)
);

#endif /* INPUTHANDLER_H */
