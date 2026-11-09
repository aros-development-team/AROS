#ifndef CONSOLEIF_H
#define CONSOLEIF_H
/*
    Copyright © 1995-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include for the console class
    Lang: english
*/

#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif
#ifndef PROTO_INTUITION_H
#   include <proto/intuition.h>
#endif
#ifndef PROTO_ALIB_H
#   include <proto/alib.h>
#endif
#ifndef DEVICES_INPUTEVENT_H
#   include <devices/inputevent.h>
#endif

enum
{
    A_Console_Window = TAG_USER + 1
};

enum
{
    M_Console_Write = TAG_USER + 1,
    M_Console_Scroll,
    M_Console_DoCommand,
    M_Console_Left,
    M_Console_Right,
    M_Console_Up,
    M_Console_Down,
    M_Console_GetDefaultParams,
    M_Console_RenderCursor,
    M_Console_UnRenderCursor,
    M_Console_ClearCell,
    M_Console_NewWindowSize,
    M_Console_HandleGadgets,
    M_Console_Copy,
    M_Console_Paste,
    M_Console_GetColorPen = M_Console_Paste + 5
};

struct P_Console_ScrollDown
{
    ULONG MethodID;
    WORD LinePos;               /* Lines including this one will be scrolled */

};

struct P_Console_ScrollUp
{
    ULONG MethodID;
    WORD LinePos;               /* Lines including this one will be scrolled */

};

struct P_Console_DoCommand
{
    ULONG MethodID;
    BYTE Command;               /* Erase in display, scroll, next line etc.. */
    UBYTE NumParams;
    IPTR *Params;               /* The command's parameters */
};

struct P_Console_Left
{
    ULONG MethodID;
    IPTR Num;
};
struct P_Console_Right
{
    ULONG MethodID;
    IPTR Num;
};
struct P_Console_Up
{
    ULONG MethodID;
    IPTR Num;
};

struct P_Console_Down
{
    ULONG MethodID;
    IPTR Num;
};

struct P_Console_RenderCursor
{
    ULONG MethodID;
};

struct P_Console_UnRenderCursor
{
    ULONG MethodID;
};

struct P_Console_ClearCell
{
    ULONG MethodID;
    WORD X;
    WORD Y;
};

struct P_Console_NewWindowSize
{
    ULONG MethodID;
};

struct P_Console_Copy
{
    ULONG MethodID;
};

struct P_Console_Paste
{
    ULONG MethodID;
};

struct P_Console_HandleGadgets
{
    ULONG MethodID;
    struct InputEvent *Event;
};

struct P_Console_GetDefaultParams
{
    ULONG MethodID;
    BYTE Command;
    IPTR *Params;
};

struct P_Console_GetColorPen
{
    ULONG MethodID;
    UWORD ColorIdx;
    UBYTE *PenPtr;
};

/*
 * Method stubs. Each builds the message for one method of the console
 * classes and sends it to the object; the result is that of the method.
 */

/* Pass a method on to the superclass of cl */
static inline IPTR Console_Super(Class *cl, Object *o, Msg msg)
{
    return DoSuperMethodA(cl, o, msg);
}

static inline IPTR Console_DoCommand(Object *o, BYTE cmd, UBYTE numparams,
    APTR params)
{
    struct P_Console_DoCommand p;

    p.MethodID = M_Console_DoCommand;
    p.Command = cmd;
    p.NumParams = numparams;
    p.Params = (IPTR *)params;
    return DoMethodA(o, (Msg)&p);
}

/*
 * M_Console_Left, M_Console_Right, M_Console_Up and M_Console_Down take
 * the same message, so one stub sends any of them.
 */
_Static_assert(sizeof(struct P_Console_Right) == sizeof(struct P_Console_Left)
    && sizeof(struct P_Console_Up) == sizeof(struct P_Console_Left)
    && sizeof(struct P_Console_Down) == sizeof(struct P_Console_Left),
    "the cursor movement messages must have the same layout");

static inline IPTR Console_Move(Object *o, ULONG mid, IPTR num)
{
    struct P_Console_Left p;

    p.MethodID = mid;
    p.Num = num;
    return DoMethodA(o, (Msg)&p);
}

#define Console_Left(o, num)    Console_Move((o), M_Console_Left, (num))
#define Console_Right(o, num)   Console_Move((o), M_Console_Right, (num))
#define Console_Up(o, num)      Console_Move((o), M_Console_Up, (num))
#define Console_Down(o, num)    Console_Move((o), M_Console_Down, (num))

static inline IPTR Console_GetDefaultParams(Object *o, BYTE cmd, IPTR *params)
{
    struct P_Console_GetDefaultParams p;

    p.MethodID = M_Console_GetDefaultParams;
    p.Command = cmd;
    p.Params = params;
    return DoMethodA(o, (Msg)&p);
}

static inline IPTR Console_GetColorPen(Object *o, UWORD coloridx,
    UBYTE *penptr)
{
    struct P_Console_GetColorPen p;

    p.MethodID = M_Console_GetColorPen;
    p.ColorIdx = coloridx;
    p.PenPtr = penptr;
    return DoMethodA(o, (Msg)&p);
}

static inline IPTR Console_RenderCursor(Object *o)
{
    struct P_Console_RenderCursor p;

    p.MethodID = M_Console_RenderCursor;
    return DoMethodA(o, (Msg)&p);
}

static inline IPTR Console_UnRenderCursor(Object *o)
{
    struct P_Console_UnRenderCursor p;

    p.MethodID = M_Console_UnRenderCursor;
    return DoMethodA(o, (Msg)&p);
}

#define Console_ClearCell(o, x, y)		\
({						\
    struct P_Console_ClearCell p;		\
    p.MethodID	= M_Console_ClearCell;		\
    p.X		= x;				\
    p.Y		= y;				\
    DoMethodA((o), (Msg)&p);			\
})

static inline IPTR Console_NewWindowSize(Object *o)
{
    struct P_Console_NewWindowSize p;

    p.MethodID = M_Console_NewWindowSize;
    return DoMethodA(o, (Msg)&p);
}

static inline IPTR Console_HandleGadgets(Object *o, struct InputEvent *e)
{
    struct P_Console_HandleGadgets p;

    p.MethodID = M_Console_HandleGadgets;
    p.Event = e;
    return DoMethodA(o, (Msg)&p);
}

#define Console_Copy(o)	       	\
({						\
  struct P_Console_Copy p;			\
  p.MethodID	= M_Console_Copy;		\
  DoMethodA((o), (Msg)&p);			\
 })

#define Console_Paste(o)	       	\
({						\
  struct P_Console_Paste p;			\
  p.MethodID	= M_Console_Paste;		\
  DoMethodA((o), (Msg)&p);			\
})


#endif /* CONSOLEIF_H */
