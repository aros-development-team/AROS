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

AROS_UFP3(struct InputEvent *, IntuiInputHandler,
    AROS_UFPA(struct InputEvent *,      oldchain,       A0),
    AROS_UFPA(struct IIHData *,         iihdata,        A1),
    AROS_UFPA(struct Window *,          w,              A2)
);

#endif /* INPUTHANDLER_H */
